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
#include "http_connection.h"
#include "utils.h"

using std::shared_ptr;

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

void http_use_route(
		const std::string &path,
	   	http_method method,
	   	http_route_handler_t &&handler)
{
	route_info_ptr_t route_info(new route_info_t(std::move(handler)));
	dlog(log_info, "installing http_use_route handler for \"%s\"\n", path.c_str());
	http_server_methods_routes[method][path] = route_info;
}

void http_server_connection_close(uv_handle_t *handle)
{
	auto connection = (http_connection_ptr_t *)handle->data;

	if (connection != nullptr)
	{
		assert(*connection != nullptr);
		dlog(log_warning, "%s deleting connection %ju [%d]\n",
				__FUNCTION__, uintmax_t((*connection)->client_handle),
				(int)(*connection)->instance_id);
		/* prevent the connection from accessing this handle anymore */
		(*connection)->client_handle = nullptr;
		delete connection;
	}
	else
	{
		dlog(log_error, "closing connection handle couldn't find connection\n");

		assert(0);
	}

	delete handle;
}

#ifdef DEBUG
void dump_routes()
{
	for (auto &http_server_method_routes_pair : http_server_methods_routes)
	{
		auto method = http_server_method_routes_pair.first;
		for (auto &http_server_route_pair : http_server_method_routes_pair.second)
		{
			dlog(log_info, "[%d][%s] is mapped\n", (int)method,
					http_server_route_pair.first.c_str());
		}
	}
}
#endif

int http_server_dispatch(
		http_connection_ptr_t &connection,
	   	const http_request_ptr_t &request)
{
	debug(dump_routes());
	dlog(log_info, "dispatch is looking for [%d][%s]\n", (int)request->method,
			request->uri_path().c_str());
	auto &http_server_routes = http_server_methods_routes[request->method];
	auto iter = http_server_routes.find(request->uri_path());
	if (iter == http_server_routes.end())
	{
		// TODO handle 404?
		dlog(log_info, "route not found for \"%s\"\n", request->uri_path().c_str());

		if (!uv_is_closing((uv_handle_t *)connection->client_handle))
		{
			dlog(log_warning, "%s closing socket %ju [%d]\n",
					__FUNCTION__, uintmax_t(connection->client_handle),
					connection->instance_id);

			/* For now: Close the connection. */
			uv_close((uv_handle_t *)connection->client_handle, http_server_connection_close);
		}

		return 1;
	}

	dlog(log_warning, "%s routing to handler for connection %ju [%d]\n",
			__FUNCTION__, uintmax_t(connection->client_handle),
			connection->instance_id);

	http_response_ptr_t response(new http_response_t(connection));
	auto &route_info = iter->second;
	route_info->handler(request, response);

	return 0;
}

static uv_buf_t http_server_alloc(uv_handle_t *handle, size_t suggested_size)
{
	uv_buf_t buf;
	buf.base = new char[suggested_size];
	buf.len = (buf.base != nullptr) ? suggested_size : 0;
	dlog(log_info, "http_server_alloc (allocating %ju)\n", (uintmax_t)(buf.base));
	return buf;
}

static void http_server_read(uv_stream_t *client_handle, ssize_t nread, uv_buf_t buf)
{
	http_connection_ptr_t *connection_ptr = (http_connection_ptr_t *)client_handle->data;
	assert(client_handle == (*connection_ptr)->client_handle);
	assert(connection_ptr != nullptr);

	if (nread < 0)
	{
		if (uv_last_error(uv_default_loop()).code == UV_EOF)
		{
			dlog(log_warning, "end of socket encountered\n");

			/* No more data. Close the connection. */
			uv_close((uv_handle_t *)client_handle, http_server_connection_close);
		}
		else
		{
			log_uv_errors();
		}
	}
	else if (nread > 0)
	{
		auto &connection = *connection_ptr;
		connection->parse_http(buf.base, nread);
	}

	dlog(log_info, "--- (freeing %ju)\n", (uintmax_t)(buf.base));
	delete buf.base;
}


static void http_connection(uv_stream_t *server, int status)
{
	dlog(log_info, "client connection created\n");
	uv_tcp_t *client_handle = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), client_handle);

	if (!uv_accept((uv_stream_t *)server, (uv_stream_t *)client_handle))
	{
		dlog(log_info, "connection accepted\n");

		/* setup a client connection object */
		assert(client_handle->data == nullptr);
		client_handle->data = new http_connection_ptr_t(new http_connection_t((uv_stream_t *)client_handle));
		client_handle->close_cb = http_server_connection_close;
		if (!uv_read_start((uv_stream_t *)client_handle, http_server_alloc,
					http_server_read))
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

