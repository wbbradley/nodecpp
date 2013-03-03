#pragma once
#include "http_parser.h"
#include <string>
#include "unordered.h"

/* http_request_t informs the server handler what the client is asking for */
struct http_request_t
{
	http_method method;
	std::string target_uri;
	unordered_map<std::string, std::string> query_params;
};

struct http_get_request_t : public http_request_t
{
};

struct http_post_request_t : public http_request_t
{
	const std::string &body() const { return _body; }

private:
	std::string _body;
};

