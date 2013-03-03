#pragma once
#include "http_response.h"
#include "http_request.h"

using response_callback_t = std::function<void(const http_response_t &)>;

/* nodecpp as client API */
void http_get(const std::string &hostname, int port, response_callback_t &&callback);



