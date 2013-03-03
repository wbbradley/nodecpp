#pragma once
#include "http_request.h"
#include "http_respond.h"

using get_route_handler_t = std::function<void(const http_get_request_t &req, http_respond_t &respond)>;

/* nodecpp as server API */
void http_get_route(const std::string &path, get_route_handler_t &&handler);
void http_listen(int port, int backlog);

