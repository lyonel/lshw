#include "cpuinfo.h"
#include "osutils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <vector>

#define PROC_IDE "/proc/ide"

static int selectdir(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  return S_ISDIR(buf.st_mode);
}

bool scan_ide(hwNode & n)
{
  struct dirent **namelist;
  int nentries;

  pushd(PROC_IDE);
  nentries = scandir(".", &namelist, selectdir, NULL);
  popd();

  if (nentries < 0)
    return false;

  for (int i = 0; i < nentries; i++)
  {
    vector < string > config;
    hwNode ide("ide",
	       hw::storage);

    if (loadfile
	(string(PROC_IDE) + "/" + namelist[i]->d_name + "/config", config))
    {
      vector < string > identify;

      splitlines(config[0], identify, ' ');
      config.clear();

    }
  }

  return false;
}

static char *id =
  "@(#) $Id: ide.cc,v 1.1 2003/02/03 22:51:00 ezix Exp $";
