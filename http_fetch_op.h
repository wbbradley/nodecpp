#pragma once
#include "http_response.h"
#include <string>
#include <functional>
#include "http_parser.h"
#include <uv.h>

using response_callback_t = std::function<void(const http_response_t &)>;

void http_get(const std::string &hostname, int port, response_callback_t &&callback);

struct http_fetch_op_t
{
	http_fetch_op_t(const std::string &hostname, int port, const std::function<void(const http_response_t &)> &callback);

private:
	std::string hostname;
	int port;
	http_parser parser;
	http_response_t response;
	std::function<void(const http_response_t &res)> callback;

	static void after_write(uv_write_t *write_req, int status);
	static void on_close(uv_handle_t *handle);
	static void on_read(uv_stream_t *tcp_handle, ssize_t nread, uv_buf_t buf);
	static uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size);
	static void after_connect(uv_connect_t *connect_req, int status);
	static void after_getaddrinfo(uv_getaddrinfo_t *gai_req, int status, struct addrinfo *ai);

	friend void http_get(const std::string &hostname, int port, response_callback_t &&callback);
};


