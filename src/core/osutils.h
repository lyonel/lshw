#ifndef _OSUTILS_H_
#define _OSUTILS_H_

#include <string>
#include <vector>
#include <sys/types.h>

bool pushd(const std::string & dir = "");
std::string popd();
std::string pwd();

bool exists(const std::string & path);
bool samefile(const std::string & path1, const std::string & path2);
std::string resolvesymlinks(const std::string & path);
bool loadfile(const std::string & file, std::vector < std::string > &lines);

size_t splitlines(const std::string & s,
		std::vector < std::string > &lines,
		char separator = '\n');
std::string get_string(const std::string & path, const std::string & def = "");

std::string find_deventry(mode_t mode, dev_t device);
std::string get_devid(const std::string &);

std::string uppercase(const std::string &);
std::string lowercase(const std::string &);

std::string join(const std::string &, const std::string &, const std::string &);

#endif
