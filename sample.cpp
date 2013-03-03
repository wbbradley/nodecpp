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

	http_use_route("/", HTTP_GET, [](const http_request_t &req, http_respond_t &respond) {
		dlog(log_info, "received get against \"%s\"\n", req.target_uri.c_str());
	});

	http_listen(80 /*port*/, 1000 /*backlog*/);

	std::string url;
	if (get_option(options, option_get, url))
	{
		/* issue a GET request */
		http_get(url, 80 /*port*/, [](const http_response_t &res) {
			for (auto &field : res.fields)
			{
				dlog(log_info, "%s: %s\n", field.key.c_str(),
					field.value.c_str());
			}
			dlog(log_info, "http_get callback called\n");
		});
	}

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	return EXIT_SUCCESS;
}

