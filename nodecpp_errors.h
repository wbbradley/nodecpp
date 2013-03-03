#pragma once
#include "http_parser.h"

void log_uv_errors();
void log_http_errors(http_parser *parser);
