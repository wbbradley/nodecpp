#include "http_request.h"

http_request_t::http_request_t(http_method method, const std::string &target_uri) : method(method), target_uri(target_uri)
{
}
