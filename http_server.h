#pragma once
#include "http_request.h"
#include "http_respond.h"

using http_route_handler_t = std::function<void(const http_request_t &req, http_respond_t &respond)>;

/* nodecpp as server API */
void http_use_route(const std::string &path, http_method method, http_route_handler_t &&handler);
void http_listen(int port, int backlog);

