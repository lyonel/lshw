#ifndef _OSUTILS_H_
#define _OSUTILS_H_

#include <string>

bool pushd(const std::string & dir = "");
std::string popd();
std::string pwd();

#endif
