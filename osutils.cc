#include "osutils.h"
#include <stack>
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

static char *id = "@(#) $Id: osutils.cc,v 1.2 2003/01/25 10:00:30 ezix Exp $";
