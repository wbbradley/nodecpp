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
#include "http_fetch_op.h"

const char *option_nodecpp = "nodecpp";
const char *option_verbose = "verbose";


cmd_option_t cmd_options[] =
{
	{ option_nodecpp, "-j" /*opt*/, true /*mandatory*/, true /*has_data*/ },
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

	std::string nodecpp;
	get_option(options, option_nodecpp, nodecpp);

	http_get(nodecpp, 80, [=](const http_response_t &res) {
			dlog(log_info, "http_get callback called\n");
	});

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	return EXIT_SUCCESS;
}

