#include "osutils.h"
#include <stack>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

static stack < string > dirs;

bool pushd(const string & dir)
{
  string curdir = pwd();

  if (dir == "")
  {
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

static char *id = "@(#) $Id: osutils.cc,v 1.4 2003/01/30 08:04:25 ezix Exp $";
