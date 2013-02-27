// REVIEWED
#include <assert.h>
#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <iomanip>
#include <sstream>
#include "disk.h"
#include "utils.h"
#ifdef DEBUG
#include <execinfo.h>
#endif

const int max_log_entries_per_file = 100000;

int logger_level = log_info | log_warning | log_error;

void log_enable(int log_level)
{
	logger_level = log_level;
}

logger *logger::s_plogger = NULL;

logger::logger(const std::string &name, const std::string &root_file_path) : m_name(name), m_fp(NULL), m_entries(0)
{
	m_root_file_path = root_file_path;
	if (m_root_file_path[m_root_file_path.size() - 1] != '/')
			m_root_file_path.append("/");
	m_root_file_path.append("logs");
	if (!ensure_directory_exists(m_root_file_path))
	{
		fprintf(stderr, "logger : couldn't guarantee that directory %s exists\naborting...\n", m_root_file_path.c_str());
		exit(1);
	}
	if (s_plogger == NULL)
		s_plogger = this;
	else
		fprintf(stderr, "multiple loggers are loaded!");

	open();
}

void append_time(std::ostream &os, double time_exact, bool exact, bool for_humans, const char sep)
{
	time_t time = (time_t)time_exact;

	tm tdata;
	gmtime_r(&time, &tdata);
	os.setf(std::ios::fixed, std::ios::floatfield);
	os.fill('0'); // Pad on left with '0'
	if (for_humans)
	{
		os << std::setw(2) << tdata.tm_mon + 1 << '/'
			<< std::setw(2) << tdata.tm_mday << '/'
			<< std::setw(2) << tdata.tm_year + 1900 << sep
			<< std::setw(2) << tdata.tm_hour << ':'
			<< std::setw(2) << tdata.tm_min << ':'
			<< std::setw(2) << tdata.tm_sec;
	}
	else
	{
		os << std::setw(2) << tdata.tm_year + 1900
			<< std::setw(2) << tdata.tm_mon + 1
			<< std::setw(2) << tdata.tm_mday << 'T'
			<< std::setw(2) << tdata.tm_hour
			<< std::setw(2) << tdata.tm_min
			<< std::setw(2) << tdata.tm_sec;
		if (exact)
		{
			double decimals = (time_exact - (double)time);
			/* Turn it into milliseconds */
			decimals *= 1000;
			os << '.' << std::setw(3) << (int)decimals;
		}
	}
}

void time_now(std::ostream &os, bool exact, bool for_humans)
{
	double time_exact = get_current_time();
	append_time(os, time_exact, exact, for_humans);
}

void logger::open()
{
	m_entries = 0;
	assert(m_fp == NULL);
	if (m_root_file_path.size() > 0)
	{
		std::string logfile(m_root_file_path);
		if (logfile[logfile.size() - 1] != '/')
			logfile.append("/");

		std::stringstream ss;
		ss.setf(std::ios::fixed, std::ios::floatfield);
		ss.fill('0'); // Pad on left with '0'
		ss << m_name << "-";
		time_now(ss, false /*exact*/, false /*for_humans*/);
		ss << ".log";
		logfile.append(ss.str());
		m_current_logfile = logfile;
		m_fp = fopen(logfile.c_str(), "wb");
	}
}

const char *logstr(log_level_t ll)
{
	switch (ll)
	{
	case log_info:
		return "I\t";
	case log_warning:
		return "W\t";
	case log_error:
		return "E\t";
	default:
		return "X\t";
	}
}

void log(void (*func)(FILE *))
{
	if (logger::s_plogger != NULL)
		logger::s_plogger->call_logging_function(func);
}

void log(log_level_t level, const char *format, ...)
{
	if (mask(logger_level,level) == 0)
		return;

	if (logger::s_plogger == NULL)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		return;
	}

	va_list args;
	va_start(args, format);
	logger::s_plogger->logv(level, format, args);
	va_end(args);

#ifdef DEBUG
	if (level != log_direct)
	{
		fprintf(stderr, "%s", logstr(level));
	}
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
#endif

}

void log_flush()
{
	if (logger::s_plogger == NULL)
	{
		fflush(stderr);
		fflush(stdout);
	}
	else
	{
		logger::s_plogger->flush();
	}
}

void logger::flush()
{
	if (m_fp != NULL)
		fflush(m_fp);
}

void logger::call_logging_function(void (*func)(FILE *))
{
	if (m_fp != NULL)
		func(m_fp);
}

void logger::log(log_level_t level, const char *format, ...)
{
	if (mask(logger_level, level) == 0)
		return;

	va_list args;
	va_start(args, format);
	logv(level, format, args);
	va_end(args);
}

void logger::logv(log_level_t level, const char *format, va_list args)
{
	if (mask(logger_level, level) == 0)
		return;

	FILE *fp = m_fp;
	if (fp != NULL)
	{
		m_entries++;

		if (m_entries >= max_log_entries_per_file)
		{
			close();
			open();
			fp = m_fp;
		}
	}

	if (fp == NULL)
		fp = stderr;

	if (level == log_direct)
	{
		vfprintf(fp, format, args);
	}
	else
	{
		std::stringstream ss;
		time_now(ss, true /*exact*/, true /*for_humans*/);
		fprintf(fp, "%s\t%s", ss.str().c_str(), logstr(level));
		vfprintf(fp, format, args);
	}

#ifdef DEBUG
	if (level == log_error)
	{
		void *callstack[128];
		int frames = backtrace(callstack, 128);
		char **strs = backtrace_symbols(callstack, frames);
		for (int i = 0; i < frames; ++i)
		{
			fprintf(fp, "%s\n", strs[i]);
			if (fp != stderr)
				fprintf(stderr, "%s\n", strs[i]);

		}
		free(strs);
	}
#endif

	fflush(fp);
}

void logger::close()
{
	if (m_fp != NULL)
		fclose(m_fp);
	m_fp = NULL;
}

logger::~logger()
{
	close();
}

