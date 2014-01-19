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
#include <functional>
#include <assert.h>
#include "http.h"

const char *option_get = "GET";
const char *option_verbose = "verbose";

cmd_option_t cmd_options[] =
{
	{ option_get, "-g" /*opt*/, false /*mandatory*/, true /*has_data*/ },
	{ option_verbose, "-v" /*opt*/, false /*mandatory*/, false /*has_data*/ },
};

int main(int argc, char *argv[])
{
	cmd_options_t options;
	if (!get_options(argc, argv, cmd_options, countof(cmd_options), options))
		return EXIT_FAILURE;

	if (get_option_exists(options, option_verbose))
		log_enable(log_error | log_warning | log_info);
	else
		log_enable(log_error);

	uv_default_loop();

	std::string url;
	if (get_option(options, option_get, url))
	{
		/* issue a GET request */
		http_get(url, 8080 /*port*/, [](const http_client_response_t &res) {
			for (auto &field : res.fields)
			{
				dlog(log_info, "%s: %s\n", field.key.c_str(),
					field.value.c_str());
			}
			dlog(log_info, "http_get callback called\n");
		});
	}
	else
	{
		http_use_route("/", HTTP_GET, [](const http_request_ptr_t &request, const http_response_ptr_t &response) {
			dlog(log_info, "received get against \"%s\"\n",
				request->uri_path().c_str());
			response->set_response(200, "OK", "text/html");
			response->send("<html>SUCCESS!</html>");
			response->end();
		});

		http_use_route("/objects", HTTP_GET, [](const http_request_ptr_t &request, const http_response_ptr_t &response) {
			dlog(log_info, "received get against \"%s\"\n",
				request->uri_path().c_str());
			response->set_response(200, "OK", "text/html");
			std::stringstream ss;
			ss << "<code>report<br/>test</code>";
			response->send(ss.str());
			response->end();
		});

		/* Consider consulting ulimit -aH as a signpost for what's a good backlog? */
		http_listen(8000 /*port*/, 6000 /*backlog*/);
	}
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	return EXIT_SUCCESS;
}

