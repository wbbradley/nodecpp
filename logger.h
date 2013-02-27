#pragma once
#include <string>
#include <stdio.h>
#include <iosfwd>
#include <ostream>
#include "logger_decls.h"

const char *logstr(log_level_t ll);
void time_now(std::ostream &os, bool exact, bool for_humans);
void append_time(std::ostream &os, double time_exact, bool exact, bool for_humans, const char sep = '\t');

// single-threaded logger is not safe to use in a multi-threaded app
class logger
{
public:
	logger(const std::string &name, const std::string &root_file_path);
	~logger();

	void logv(log_level_t level, const char *format, va_list args);
	void log(log_level_t level, const char *format, ...);
	void close();
	void open();
	void flush();
	void call_logging_function(void (*func)(FILE *));

	friend void log(log_level_t level, const char *format, ...);
	friend void log(void (*func)(FILE *));
	friend void log_flush();

private:
	static logger *s_plogger;
	std::string m_name;
	std::string m_root_file_path;
	std::string m_current_logfile;
	FILE *m_fp;
	int m_entries;
};

