#pragma once
#include "http_parser.h"
#include <string>
#include "unordered.h"
#include <assert.h>
#include "nocopy.h"

/* http_request_t informs the server handler what the client is asking for */
struct http_request_t
{
	NOCOPY(http_request_t);
	http_request_t(http_method method, const std::string &target_uri);
	http_method method;
	std::string target_uri;
	unordered_map<std::string, std::string> query_params;

	const std::string &body() const { assert(method == HTTP_POST); return _body; }

private:
	std::string _body;
};

