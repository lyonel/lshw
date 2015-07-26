#ifndef _OSUTILS_H_
#define _OSUTILS_H_

#include <string>
#include <vector>
#include <sys/types.h>
#include <stdint.h>

bool pushd(const std::string & dir = "");
std::string popd();
std::string pwd();

bool exists(const std::string & path);
bool samefile(const std::string & path1, const std::string & path2);
std::string readlink(const std::string & path);
std::string realpath(const std::string & path);
std::string dirname(const std::string & path);
bool loadfile(const std::string & file, std::vector < std::string > &lines);

size_t splitlines(const std::string & s,
std::vector < std::string > &lines,
char separator = '\n');
std::string get_string(const std::string & path, const std::string & def = "");
long get_number(const std::string & path, long def = 0);

std::string find_deventry(mode_t mode, dev_t device);
std::string get_devid(const std::string &);

std::string uppercase(const std::string &);
std::string lowercase(const std::string &);
std::string tostring(unsigned long long);
std::string tohex(unsigned long long);
std::string utf8(wchar_t);
std::string utf8_sanitize(const std::string &, bool autotruncate = true);
std::string utf8(uint16_t *s, ssize_t length = -1, bool forcelittleendian = false);

std::string spaces(unsigned int count, const std::string & space = " ");

std::string escape(const std::string &);
std::string escapeJSON(const std::string &);
std::string escapecomment(const std::string &);

std::string join(const std::string &, const std::string &, const std::string &);

std::string decimalkilos(unsigned long long value);
std::string kilobytes(unsigned long long value);

bool matches(const std::string & s, const std::string & pattern, int cflags=0);

int selectdir(const struct dirent *d);
int selectlink(const struct dirent *d);
int selectfile(const struct dirent *d);

unsigned short be_short(const void *);
unsigned short le_short(const void *);
unsigned long be_long(const void *);
unsigned long le_long(const void *);
unsigned long long be_longlong(const void *);
unsigned long long le_longlong(const void *);

int open_dev(dev_t dev, int dev_type, const std::string & name="");

std::string platform();
std::string operating_system();
#endif
