// REVIEWED
#pragma once
#include <string>
#include <sstream>
#include "logger_decls.h"

#define debug_ex(x)

#ifdef DEBUG
#define debug(x) x
#define assert_implies(x, y) do { if (x) assert(y); } while (0)
#else
#define debug(x)
#define assert_implies(x, y)
#endif

bool check_errno(const char *tag);
std::string ellipsis(const std::string &text, int max_len);
bool streamed_replace(const std::string &input_buffer_name, const char *pch_begin, const char * const pch_end, const std::string &before, const std::string &after, std::ostream &ofs, bool print_matches, bool pretty_print, bool do_replace);
double get_current_time();

inline bool mask(int grf, int grf_mask)
{
	return grf & grf_mask;
}

template <typename T>
inline size_t countof(const T &t)
{
	return t.size();
}

template <typename T, size_t N>
constexpr size_t countof(T (&array)[N])
{
	return N;
}

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define STATIC_COUNT_DEBUG

template <class T, unsigned int T_Max = 0, bool T_report = true>
class static_count
{
protected:
#ifdef STATIC_COUNT_DEBUG
	void report() const
	{
		if (T_report)
		{
			std::stringstream ss;
			const char *name = typeid(T).name();
			while (isdigit(*name))
				name++;
			ss << "static_count<" << name << "> : count is ";
		   	ss << s_objCount << "\n";
			log(log_info, ss.str().c_str());
		}
	}
#endif // DEBUG

	static_count() : instance_id(++s_instance_id_next)
	{
		++s_objCount;
#ifdef STATIC_COUNT_DEBUG
		report();
		if ((int)T_Max > 0 && s_objCount > T_Max)
		{
			std::stringstream ss;
			ss << __PRETTY_FUNCTION__ << " : unexpected : too many objects exist.";
			dlog(log_info, ss.str().c_str());
		}
#endif // DEBUG
	}
	~static_count()
	{
		--s_objCount;
#ifdef STATIC_COUNT_DEBUG
		report();
#endif // DEBUG
	}

public:
	static unsigned int class_instance_count()
	{
		return s_objCount;
	}

	unsigned int instance_id;

protected:
	static unsigned int s_objCount;
	static unsigned int s_instance_id_next;
};

template <class T, unsigned int T_Max, bool T_report>
unsigned int static_count<T, T_Max, T_report>::s_objCount = 0;

template <class T, unsigned int T_Max, bool T_report>
unsigned int static_count<T, T_Max, T_report>::s_instance_id_next = 0;

