#include "http_request.h"
#include "logger_decls.h"
#include "nodecpp_errors.h"
#include "utils.h"

#ifdef DEBUG_EX
void dump_url(const char *url, const struct http_parser_url *u)
{
	unsigned int i;

	printf("\tfield_set: 0x%x, port: %u\n", u->field_set, u->port);
	for (i = 0; i < UF_MAX; i++)
   	{
		if ((u->field_set & (1 << i)) == 0)
	   	{
			printf("\tfield_data[%u]: unset\n", i);
			continue;
		}

		printf("\tfield_data[%u]: off: %u len: %u part: \"%.*s\n",
				i,
				u->field_data[i].off,
				u->field_data[i].len,
				u->field_data[i].len,
				url + u->field_data[i].off);
	}
}
#endif

http_request_t::http_request_t(http_method method, const std::string &target_uri)
	: method(method), target_uri(target_uri)
{
	memset(&parsed_url, 0, sizeof(parsed_url));
	if (http_parser_parse_url(target_uri.c_str(), target_uri.size(),
			   	false /*is_connect*/, &parsed_url))
	{
		dlog(log_error, "error parsing target-uri \"%s\"\n", target_uri.c_str());
	}
	else
	{
		debug_ex(dump_url(target_uri.c_str(), &parsed_url));
	}
}

std::string http_request_t::uri_path() const
{
	dlog_assert(parsed_url.field_set & (1 << UF_PATH), "invalid target-uri : no path\n");
	return std::string(target_uri.c_str() + parsed_url.field_data[UF_PATH].off, parsed_url.field_data[UF_PATH].len);
}

bool http_request_t::keep_alive() const
{
   	return parser.http_major == 1 && parser.http_minor == 1;
}

