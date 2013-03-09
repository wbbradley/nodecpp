#pragma once
#include "http_parser.h"
#include <string>
#include "unordered.h"
#include <assert.h>
#include "nocopy.h"
#include <memory>
#include "utils.h"

struct http_connection_t;

/* http_request_t informs the server handler what the client is asking for */
struct http_request_t : static_count<http_request_t>
{
	friend struct http_connection_t;

	NOCOPY(http_request_t);
	http_request_t(http_method method, const std::string &target_uri);

	http_method method;
	unordered_map<std::string, std::string> query_params;

	std::string uri_path() const;
	const std::string &body() const { assert(method == HTTP_POST); return _body; }

	bool keep_alive() const;

private:
	http_parser parser;
	std::string target_uri;
	http_parser_url parsed_url;
	std::string _body;
};

typedef std::shared_ptr<http_request_t> http_request_ptr_t;
