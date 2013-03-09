#include "http_response.h"
#include "assert.h"
#include "logger_decls.h"
#include <sstream>
#include "http_connection.h"

http_response_t::http_response_t(const http_connection_ptr_t &connection, bool keep_alive)
	: weak_connection(connection), keep_alive(keep_alive)
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
	if (content_complete && !keep_alive)
		close_after_write = true;

	auto connection = weak_connection.lock();
	if (connection != nullptr)
	{
		std::stringstream ss;
		if (!sent_headers)
		{
			ss << "HTTP/" << (keep_alive ? "1.1 ": "1.0 ") << code << " " << reason << "\r\n";
			for (auto &field_pair : fields)
			{
				ss << field_pair.first << ": " << field_pair.second << "\r\n";
			}
			if (payload.size() != 0)
			{
				ss << "Content-Type: " << content_type << "\r\n";
				//ss << "Transfer-Encoding: chunked\r\n";
				if (keep_alive)
					ss << "Connection: keep-alive\r\n";
				ss << "Content-Length: " << payload.size() << "\r\n";
				ss << "\r\n";
			}
		}

		//ss << std::hex << payload.size() << "\r\n";
		ss.write(payload.c_str(), payload.size());
		//ss << "\r\n";

		if (payload.size() != 0 || close_after_write)
			connection->queue_write(ss.str(), close_after_write);

		sent_headers = true;

		if (content_complete)
		{
			dlog(log_info, "request completed\n");
			connection->request_completed();
		}
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


