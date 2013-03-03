#include "nodecpp_errors.h"
#include "logger_decls.h"
#include <uv.h>
#include "utils.h"

void log_uv_errors()
{
	auto uv_error = uv_last_error(uv_default_loop());
	if (uv_error.code != UV_OK)
	{
		log(log_error, "uv errored with %d (see UV_ERRNO_MAP)\n", uv_error);
	}
	else
	{
		/* no errors */
	}
}

void log_http_errors(http_parser *parser)
{
	enum http_errno err = HTTP_PARSER_ERRNO(parser);
	if (err != HPE_OK)
	{
		log(log_error, "http_parser error code = %s", http_errno_name(err));
	}
	else
	{
		/* no errors */
	}
}
