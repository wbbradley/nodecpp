#include "http_fetch_op.h"
#include "logger_decls.h"
#include <sstream>
#include <assert.h>
#include <utility>

int on_url(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

int on_body(http_parser *parser, const char *at, size_t length)
{
	dlog(log_info, "%s : %s\n", __FUNCTION__, std::string(at, length).c_str());
	return 0;
}

int on_message_begin(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	return 0;
}

int on_headers_complete(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	return 0;
}

int on_message_complete(http_parser *parser)
{
	dlog(log_info, "%s : HTTP/%d.%d status = %d\n", __FUNCTION__,
			parser->http_major, parser->http_minor, parser->status_code);
	return 0;
}

static http_parser_settings parser_settings = 
{
	on_message_begin,
	on_url,
	on_header_field,
	on_header_value,
	on_headers_complete,
	on_body,
	on_message_complete,
};

http_fetch_op_t::http_fetch_op_t(
		const std::string &hostname,
	   	int port,
	   	const std::function<void(const http_response_t &)> &callback)
: hostname(hostname), port(port), callback(callback)
{
	http_parser_init(&parser, HTTP_REQUEST);
}

void http_fetch_op_t::after_write(uv_write_t *write_req, int status)
{
}

void http_fetch_op_t::on_close(uv_handle_t *handle)
{
	free(handle);
}

void http_fetch_op_t::on_read(uv_stream_t *tcp_handle, ssize_t nread, uv_buf_t buf)
{
	auto &self = *static_cast<http_fetch_op_t *>(tcp_handle->data);

	if (nread < 0)
	{
		if (uv_last_error(uv_default_loop()).code == UV_EOF)
		{
			/* No more data. Close the connection. */
			uv_close((uv_handle_t *)tcp_handle, on_close);
			self.callback(self.response);
		}
		else
		{
			abort();
		}
	}

	if (nread > 0)
	{
		if (nread != http_parser_execute(&self.parser, &parser_settings, buf.base, nread))
		{
			dlog(log_error, "unexpected thing : http_parser_execute didn't read all my bytes! (%s)\n",
					std::string(buf.base, nread).c_str());
		}
	}

	dlog(log_info, "--- (freeing %ju)\n", (uintmax_t)(buf.base));
	delete buf.base;
}

uv_buf_t http_fetch_op_t::on_alloc(uv_handle_t* handle, size_t suggested_size)
{
	uv_buf_t buf;
	buf.base = new char[suggested_size];
	buf.len = (buf.base != nullptr) ? suggested_size : 0;
	dlog(log_info, "on_alloc (allocating %ju)\n", (uintmax_t)(buf.base));
	return buf;
}

void http_fetch_op_t::after_connect(uv_connect_t *connect_req, int status)
{
	if (status < 0)
		abort();

	uv_write_t *write_req = new uv_write_t;
	write_req->data = connect_req->data;
	std::stringstream ss;
	ss << "GET / HTTP/1.0\r\n";
	ss << "Host: " << static_cast<http_fetch_op_t *>(connect_req->data)->hostname.c_str();
	ss << "\r\n\r\n";

	std::string header = ss.str();

	uv_buf_t buf;
	buf.base = new char[header.size()];
	memcpy(buf.base, header.c_str(), header.size());
	buf.len = header.size();

	uv_write(write_req, connect_req->handle, &buf, 1, after_write);

	delete buf.base;

	assert(connect_req->handle->data == nullptr);
	connect_req->handle->data = write_req->data;

	uv_read_start(connect_req->handle, on_alloc, on_read);

	delete connect_req;
}

void http_fetch_op_t::after_getaddrinfo(
		uv_getaddrinfo_t *gai_req,
	   	int status,
	   	struct addrinfo *ai)
{
	uv_tcp_t *tcp_handle;
	uv_connect_t *connect_req;
	if (status < 0)
		abort();

	tcp_handle = new uv_tcp_t;
	uv_tcp_init(uv_default_loop(), tcp_handle);

	connect_req = new uv_connect_t;
	connect_req->data = gai_req->data;
	uv_tcp_connect(connect_req,
			tcp_handle,
			*(struct sockaddr_in *)ai->ai_addr,
			after_connect);

	delete gai_req;
	uv_freeaddrinfo(ai);
}

void http_get(
		const std::string &hostname,
	   	int port,
		std::function<void(const http_response_t &)> &&callback)
{
	http_fetch_op_t *http_fetch_op = new http_fetch_op_t(hostname, port, std::move(callback));

	uv_getaddrinfo_t *gai_req = new uv_getaddrinfo_t;
	gai_req->data = http_fetch_op;

	std::stringstream ss;
	ss << port;

	uv_getaddrinfo(uv_default_loop(),
			gai_req,
			http_fetch_op_t::after_getaddrinfo,
			hostname.c_str(),
			ss.str().c_str(),
			NULL);

}
