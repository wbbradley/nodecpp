// REVIEWED
#include <assert.h>
#include "disk.h"
#include <stdio.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <copyfile.h>
#include <fstream>
#include "logger_decls.h"
#include "utils.h"
#include <algorithm>

bool file_exists(const std::string &file_path)
{
	if (file_path.size() == 0)
		return false;

	errno = 0;
	return access(file_path.c_str(), R_OK) != -1;
}

bool folder_exists(const std::string &path)
{
	struct stat stDirInfo;

	errno = 0;
	if (lstat(path.c_str(), &stDirInfo) < 0)
		return false;

	if (!S_ISDIR(stDirInfo.st_mode))
		return false;

	return true;
}

off_t file_size(const char *filename)
{
	struct stat st;

	if (stat(filename, &st) == 0)
		return st.st_size;

	return 0;
}

bool get_line_col(const std::string &file_path, size_t offset, size_t &line, size_t &col)
{
	FILE *fp = fopen(file_path.c_str(), "rt");
	if (fp != NULL)
	{
		char ch;
		line = 1;
		col = 1;
		for (size_t i = 0; i < offset; i++)
		{
			if (fread(&ch, 1, 1, fp) == 0)
				return false;
			switch (ch)
			{
			case '\n':
				line++;
				col = 1;
				continue;
			default:
				col++;
				continue;
			}
		}
		fclose(fp);
		return true;
	}
	return false;
}

bool list_files(const std::string &folder,
				const std::string &match,
				std::vector<std::string> &leaf_names)
{
	struct stat stDirInfo;
	struct dirent *stFiles;
	DIR *stDirIn;

	if (lstat(folder.c_str(), &stDirInfo) < 0)
	{
		dlog(log_info, "list_files : error : error in lstat of %s\n", folder.c_str());
		return false;
	}
	if (!S_ISDIR(stDirInfo.st_mode))
		return false;
	if ((stDirIn = opendir(folder.c_str())) == NULL)
	{
		dlog(log_info, "list_files : error : funky error #3 on %s\n", folder.c_str());
		return false;
	}
	while ((stFiles = readdir(stDirIn)) != NULL)
	{
		std::string leaf_name = stFiles->d_name;
		if (leaf_name == "."
				|| leaf_name == ".."
				|| (leaf_name.find(match) == std::string::npos))
		{
			continue;
		}
		leaf_names.push_back(leaf_name);
	}
	closedir(stDirIn);

	return true;
}

bool move_files(const std::string &source, const std::string &dest)
{
	if (!ensure_directory_exists(dest))
	{
		return false;
	}

	struct stat stDirInfo;
	struct dirent *stFiles;
	DIR *stDirIn;
	struct stat stFileInfo;

	if (lstat(source.c_str(), &stDirInfo) < 0)
	{
		dlog(log_info, "move_files : error : error in lstat of %s\n", source.c_str());
		return false;
	}
	if (!S_ISDIR(stDirInfo.st_mode))
		return false;
	if ((stDirIn = opendir(source.c_str())) == NULL)
	{
		dlog(log_info, "move_files : error : funky error #3 on %s\n", source.c_str());
		return false;
	}
	while ((stFiles = readdir(stDirIn)) != NULL)
	{
		std::string leaf_name = stFiles->d_name;
		if (leaf_name == "."
				|| leaf_name == "..")
		{
			continue;
		}

		std::string full_source_path = source + "/" + leaf_name;

		if (lstat(full_source_path.c_str(), &stFileInfo) < 0)
		{
			dlog(log_info, "move_files : error : funky error #1 on %s\n",
					full_source_path.c_str());
			continue;
		}

		std::string full_target_path = dest + "/" + leaf_name;
		dlog(log_info, "move_files : info : renaming %s to %s\n",
				full_source_path.c_str(), full_target_path.c_str());
		if (rename(full_source_path.c_str(), full_target_path.c_str()) != 0)
		{
			closedir(stDirIn);
			return false;
		}
		assert(!file_exists(full_source_path.c_str()));
		assert(file_exists(full_target_path.c_str()));
	}
	closedir(stDirIn);

	return true;
}

bool ensure_directory_exists(const std::string &name)
{
	errno = 0;
	mode_t mode = S_IRWXU;
	return ((mkdir(name.c_str(), mode) == 0) || (errno == EEXIST));
}

std::string directory_from_file_path(const std::string &file_path)
{
	size_t pos = file_path.find_last_of('/');
	if (pos == std::string::npos)
		return std::string();

	return file_path.substr(0, pos);
}

std::string leaf_from_file_path(const std::string &file_path)
{
	size_t pos = file_path.find_last_of('/');
	if (pos == std::string::npos)
		return std::string();

	return file_path.substr(pos + 1);
}

