#include "http_connection.h"
#include "logger_decls.h"
#include <assert.h>
#include <uv.h>
#include "nodecpp_errors.h"
#include <sstream>
#include "http_server.h"

extern void http_server_dispatch(const http_connection_ptr_t &connection, const http_request_ptr_t &request);

http_connection_t::http_connection_t(uv_stream_t *client_handle) : client_handle(client_handle)
{
	assert(client_handle != nullptr);
	assert(client_handle->data == nullptr);

	reset_parser();
}

std::shared_ptr<http_request_t> http_connection_t::pop_request()
{
	dlog(log_warning, "%s called on [%d]\n", __FUNCTION__, instance_id);
	reset_parser();
	std::shared_ptr<http_request_t> top_request;
	std::swap(top_request, request_being_created);
	return top_request;
}

void http_connection_t::reset_parser()
{
	http_parser_init(&parser, HTTP_REQUEST);
	parser.data = this;
}

void http_connection_t::start_request(http_method method, const std::string &target_uri)
{
	dlog(log_warning, "%s called on [%d]\n", __FUNCTION__, instance_id);
	request_being_created.reset(new http_request_t(method, target_uri));
}

static int http_connection_url(http_parser *parser, const char *at, size_t length)
{
	auto connection = (http_connection_t *)parser->data;
	connection->start_request((http_method)parser->method,
			std::string(at, length));

	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

static int http_connection_header_field(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

static int http_connection_header_value(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

static int http_connection_body(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

static int http_connection_message_begin(http_parser *parser)
{
	dlog(log_info, "%s\n", __FUNCTION__);
	return 0;
}

static int http_connection_status_complete(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	return 0;
}

static int http_connection_headers_complete(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	return 0;
}

static int http_connection_message_complete(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	auto connection = ((http_connection_t *)parser->data)->shared_from_this();

	connection->queue_request(connection->pop_request());

	return 0;
}

static const http_parser_settings parser_settings =
{
	http_connection_message_begin,
	http_connection_url,
	http_connection_status_complete,
	http_connection_header_field,
	http_connection_header_value,
	http_connection_headers_complete,
	http_connection_body,
	http_connection_message_complete,
};

void http_connection_t::parse_http(const char *buf, int nread)
{
	auto parsed_count = http_parser_execute(&parser, &parser_settings, buf, nread);
	if (nread != parsed_count)
	{
		dlog(log_error, "unexpected thing : http_parser_execute only parsed %d/%d bytes! (%s)\n",
				parsed_count, nread, std::string(buf, nread).c_str());

		/* avoid trying to read or write anything, this socket is messed up */
		client_handle = nullptr;
		assert(!"handle this better");
	}
}

http_connection_t::~http_connection_t()
{
	assert(client_handle == nullptr);
}

struct http_connection_write_data_t
{
	NOCOPY(http_connection_write_data_t);

	/* this structure is owned by uv_write_t */
	http_connection_write_data_t(
			const http_connection_ptr_t &connection,
		   	const std::string &payload,
			bool close_after_write)
	   	: connection(connection), payload(payload), close_after_write(close_after_write)
	{
	}

	~http_connection_write_data_t()
	{
	}

	http_connection_ptr_t connection;
	std::string payload;
	bool close_after_write = false;
};

void http_connection_t::http_connection_write_cb(uv_write_t *req, int status)
{
	auto *write_data = (http_connection_write_data_t *)req->data;
	bool close_after_write = write_data->close_after_write;

	if (status != 0)
	{
		dlog(log_warning, "http_connection_write_cb : write "
				"of \"%s\" failed with code %d\n",
				ellipsis(write_data->payload, 10).c_str(),
				status);
	}
	else
	{
		dlog(log_info, "http_connection_write_cb : write of \"%s\" succeeded\n",
				ellipsis(write_data->payload, 10).c_str());
	}

	auto connection = write_data->connection;

	assert(connection != nullptr);

	if ((connection->client_handle != nullptr)
		   	&& !uv_is_closing((uv_handle_t *)connection->client_handle))
	{
		if (close_after_write)
		{
			/* we're done with this connection, make sure to close it */
			dlog(log_warning, "%s closing socket %ju [%d] after writing %s\n",
					__FUNCTION__, uintmax_t(connection->client_handle),
					(int)connection->instance_id,
					ellipsis(write_data->payload, 10).c_str());
			uv_close((uv_handle_t *)connection->client_handle,
					http_server_connection_close);
		}
		else
		{
			/* okay, the app may want to keep sending chunks */
		}
	}
	else
	{
		/* client handle has already been closed, I guess. */
	}

	delete write_data;
	delete req;
}

void http_connection_t::request_completed()
{
	request_in_service.reset();

	// TODO eliminate the recursion that could happen here
	// by queueing the dispatch
	service_next_request();
}

void http_connection_t::service_next_request()
{
	assert(request_in_service == nullptr);

	if (!request_queue.empty())
	{
		dlog(log_warning, "%s : request queue not empty\n", __FUNCTION__);
		request_in_service = request_queue.front();
		request_queue.pop();
		http_server_dispatch(shared_from_this(), request_in_service);
	}
}

void http_connection_t::queue_request(const http_request_ptr_t &request)
{
	if (request_queue.empty() && (request_in_service == nullptr))
	{
		request_in_service = request;
		dlog(log_warning, "%s : dispatching\n", __FUNCTION__);
		http_server_dispatch(shared_from_this(), request_in_service);
	}
	else
	{
		request_queue.push(request);
	}
}

void http_connection_t::queue_write(
		const std::string &write_blob,
	   	bool close_after_write)
{
	if (client_handle != nullptr)
	{
		assert(!uv_is_closing((uv_handle_t *)client_handle));
		dlog(log_info, "queue_write on socket handle %ju called with \"%s\"\n", uintmax_t(client_handle), write_blob.c_str());
		/* create a new write request */
		uv_write_t *write_req = new uv_write_t;

		/* create a new data nugget for the write request */
		http_connection_write_data_t *write_data = new http_connection_write_data_t(shared_from_this(), write_blob, close_after_write);
		write_req->data = write_data;

		/* create pointers to the data nugget */
		uv_buf_t buf;
		buf.base = const_cast<char *>(write_data->payload.c_str());
		buf.len = write_data->payload.size();

		/* submit the write to the queue */
		if (uv_write(write_req, client_handle, &buf, 1 /*bufcnt*/,
					http_connection_write_cb))
		{
			log_uv_errors();
		}
	}
	else
	{
		dlog(log_warning, "%s attempt to write to null client_handle\n", __FUNCTION__);
	}
}

