#include "version.h"
#include "osutils.h"
#include <sstream>
#include <iomanip>
#include <stack>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgen.h>
#include <regex.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <wchar.h>
#include <sys/utsname.h>
#ifndef MINOR
#include <linux/kdev_t.h>
#endif

__ID("@(#) $Id$");

using namespace std;

static stack < string > dirs;

bool pushd(const string & dir)
{
  string curdir = pwd();

  if (dir == "")
  {
    if (dirs.size() == 0)
      return true;

    if (chdir(dirs.top().c_str()) == 0)
    {
      dirs.pop();
      dirs.push(curdir);
      return true;
    }
    else
      return false;
  }

  if (chdir(dir.c_str()) == 0)
  {
    dirs.push(curdir);
    return true;
  }
  else
    return false;
}


string popd()
{
  string curdir = pwd();

  if (dirs.size() == 0)
    return curdir;

  if (chdir(dirs.top().c_str()) == 0)
    dirs.pop();

  return curdir;
}


string pwd()
{
  char curdir[PATH_MAX + 1];

  if (getcwd(curdir, sizeof(curdir)))
    return string(curdir);
  else
    return "";
}


size_t splitlines(const string & s,
vector < string > &lines,
char separator)
{
  size_t i = 0, j = 0;
  size_t count = 0;

  lines.clear();

  while ((j < s.length()) && ((i = s.find(separator, j)) != string::npos))
  {
    lines.push_back(s.substr(j, i - j));
    count++;
    i++;
    j = i;
  }
  if (j < s.length())
  {
    lines.push_back(s.substr(j));
    count++;
  }

  return count;
}


bool exists(const string & path)
{
  return access(path.c_str(), F_OK) == 0;
}


bool loadfile(const string & file,
vector < string > &list)
{
  char buffer[1024];
  string buffer_str = "";
  size_t count = 0;
  int fd = open(file.c_str(), O_RDONLY);

  if (fd < 0)
    return false;

  while ((count = read(fd, buffer, sizeof(buffer))) > 0)
    buffer_str += string(buffer, count);

  splitlines(buffer_str, list);

  close(fd);

  return true;
}


string get_string(const string & path,
const string & def)
{
  int fd = open(path.c_str(), O_RDONLY);
  string result = def;

  if (fd >= 0)
  {
    char buffer[1024];
    size_t count = 0;

    memset(buffer, 0, sizeof(buffer));
    result = "";

    while ((count = read(fd, buffer, sizeof(buffer))) > 0)
      result += string(buffer, count);

    close(fd);
  }

  return result;
}

long get_number(const string & path, long def)
{
  string s = get_string(path, "");

  if(s=="") return def;

  return strtol(s.c_str(), NULL, 10);
}

int selectdir(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  return S_ISDIR(buf.st_mode);
}


int selectlink(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  return S_ISLNK(buf.st_mode);
}

int selectfile(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  return S_ISREG(buf.st_mode);
}

static int selectdevice(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  return S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode);
}


static bool matches(string name,
mode_t mode,
dev_t device)
{
  struct stat buf;

  if (lstat(name.c_str(), &buf) != 0)
    return false;

  return ((S_ISCHR(buf.st_mode) && S_ISCHR(mode)) ||
    (S_ISBLK(buf.st_mode) && S_ISBLK(mode))) && (buf.st_dev == device);
}


static string find_deventry(string basepath,
mode_t mode,
dev_t device)
{
  struct dirent **namelist;
  int n, i;
  string result = "";

  pushd(basepath);

  n = scandir(".", &namelist, selectdevice, alphasort);

  if (n < 0)
  {
    popd();
    return "";
  }

  for (i = 0; i < n; i++)
  {
    if (result == "" && matches(namelist[i]->d_name, mode, device))
      result = string(namelist[i]->d_name);
    free(namelist[i]);
  }
  free(namelist);

  popd();

  if (result != "")
    return basepath + "/" + result;

  pushd(basepath);
  n = scandir(".", &namelist, selectdir, alphasort);
  popd();

  if (n < 0)
    return "";

  for (i = 0; i < n; i++)
  {
    if (result == "")
      result =
        find_deventry(basepath + "/" + string(namelist[i]->d_name), mode,
        device);
    free(namelist[i]);
  }
  free(namelist);

  return result;
}


string find_deventry(mode_t mode,
dev_t device)
{
  return find_deventry("/dev", mode, device);
}


string get_devid(const string & name)
{
  struct stat buf;

  if((stat(name.c_str(), &buf)==0) && (S_ISBLK(buf.st_mode) || S_ISCHR(buf.st_mode)))
  {
    char devid[80];

    snprintf(devid, sizeof(devid), "%u:%u", (unsigned int)MAJOR(buf.st_rdev), (unsigned int)MINOR(buf.st_rdev));
    return string(devid);
  }
  else
    return "";
}


bool samefile(const string & path1, const string & path2)
{
  struct stat stat1;
  struct stat stat2;

  if (stat(path1.c_str(), &stat1) != 0)
    return false;
  if (stat(path2.c_str(), &stat2) != 0)
    return false;

  return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}


string uppercase(const string & s)
{
  string result;

  for (unsigned int i = 0; i < s.length(); i++)
    result += toupper(s[i]);

  return result;
}


string lowercase(const string & s)
{
  string result;

  for (unsigned int i = 0; i < s.length(); i++)
    result += tolower(s[i]);

  return result;
}


string tostring(unsigned long long n)
{
  char buffer[80];

  snprintf(buffer, sizeof(buffer), "%lld", n);

  return string(buffer);
}


string tohex(unsigned long long n)
{
  char buffer[80];

  snprintf(buffer, sizeof(buffer), "%llX", n);

  return string(buffer);
}

string join(const string & j, const string & s1, const string & s2)
{
  if(s1 == "") return s2;
  if(s2 == "") return s1;

  return s1 + j + s2;
}


bool matches(const string & s, const string & pattern, int cflags)
{
  regex_t r;
  bool result = false;

  if(regcomp(&r, pattern.c_str(), REG_EXTENDED | REG_NOSUB | cflags) != 0)
    return false;

  result = (regexec(&r, s.c_str(), 0, NULL, 0) == 0);

  regfree(&r);

  return result;
}


string readlink(const string & path)
{
  char buffer[PATH_MAX+1];

  memset(buffer, 0, sizeof(buffer));
  if(readlink(path.c_str(), buffer, sizeof(buffer)-1)>0)
    return string(buffer);
  else
    return path;
}


string realpath(const string & path)
{
  char buffer[PATH_MAX+1];

  memset(buffer, 0, sizeof(buffer));
  if(realpath(path.c_str(), buffer))
    return string(buffer);
  else
    return path;
}


string dirname(const string & path)
{
  size_t len = path.length();
  char *buffer = new char[len + 1];
  path.copy(buffer, len);
  buffer[len] = '\0';
  string result = dirname(buffer);
  delete buffer;
  return result;
}


string spaces(unsigned int count, const string & space)
{
  string result = "";
  while (count-- > 0)
    result += space;

  return result;
}

string escape(const string & s)
{
  string result = "";

  for (unsigned int i = 0; i < s.length(); i++)
    // #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
    if (s[i] == 0x9
        || s[i] == 0xA
        || s[i] == 0xD
        || s[i] >= 0x20)
      switch (s[i])
      {
        case '<':
          result += "&lt;";
          break;
        case '>':
          result += "&gt;";
          break;
        case '&':
          result += "&amp;";
          break;
        case '"':
          result += "&quot;";
          break;
      default:
        result += s[i];
    }

  return result;
}

string escapeJSON(const string & s)
{
  string result = "";

  for (unsigned int i = 0; i < s.length(); i++)
    switch (s[i])
    {
      case '\r':
        result += "\\r";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\t':
        result += "\\t";
        break;
      case '"':
        result += "\\\"";
        break;
    default:
      result += s[i];
  }

  return result;
}

string escapecomment(const string & s)
{
  string result = "";
  char previous = 0;

  for (unsigned int i = 0; i < s.length(); i++)
    if(!(previous == '-' && s[i] == '-'))
    {
      result += s[i];
      previous = s[i];
    }

  return result;
}

unsigned short be_short(const void * from)
{
  uint8_t *p = (uint8_t*)from;

  return ((uint16_t)(p[0]) << 8) +
    (uint16_t)p[1];
}


unsigned short le_short(const void * from)
{
  uint8_t *p = (uint8_t*)from;

  return ((uint16_t)(p[1]) << 8) +
    (uint16_t)p[0];
}


unsigned long be_long(const void * from)
{
  uint8_t *p = (uint8_t*)from;

  return ((uint32_t)(p[0]) << 24) +
    ((uint32_t)(p[1]) << 16) +
    ((uint32_t)(p[2]) << 8) +
    (uint32_t)p[3];
}


unsigned long le_long(const void * from)
{
  uint8_t *p = (uint8_t*)from;

  return ((uint32_t)(p[3]) << 24) +
    ((uint32_t)(p[2]) << 16) +
    ((uint32_t)(p[1]) << 8) +
    (uint32_t)p[0];

}


unsigned long long be_longlong(const void * from)
{
  uint8_t *p = (uint8_t*)from;

  return ((unsigned long long)(p[0]) << 56) +
    ((unsigned long long)(p[1]) << 48) +
    ((unsigned long long)(p[2]) << 40) +
    ((unsigned long long)(p[3]) << 32) +
    ((unsigned long long)(p[4]) << 24) +
    ((unsigned long long)(p[5]) << 16) +
    ((unsigned long long)(p[6]) << 8) +
    (unsigned long long)p[7];
}


unsigned long long le_longlong(const void * from)
{
  uint8_t *p = (uint8_t*)from;

  return ((unsigned long long)(p[7]) << 56) +
    ((unsigned long long)(p[6]) << 48) +
    ((unsigned long long)(p[5]) << 40) +
    ((unsigned long long)(p[4]) << 32) +
    ((unsigned long long)(p[3]) << 24) +
    ((unsigned long long)(p[2]) << 16) +
    ((unsigned long long)(p[1]) << 8) +
    (unsigned long long)p[0];
}


int open_dev(dev_t dev, int dev_type, const string & name)
{
  static const char *paths[] =
  {
    "/usr/tmp", "/var/tmp", "/var/run", "/dev", "/tmp", NULL
  };
  char const **p;
  char fn[64];
  int fd;

  for (p = paths; *p; p++)
  {
    if(name=="")
      snprintf(fn, sizeof(fn), "%s/lshw-%d", *p, getpid());
    else
      snprintf(fn, sizeof(fn), "%s", name.c_str());
    if ((mknod(fn, (dev_type | S_IREAD), dev) == 0) || (errno == EEXIST))
    {
      fd = open(fn, O_RDONLY);
      if(name=="") unlink(fn);
      if (fd >= 0)
        return fd;
    }
  }
  return -1;
}                                                 /* open_dev */

#define putchar(c) ((char)((c) & 0xff))

string utf8(wchar_t c)
{
  string result = "";

  if (c < 0x80)
  {
    result += putchar (c);
  }
  else if (c < 0x800)
  {
    result += putchar (0xC0 | c>>6);
    result += putchar (0x80 | (c & 0x3F));
  }
  else if (c < 0x10000)
  {
    result += putchar (0xE0 | c>>12);
    result += putchar (0x80 | (c>>6 & 0x3F));
    result += putchar (0x80 | (c & 0x3F));
  }
  else if (c < 0x200000)
  {
    result += putchar (0xF0 | c>>18);
    result += putchar (0x80 | (c>>12 & 0x3F));
    result += putchar (0x80 | (c>>6 & 0x3F));
    result += putchar (0x80 | (c & 0x3F));
  }

  return result;
}

string utf8(uint16_t * s, ssize_t length, bool forcelittleendian)
{
  string result = "";
  ssize_t i;

  for(i=0; (length<0) || (i<length); i++)
    if(s[i])
      result += utf8(forcelittleendian?le_short(s+i):s[i]);
    else
      break;	// NUL found

  return result;
}

// U+FFFD replacement character
#define REPLACEMENT  "\357\277\275"

string utf8_sanitize(const string & s, bool autotruncate)
{
  unsigned int i = 0;
  unsigned int remaining = 0;
  string result = "";
  string emit = "";
  unsigned char c = 0;

  while(i<s.length())
  {
    c = s[i];
    switch(remaining)
    {
      case 3:
      case 2:
      case 1:
        if((0x80<=c) && (c<=0xbf))
        {
          emit += s[i];
          remaining--;
        }
        else		// invalid sequence (truncated)
        {
          if(autotruncate) return result;
          emit = REPLACEMENT;
          emit += s[i];
          remaining = 0;
        }
        break;

      case 0:
        result += emit;
        emit = "";

        if(c<=0x7f)
          emit = s[i];
        else
        if((0xc2<=c) && (c<=0xdf))	// start 2-byte sequence
        {
          remaining = 1;
          emit = s[i];
        }
        else
        if((0xe0<=c) && (c<=0xef))	// start 3-byte sequence
        {
          remaining = 2;
          emit = s[i];
        }
        else
        if((0xf0<=c) && (c<=0xf4))	// start 4-byte sequence
        {
          remaining = 3;
          emit = s[i];
        }
        else
        {
          if(autotruncate) return result;
          emit = REPLACEMENT;	// invalid character
        }

        break;
    }

    i++;
  }

  if(remaining == 0)
    result += emit;

  return result;
}

string decimalkilos(unsigned long long value)
{
  const char *prefixes = "KMGTPEZY";
  unsigned int i = 0;
  ostringstream out;

  while ((i <= strlen(prefixes)) && ((value > 10000) || (value % 1000 == 0)))
  {
    value = value / 1000;
    i++;
  }

  out << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    out << prefixes[i - 1];

  return out.str();
}


string kilobytes(unsigned long long value)
{
  const char *prefixes = "KMGTPEZY";
  unsigned int i = 0;
  ostringstream out;

  while ((i <= strlen(prefixes)) && ((value > 10240) || (value % 1024 == 0)))
  {
    value = value >> 10;
    i++;
  }

  out << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    out << prefixes[i - 1];
  out << "iB";

  return out.str();
}

string operating_system()
{
  vector<string> osinfo;
  struct utsname u;
  string os = "";

  if(loadfile("/etc/lsb-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/lsb_release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/system-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/arch-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/arklinux-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/aurox-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/conectiva-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/debian_version", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/fedora-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/gentoo-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/linuxppc-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/mandrake-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/mandriva-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/novell-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/pld-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/redhat-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/slackware-version", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/sun-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/SuSE-release", osinfo))
    os = osinfo[0];
  else if(loadfile("/etc/yellowdog-release", osinfo))
    os = osinfo[0];

  if(uname(&u) != 0) return "";

  os += (os == ""?"":" ; ") + string(u.sysname)+" "+string(u.release);

#if defined(__GLIBC__) && defined(_CS_GNU_LIBC_VERSION)
  char version[PATH_MAX];

      if(confstr(_CS_GNU_LIBC_VERSION, version, sizeof(version))>0)
        os += " ; "+string(version);
#endif

  return os;
}

string platform()
{
  string p = "";
  struct utsname u;

#ifdef __i386__
  p = "i386";
#endif

  if(uname(&u) != 0)
    return p;
  else
    return p + (p!=""?"/":"") + string(u.machine);
}
