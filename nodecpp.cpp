#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "cmd_options.h"
#include "disk.h"
#include "utils.h"
#include <uv.h>
#include <tr1/functional>
#include <assert.h>

const char *option_nodecpp = "nodecpp";
const char *option_verbose = "verbose";

struct http_fetch_op_t
{
	std::string hostname;
	std::tr1::function<void(char *,size_t)> callback;
};


cmd_option_t cmd_options[] =
{
	{ option_nodecpp, "-j" /*opt*/, true /*mandatory*/, true /*has_data*/ },
	{ option_verbose, "-v" /*opt*/, false /*mandatory*/, false /*has_data*/ },
};

void after_write(uv_write_t *write_req, int status)
{
}

void on_close(uv_handle_t *handle)
{
	free(handle);
}

void on_read(uv_stream_t *tcp_handle, ssize_t nread, uv_buf_t buf)
{
	if (nread < 0)
	{
		if (uv_last_error(uv_default_loop()).code == UV_EOF)
		{
			/* No more data. Close the connection. */
			uv_close((uv_handle_t *)tcp_handle, on_close);
		}
		else
		{
			abort();
		}
	}

	if (nread > 0)
	{
		static_cast<http_fetch_op_t *>(tcp_handle->data)->callback(
				buf.base, nread);
	}

	dlog(log_info, "--- (freeing %ju)\n", (uintmax_t)(buf.base));
	delete buf.base;
}

uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size)
{
	uv_buf_t buf;
	buf.base = new char[suggested_size];
	buf.len = (buf.base != nullptr) ? suggested_size : 0;
	dlog(log_info, "on_alloc (allocating %ju)\n", (uintmax_t)(buf.base));
	return buf;
}

void after_connect(uv_connect_t *connect_req, int status)
{
	if (status < 0)
		abort();

	uv_write_t *write_req = new uv_write_t;
	write_req->data = connect_req->data;
	std::stringstream ss;
	ss << "GET / HTTP/1.0\r\n";
	ss << "Host: " << static_cast<http_fetch_op_t *>(connect_req->data)->hostname.c_str();
	ss << "\r\n\r\n";

	std::string header = ss.str();

	uv_buf_t buf;
	buf.base = new char[header.size()];
	memcpy(buf.base, header.c_str(), header.size());
	buf.len = header.size();

	uv_write(write_req, connect_req->handle, &buf, 1, after_write);

	delete buf.base;

	assert(connect_req->handle->data == nullptr);
	connect_req->handle->data = write_req->data;

	uv_read_start(connect_req->handle, on_alloc, on_read);

	delete connect_req;
}

void after_getaddrinfo(uv_getaddrinfo_t *gai_req, int status, struct addrinfo *ai)
{
	uv_tcp_t *tcp_handle;
	uv_connect_t *connect_req;
	if (status < 0)
		abort();

	tcp_handle = new uv_tcp_t;
	uv_tcp_init(uv_default_loop(), tcp_handle);

	connect_req = new uv_connect_t;
	connect_req->data = gai_req->data;
	uv_tcp_connect(connect_req,
			tcp_handle,
			*(struct sockaddr_in *) ai->ai_addr,
			after_connect);

	delete gai_req;
	uv_freeaddrinfo(ai);
}

int main(int argc, char *argv[])
{
	cmd_options_t options;
	if (!get_options(argc, argv, cmd_options, countof(cmd_options), options))
		return EXIT_FAILURE;

	if (get_option_exists(options, option_verbose))
		log_enable(log_error | log_warning | log_info);
	else
		log_enable(log_error);

	std::string nodecpp;
	get_option(options, option_nodecpp, nodecpp);

	http_fetch_op_t *http_fetch_op = new http_fetch_op_t;
	http_fetch_op->callback = [=](char *buffer, size_t len) {
		fprintf(stdout, "[%s] ", nodecpp.c_str());
		fwrite(buffer, 1, len, stdout);
	};
	uv_getaddrinfo_t *gai_req = new uv_getaddrinfo_t;
	gai_req->data = http_fetch_op;

	uv_getaddrinfo(uv_default_loop(),
			gai_req,
			after_getaddrinfo,
			nodecpp.c_str(),
			"80",
			NULL);

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	return EXIT_SUCCESS;
}

