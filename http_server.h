#pragma once
#include "http_connection.h"

typedef std::function<void(const http_request_ptr_t &request, const http_response_ptr_t &response)> http_route_handler_t;

/* nodecpp as server API */
void http_use_route(const std::string &path, http_method method, http_route_handler_t &&handler);
void http_listen(int port, int backlog);

void http_server_connection_close(uv_handle_t *handle);

