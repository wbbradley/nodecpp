#pragma once
#include "http_response.h"
#include "http_request.h"
#include <vector>

struct http_field_t
{
	std::string key;
	std::string value;
};

struct http_client_response_t
{
	std::string version;
	int code;
	std::string reason;
	std::vector<http_field_t> fields;
};

using response_callback_t = std::function<void(const http_client_response_t &)>;

/* nodecpp as client API */
void http_get(const std::string &hostname, int port, response_callback_t &&callback);

