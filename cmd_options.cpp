// REVIEWED
#include "cmd_options.h"
#include <algorithm>
#include "logger_decls.h"
#include <string.h>
#include <vector>
#include "utils.h"

auto find_option(const char *arg, std::vector<cmd_option_t> &options) -> decltype(options.end())
{
	auto i = options.begin();
	for (; i != options.end(); ++i)
	{
		if (strcmp(arg, i->opt) == 0)
			return i;
	}
	return i;
}

void dump_options(const char *app, const cmd_option_t *options, size_t option_count)
{
	printf("usage:\n%s", app);
	for (size_t i = 0; i < option_count; i++)
	{
		if (options[i].mandatory)
			printf(" %s", options[i].opt);
		else
			printf(" [%s", options[i].opt);

		if (options[i].has_data)
			printf(" %s", options[i].codename);
		if (!options[i].mandatory)
			printf("]");
	}
	printf("\n");
}

bool get_options(
		int argc,
	   	char *argv[],
	   	const cmd_option_t *options_,
	   	size_t option_count,
	   	cmd_options_t &cmd_options)
{
	if (argc == 2 && strcmp(argv[1], "--help") == 0)
	{
		dump_options(argv[0], options_, option_count);
		return false;
	}
	std::vector<cmd_option_t> options;
	std::copy(options_, options_ + option_count, std::back_inserter(options));
	for (int i = 1; i < argc; ++i)
	{
		auto iter = find_option(argv[i], options);
		if (iter != options.end())
		{
			iter->mandatory = false;

			if (iter->has_data)
			{
				if (++i >= argc)
				{
					log(log_error, "get_options : missing option value for %s\n", iter->opt);
					dump_options(argv[0], options_, option_count);
					return false;
				}
				debug_ex(printf("setting option %s to %s\n", iter->codename, argv[i]));
				cmd_options[iter->codename] = argv[i];
			}
			else
			{
				debug_ex(printf("setting option %s to yes\n", iter->codename));
				cmd_options[iter->codename] = "yes";
			}
		}
	}
	for (auto option : options)
	{
		if (option.mandatory)
		{
			log(log_error, "get_options : missing option %s <%s>\n", option.opt, option.codename);
			dump_options(argv[0], options_, option_count);
			return false;
		}
	}
	return true;
}

bool get_option(const cmd_options_t &cmd_options, const std::string &codename, double &value)
{
	auto i = cmd_options.find(codename);
	if (i == cmd_options.end())
		return false;
	value = atof(i->second.c_str());
	return true;
}

bool get_option(const cmd_options_t &cmd_options, const std::string &codename, int32_t &value)
{
	auto i = cmd_options.find(codename);
	if (i == cmd_options.end())
		return false;
	value = atoi(i->second.c_str());
	return true;
}

bool option_exists(const cmd_options_t &cmd_options, const std::string &codename)
{
	auto i = cmd_options.find(codename);
	return (i != cmd_options.end());
}

bool get_option(const cmd_options_t &cmd_options, const std::string &codename, std::string &value)
{
	auto i = cmd_options.find(codename);
	if (i == cmd_options.end())
		return false;
	value = i->second;
	return true;
}

bool get_option_exists(const cmd_options_t &cmd_options, const std::string &codename)
{
	auto i = cmd_options.find(codename);
	return (i != cmd_options.end());
}

