#pragma once
#include "nocopy.h"
#include "http_parser.h"
#include <string>
#include "http_request.h"
#include "http_response.h"
#include <uv.h>
#include "utils.h"
#include <queue>

/* a server/client connection - in memory on the server */
struct http_connection_t : public std::enable_shared_from_this<http_connection_t>, public static_count<http_connection_t>
{
	NOCOPY(http_connection_t);
	http_connection_t(uv_stream_t *client_handle);
	~http_connection_t();

	/* request API */
	void start_request(http_method method, const std::string &target_uri);
	void parse_http(const char *buf, int nread);
	http_request_ptr_t pop_request();

	uv_stream_t *client_handle;

	void request_completed();
	void queue_request(const http_request_ptr_t &request);
	void queue_write(const std::string &write_blob, bool close_after_write);

private:
	http_parser parser;

	/* request is currently being built up */
	http_request_ptr_t request_being_created;
	http_request_ptr_t request_in_service;
	std::queue<http_request_ptr_t> request_queue;

	void reset_parser();
	void service_next_request();

	static void http_connection_write_cb(uv_write_t *req, int status);
};

typedef std::shared_ptr<http_connection_t> http_connection_ptr_t;

