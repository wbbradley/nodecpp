// REVIEWED
#pragma once
#include <stdio.h>

enum log_level_t
{
	log_direct = 8,
	log_info = 1,
	log_warning = 2,
	log_error = 4,
};

void log_enable(int log_level);
void log(log_level_t level, const char *format, ...);
void log(void (*func)(FILE *));
void log_flush();

#ifdef DEBUG
#define dlog(params...) log(params)
#else
#define dlog(params...)
#endif

