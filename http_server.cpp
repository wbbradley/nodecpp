#include "logger_decls.h"
#include "http.h"
#include "nodecpp_errors.h"
#include <assert.h>
#include "unordered.h"
#include "nocopy.h"
#include <memory>
#include <utility>
#include "http_parser.h"
#include <uv.h>

using std::shared_ptr;

struct route_info_t
{
	NOCOPY(route_info_t);
	route_info_t(get_route_handler_t &&handler) : handler(std::move(handler))
	{
	}

	get_route_handler_t handler;
};

using route_info_ptr_t = shared_ptr<route_info_t>;

/* global GET routes */
static unordered_map<std::string, route_info_ptr_t> http_get_routes;

void http_get_route(const std::string &path, get_route_handler_t &&handler)
{
	route_info_ptr_t route_info(new route_info_t(std::move(handler)));
	dlog(log_info, "installing http_get_route handler for \"%s\"\n", path.c_str());
	http_get_routes[path] = route_info;
}

void http_close(uv_handle_t* handle)
{
	dlog(log_info, "closing connection handle\n");

	// TODO cleanup handle->data
	assert(handle->data == nullptr);

	delete handle;
}

void http_connection(uv_stream_t *server, int status)
{
	dlog(log_info, "client connection created\n");
	uv_tcp_t *client = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), client);

	if (!uv_accept((uv_stream_t *)server, (uv_stream_t *)client))
	{
		dlog(log_info, "connection accepted\n");

		// TODO start reading the HTTP header
		// TEMP just close the connection
		uv_close((uv_handle_t *)client, http_close);
	}
	else
	{
		log_uv_errors();
	}
}

void http_listen(int port, int backlog)
{
	uv_tcp_t *handle = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), handle);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(port);
	if (!uv_tcp_bind(handle,  addr))
	{
		dlog(log_info, "bound tcp server to port %d\n", port);
	}
	else
	{
		log_uv_errors();
	}

	if (!uv_listen((uv_stream_t *)handle, backlog, http_connection))
	{
		dlog(log_info, "tcp server is listening\n");
	}
	else
	{
		log_uv_errors();
	}
}

