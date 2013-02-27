#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "utils.h"
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <iomanip>
#include "logger_decls.h"

#define case_error(error) case error: error_string = #error; break

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

bool check_errno(const char *tag)
{
	int err = errno;
	if (err == 0)
		return true;
	const char *error_string = "unknown";
	switch (err)
	{
		case_error(EACCES);
		case_error(EAFNOSUPPORT);
		case_error(EISCONN);
		case_error(EMFILE);
		case_error(ENFILE);
		case_error(ENOBUFS);
		case_error(ENOMEM);
		case_error(EPROTO);
		case_error(EHOSTDOWN);
		case_error(EHOSTUNREACH);
		case_error(ENETUNREACH);
		case_error(EPROTONOSUPPORT);
		case_error(EPROTOTYPE);
		case_error(EDQUOT);
		case_error(EAGAIN);
		case_error(EBADF);
		case_error(ECONNRESET);
		case_error(EFAULT);
		case_error(EINTR);
		case_error(EINVAL);
		case_error(ENETDOWN);
		case_error(ENOTCONN);
		case_error(ENOTSOCK);
		case_error(EOPNOTSUPP);
		case_error(ETIMEDOUT);
		case_error(EMSGSIZE);
		case_error(ECONNREFUSED);
	};
	if (err == -1)
		err = errno;

	log(log_info, "check_errno : %s %s %s\n", tag, error_string, strerror(err));

	return false;
}

void advance_line_count(
		const char *pch_dest,
		int &line,
		int &char_offset,
		const char *&pch_last_line_break)
{
	char_offset = 1;
	const char *pch = pch_last_line_break;
	assert(pch < pch_dest);
	while (++pch != pch_dest)
	{
		if (*pch == '\n')
		{
			pch_last_line_break = pch;
			char_offset = 1;
			++line;
		}
		else
		{
			++char_offset;
		}
	}
}

void advance_line_count(
		const uint16_t *pch_dest,
		int &line,
		int &char_offset,
		const uint16_t *&pch_last_line_break)
{
	char_offset = 1;
	const uint16_t *pch = pch_last_line_break;
	assert(pch < pch_dest);
	while (++pch != pch_dest)
	{
		if (*pch == '\n')
		{
			pch_last_line_break = pch;
			char_offset = 1;
			++line;
		}
		else
		{
			++char_offset;
		}
	}
}

void advance_line_count_be(
		const uint16_t *pch_dest,
		int &line,
		int &char_offset,
		const uint16_t *&pch_last_line_break)
{
	char_offset = 1;
	const uint16_t *pch = pch_last_line_break;
	assert(pch < pch_dest);
	while (++pch != pch_dest)
	{
		if (*pch == (int('\n') << 8))
		{
			pch_last_line_break = pch;
			char_offset = 1;
			++line;
		}
		else
		{
			++char_offset;
		}
	}
}

bool contains_binary(const std::string &text)
{
	for (auto ch : text)
	{
		if ((!iswspace(ch) && (ch < 32)) || (ch > 127))
			return true;
	}
	return false;
}

void print_match_line(
		const std::string &input_buffer_name,
		int line,
		int char_offset,
		bool pretty_print,
		const uint16_t *wch_last_line_break,
		const uint16_t *wch_next,
		const char *pch_end,
		int run_length)
{
	// TODO implement unicode output
	printf("%s%s%s:%d:%d: <pattern found in unicode file>\n",
			pretty_print ? KRED : "",
			input_buffer_name.c_str(),
			pretty_print ? KNRM : "",
			line,
			char_offset);
}

void print_match_line(
		const std::string &input_buffer_name,
		int line,
		int char_offset,
		bool pretty_print,
		const char *pch_last_line_break,
		const char *pch_next,
		const char *pch_end,
		int run_length)
{
	const char *pch_end_of_line = pch_next;
	while (true)
	{
		if (pch_end_of_line == pch_end)
			break;
		if (*pch_end_of_line == '\r' || *pch_end_of_line == '\n')
			break;
		++pch_end_of_line;
	}
	auto prefix = std::string(pch_last_line_break + 1, pch_next - pch_last_line_break - 1);
	auto text = std::string(pch_next, run_length);
	auto suffix = std::string(pch_next + run_length, (pch_end_of_line - pch_next) - run_length);

	if (contains_binary(prefix) || contains_binary(suffix))
	{
		printf("%s%s%s:%d:%d: <line matched but contained seemingly binary data>\n",
				pretty_print ? KRED : "",
				input_buffer_name.c_str(),
				pretty_print ? KNRM : "",
				line,
				char_offset);
	}
	else
	{
		printf("%s%s%s:%d:%d: %s%s%s%s%s\n",
				pretty_print ? KRED : "",
				input_buffer_name.c_str(),
				pretty_print ? KNRM : "",
				line,
				char_offset,
				prefix.c_str(),
				pretty_print ? KBLU : "",
				text.c_str(),
				pretty_print ? KNRM : "",
				suffix.c_str());
	}
}

bool odd_pointer(const void *p)
{
	static_assert(sizeof(unsigned long) == sizeof(void *), "find a way to check this");
	if (reinterpret_cast<unsigned long>(p) & 1)
	{
		debug_ex(dlog(log_info, "odd pointer is 0x%8lx\n", (unsigned long)p));
		return true;
	}

	return false;
}

bool streamed_replace(
		const std::string &input_buffer_name,
		const char *pch_begin,
	   	const char * const pch_end,
	   	const std::string &before,
	   	const std::string &after,
	   	std::ostream &ofs,
		bool print_matches,
		bool pretty_print,
		bool do_replace)
{
	assert(odd_pointer((void *)0x1));
	assert(!odd_pointer((void *)0x1004));

	const char *pch = pch_begin;
	bool found_before_target = false;

	// TODO get rid of this poor man's ascii -> unicode to enable non-latin code pages, etc...
	static_assert(sizeof(int16_t) == 2, "doh!");
	std::basic_string<int16_t> wbefore, wafter;
	std::basic_string<int16_t> wbefore_be, wafter_be;
	std::copy(before.begin(), before.end(), std::back_inserter(wbefore));
	std::copy(after.begin(), after.end(), std::back_inserter(wafter));
	wbefore_be = wbefore;
	wafter_be = wafter;
	for (auto &wch : wbefore_be)
		std::swap(*(char *)&wch, *(((char *)&wch) + 1));
	for (auto &wch : wafter_be)
		std::swap(*(char *)&wch, *(((char *)&wch) + 1));

	int line = 1;
	int char_offset = 1;
	const char *pch_last_line_break = pch_begin - 1;
	const uint16_t *wch_last_line_break = ((const uint16_t *)pch_begin) - 1;

	std::vector<const char *> nexts;
	assert(((char *)wbefore.c_str())[0] == 0 || ((char *)wbefore.c_str())[1] == 0);
	while (pch < pch_end)
	{
		const uint16_t *wch_next = (const uint16_t *)memmem(pch, pch_end - pch, &wbefore[0], wbefore.size() * sizeof(wbefore[0]));
		const uint16_t *wch_next_be = (const uint16_t *)memmem(pch, pch_end - pch, &wbefore_be[0], wbefore_be.size() * sizeof(wbefore[0]));
		const char *pch_next = (const char *)memmem(pch, pch_end - pch, before.c_str(), before.size());

		if (odd_pointer(wch_next))
		{
			dlog(log_info, "clearing wch_next in file %s\n",
					input_buffer_name.c_str());
			wch_next = nullptr;
		}

		if (odd_pointer(wch_next_be))
		{
			dlog(log_info, "clearing wch_next_be in file %s\n",
					input_buffer_name.c_str());
			wch_next_be = nullptr;
		}
		nexts.resize(0);

		if (wch_next != nullptr)
			nexts.push_back((const char *)wch_next);
		if (wch_next_be != nullptr)
			nexts.push_back((const char *)wch_next_be);
		if (pch_next != nullptr)
			nexts.push_back(pch_next);

		std::sort(nexts.begin(), nexts.end());
		if (nexts.size() != 0)
		{
			found_before_target = true;
			if (nexts[0] == (const char *)wch_next)
			{
				advance_line_count(wch_next, line, char_offset, wch_last_line_break);

				if (print_matches)
				{
					print_match_line(input_buffer_name, line, char_offset,
							pretty_print, wch_last_line_break, wch_next,
							pch_end, before.size());
				}
				if (do_replace)
				{
					ofs.write(pch, ((const char *)(wch_next)) - pch);
					ofs.write((const char *)wafter.c_str(), wafter.size() * sizeof(wafter[0]));
				}
				pch = ((const char *)wch_next) + (wbefore.size() * sizeof(wbefore[0]));
			}
			if (nexts[0] == (const char *)wch_next_be)
			{
				advance_line_count_be(wch_next_be, line, char_offset, wch_last_line_break);

				if (print_matches)
				{
					print_match_line(input_buffer_name, line, char_offset,
							pretty_print, wch_last_line_break, wch_next_be,
							pch_end, before.size());
				}
				if (do_replace)
				{
					ofs.write(pch, ((const char *)(wch_next_be)) - pch);
					ofs.write((const char *)wafter_be.c_str(), wafter_be.size() * sizeof(wafter_be[0]));
				}
				pch = ((const char *)wch_next_be) + (wbefore_be.size() * sizeof(wbefore_be[0]));
			}
			else if (nexts[0] == pch_next)
			{
				advance_line_count(pch_next, line, char_offset, pch_last_line_break);

				if (print_matches)
				{
					print_match_line(input_buffer_name, line, char_offset,
							pretty_print, pch_last_line_break, pch_next,
							pch_end, before.size());
				}
				if (do_replace)
				{
					ofs.write(pch, pch_next - pch);
					ofs.write(after.c_str(), after.size());
				}
				pch = pch_next + before.size();
			}
		}
		else
		{
			if (do_replace)
				ofs.write(pch, pch_end - pch);
			pch = pch_end;
		}
	}
	return found_before_target;
}

double get_current_time()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	double time_now = tv.tv_sec + (double(tv.tv_usec) / 1000000.0);
	return time_now;
}

