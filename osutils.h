#ifndef _OSUTILS_H_
#define _OSUTILS_H_

#include <string>
#include <vector>
#include <sys/types.h>

bool pushd(const std::string & dir = "");
std::string popd();
std::string pwd();

bool exists(const std::string & path);
bool loadfile(const std::string & file, std::vector < std::string > &lines);

int splitlines(const std::string & s,
		std::vector < std::string > &lines,
		char separator = '\n');
std::string get_string(const std::string & path, const std::string & def = "");

std::string find_deventry(mode_t mode, dev_t device);

#endif
