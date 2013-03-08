#include "http_response.h"
#include "assert.h"
#include "logger_decls.h"
#include <sstream>
#include "http_connection.h"

http_response_t::http_response_t(const http_connection_ptr_t &connection)
	: weak_connection(connection)
{
}

void http_response_t::set_response(
		int code,
	   	const std::string &reason,
	   	const std::string &content_type)
{
	assert(weak_connection.lock() != nullptr);

	this->code = code;
	this->reason = reason;
	this->content_type = content_type;
}

void http_response_t::set_header(
		const std::string &key,
	   	const std::string &value)
{
	assert(weak_connection.lock() != nullptr);

	if (fields.find(key) != fields.end())
	{
		dlog(log_warning,
				"http_connection_t : overwriting existing header field \"%s: %s\"\n",
				key.c_str(),
				value.c_str());
	}

	fields[key] = value;
}

void http_response_t::send(
		const std::string &payload,
		bool content_complete,
		bool close_after_write)
{
	auto connection = weak_connection.lock();
	if (connection != nullptr)
	{
		std::stringstream ss;
		if (!sent_headers)
		{
			ss << "HTTP/1.1 " << code << " " << reason << "\r\n";
			for (auto &field_pair : fields)
			{
				ss << field_pair.first << ": " << field_pair.second << "\r\n";
			}
			if (payload.size() != 0)
			{
				ss << "Content-Type: " << content_type << "\r\n";
				ss << "Transfer-Encoding: chunked\r\n\r\n";
			}
		}

		if (sent_headers || (payload.size() != 0))
		{
			ss << std::hex << payload.size() << "\r\n";
			ss.write(payload.c_str(), payload.size());
			ss << "\r\n";
		}

		if (content_complete)
			ss << "\r\n";

		connection->queue_write(ss.str(), close_after_write);

		sent_headers = true;
	}
	else
	{
		dlog(log_warning, "%s bailed out on send\n");
	}
}

void http_response_t::end(bool close_connection)
{
	send("", true /*content_complete*/, close_connection /*close_after_write*/);
}


