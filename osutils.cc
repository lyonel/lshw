#include "osutils.h"
#include <stack>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>

static char *id = "@(#) $Id: osutils.cc,v 1.10 2004/01/19 16:15:15 ezix Exp $";

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

int splitlines(const string & s,
	       vector < string > &lines,
	       char separator)
{
  size_t i = 0, j = 0;
  int count;

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

static int selectdir(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  return S_ISDIR(buf.st_mode);
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
  (void) &id;			// avoid warning "id defined but not used"
  return find_deventry("/dev", mode, device);
}

bool samefile(const string & path1,
	      const string & path2)
{
  struct stat stat1;
  struct stat stat2;

  if (stat(path1.c_str(), &stat1) != 0)
    return false;
  if (stat(path2.c_str(), &stat2) != 0)
    return false;

  return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}
