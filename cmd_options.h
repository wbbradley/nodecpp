// REVIEWED
#pragma once
#include "unordered.h"
#include <string>
#include <stdint.h>

typedef unordered_map<std::string, std::string> cmd_options_t;

struct cmd_option_t
{
	const char *codename;
	const char *opt;
	bool mandatory;
	bool has_data;
};
bool get_options(int argc, char *argv[], const cmd_option_t *options, size_t option_count, cmd_options_t &cmd_options);

bool get_option(const cmd_options_t &cmd_options, const std::string &codename, double &value);
bool get_option(const cmd_options_t &cmd_options, const std::string &codename, int32_t &value);
bool get_option(const cmd_options_t &cmd_options, const std::string &codename, bool &value);
bool get_option(const cmd_options_t &cmd_options, const std::string &codename, std::string &value);
bool get_option_exists(const cmd_options_t &cmd_options, const std::string &codename);
