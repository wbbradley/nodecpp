#include "logger_decls.h"
#include "http.h"
#include "nodecpp_errors.h"
#include <assert.h>
#include "unordered.h"
#include "nocopy.h"
#include <memory>
#include <utility>
#include "http_parser.h"
#include <uv.h>

using std::shared_ptr;

/* a server/client connection - in memory on the server */
struct http_connection_t
{
	NOCOPY(http_connection_t);
	http_connection_t()
	{
		http_parser_init(&parser, HTTP_REQUEST);
		parser.data = this;
	}

	~http_connection_t()
	{
		//dlog(log_info, "dtor called on connection \"%s\"\n", url.c_str());
	}
	http_parser parser;
	std::string url;
};

struct route_info_t
{
	NOCOPY(route_info_t);
	route_info_t(http_route_handler_t &&handler) : handler(std::move(handler))
	{
	}

	http_route_handler_t handler;
};

using route_info_ptr_t = shared_ptr<route_info_t>;

/* global GET routes */
static unordered_map<decltype(http_parser::method), unordered_map<std::string, route_info_ptr_t>> http_server_methods_routes;

void http_use_route(const std::string &path, http_method method, http_route_handler_t &&handler)
{
	route_info_ptr_t route_info(new route_info_t(std::move(handler)));
	dlog(log_info, "installing http_use_route handler for \"%s\"\n", path.c_str());
	http_server_methods_routes[method][path] = route_info;
}

static int http_server_dispatch(http_connection_t &connection)
{
	http_request_t request((http_method)connection.parser.method, connection.url);

	auto &http_server_routes = http_server_methods_routes[connection.parser.method];
	auto iter = http_server_routes.find(request.uri_path());
	if (iter == http_server_routes.end())
	{
		// TODO handle 404?
		dlog(log_info, "route not found for \"%s\"\n", connection.url.c_str());
		// TODO investigate cleaning up failure cases
		return 1;
	}

	http_respond_t respond;
	iter->second->handler(request, respond);

	return 0;
}

static void http_server_close(uv_handle_t *handle)
{
	dlog(log_info, "closing connection handle\n");

	assert(handle->data != nullptr);
	auto connection = (http_connection_t *)handle->data;
	delete connection;
	delete handle;
}

static uv_buf_t http_server_alloc(uv_handle_t *handle, size_t suggested_size)
{
	uv_buf_t buf;
	buf.base = new char[suggested_size];
	buf.len = (buf.base != nullptr) ? suggested_size : 0;
	dlog(log_info, "http_server_alloc (allocating %ju)\n", (uintmax_t)(buf.base));
	return buf;
}

static int http_server_url(http_parser *parser, const char *at, size_t length)
{
	auto &connection = *(http_connection_t *)parser->data;

	connection.url = std::string(at, length);

	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

int http_server_header_field(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

int http_server_header_value(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

int http_server_body(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

int http_server_message_begin(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	return 0;
}

int http_server_headers_complete(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	return 0;
}

int http_server_message_complete(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	auto &connection = *(http_connection_t *)parser->data;

	return http_server_dispatch(connection);
}

static const http_parser_settings parser_settings =
{
	http_server_message_begin,
	http_server_url,
	http_server_header_field,
	http_server_header_value,
	http_server_headers_complete,
	http_server_body,
	http_server_message_complete,
};


static void http_server_read(uv_stream_t *client_handle, ssize_t nread, uv_buf_t buf)
{
	auto &connection = *(http_connection_t *)client_handle->data;

	if (nread < 0)
	{
		if (uv_last_error(uv_default_loop()).code == UV_EOF)
		{
			/* No more data. Close the connection. */
			uv_close((uv_handle_t *)client_handle, http_server_close);
		}
		else
		{
			log_uv_errors();
		}
	}

	if (nread > 0)
	{
		auto parsed_count = http_parser_execute(&connection.parser, &parser_settings,
				buf.base, nread);
		if (nread != parsed_count)
		{
			dlog(log_error, "unexpected thing : http_parser_execute only parsed %d/%d bytes! (%s)\n",
					parsed_count,
					nread,
					std::string(buf.base, nread).c_str());
		}
	}

	dlog(log_info, "--- (freeing %ju)\n", (uintmax_t)(buf.base));
	delete buf.base;
}


static void http_connection(uv_stream_t *server, int status)
{
	dlog(log_info, "client connection created\n");
	uv_tcp_t *client = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), client);

	if (!uv_accept((uv_stream_t *)server, (uv_stream_t *)client))
	{
		dlog(log_info, "connection accepted\n");

		http_connection_t *connection = new http_connection_t();
		client->data = connection;
		if (!uv_read_start((uv_stream_t *)client, http_server_alloc, http_server_read))
		{
			dlog(log_info, "started reading client connection\n");
		}
		else
		{
			log_uv_errors();
		}
	}
	else
	{
		log_uv_errors();
	}
}

void http_listen(int port, int backlog)
{
	uv_tcp_t *handle = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), handle);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(port);
	if (!uv_tcp_bind(handle,  addr))
	{
		dlog(log_info, "bound tcp server to port %d\n", port);
	}
	else
	{
		log_uv_errors();
	}

	if (!uv_listen((uv_stream_t *)handle, backlog, http_connection))
	{
		dlog(log_info, "tcp server is listening\n");
	}
	else
	{
		log_uv_errors();
	}
}

