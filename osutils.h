#ifndef _OSUTILS_H_
#define _OSUTILS_H_

#include <string>
#include <vector>

bool pushd(const std::string & dir = "");
std::string popd();
std::string pwd();

bool exists(const string & path);
bool loadfile(const std::string & file, std::vector < std::string > &lines);

int splitlines(const std::string & s,
		std::vector < std::string > &lines,
		char separator = '\n');
std::string get_string(const std::string & path, const std::string & def = "");

#endif
