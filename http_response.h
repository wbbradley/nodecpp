#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "nocopy.h"

struct http_connection_t;
typedef std::shared_ptr<http_connection_t> http_connection_ptr_t;
typedef std::weak_ptr<http_connection_t> http_connection_weak_ptr_t;

struct http_response_t : public std::enable_shared_from_this<http_response_t>
{
	NOCOPY(http_response_t);
	http_response_t(const http_connection_ptr_t &connection, bool keep_alive);

	void set_response(int code, const std::string &reason, const std::string &content_type);
	void set_header(const std::string &key, const std::string &value);
	void send(const std::string &payload, bool content_complete = false, bool close_after_write = false);
	void end(bool close_connection = false);

private:
	http_connection_weak_ptr_t weak_connection;

	bool keep_alive;
	bool sent_headers = false;
	int code = 200;
	std::string reason = "OK";
	std::string content_type;
	std::unordered_map<std::string, std::string> fields;

};

typedef std::shared_ptr<http_response_t> http_response_ptr_t;

