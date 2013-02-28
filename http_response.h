#pragma once
#include <string>
#include <vector>

struct http_field_t
{
	std::string key;
	std::string value;
};

struct http_response_t
{
	std::string version;
	int code;
	std::string reason;
	std::vector<http_field_t> fields;
};

