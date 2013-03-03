#pragma once
#include <string>

struct http_respond_t
{
	void set_response(int code, const std::string &content_type);
	void send(const std::string &payload);
	void end();
private:
	bool ended = false;
};
