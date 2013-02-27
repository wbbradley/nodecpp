// REVIEWED
#pragma once
#include <string>

#define debug_ex(x)

#ifdef DEBUG
#define debug(x) x
#else
#define debug(x)
#endif

bool check_errno(const char *tag);
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

