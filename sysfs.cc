/*
 * sysfs.cc
 *
 *
 */

#include "sysfs.h"
#include "osutils.h"
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

static char *id = "@(#) $Id: sysfs.cc,v 1.2 2004/01/19 17:20:45 ezix Exp $";

#define SYS		"/sys"

static int selectdir(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  return S_ISDIR(buf.st_mode);
}

static string sysfs_getbustype(const string & path)
{
  struct dirent **namelist;
  int i, n;
  string devname;

/*
  to determine to which kind of bus a device is connected:
  - for each subdirectory of /sys/bus,
  - look in ./devices/ for a link with the same basename as 'path'
  - check if this link and 'path' point to the same inode
  - if they do, the bus type is the name of the current directory
 */
  pushd(SYS "/bus");
  n = scandir(".", &namelist, selectdir, alphasort);
  popd();

  for (i = 0; i < n; i++)
  {
    devname =
      string(SYS "/bus/") + string(namelist[i]->d_name) + "/devices/" +
      basename((char *) path.c_str());

    if (samefile(devname, path))
      return string(namelist[i]->d_name);
  }

  return "";
}

static string sysfstopci(const string & path)
{
  if (path.length() > 7)
    return path.substr(path.length() - 7);
  else
    return "";
}

static string sysfstobusinfo(const string & path)
{
  string bustype = sysfs_getbustype(path);

  if (bustype == "pci")
    return sysfstopci(path);

  return "";
}

string sysfs_getbusinfo(const string & devclass,
			const string & devname)
{
  string basename =
    string(SYS) + string("/class/") + devclass + string("/") + devname + "/";
  string device = basename + "/device";
  char buffer[PATH_MAX + 1];
  int namelen = 0;

  if ((namelen = readlink(device.c_str(), buffer, sizeof(buffer))) < 0)
    return "";

  device = basename + string(buffer, namelen);

  if (!realpath(device.c_str(), buffer))
    return "";

  return sysfstobusinfo(hw::strip(buffer));
}

bool scan_sysfs(hwNode & n)
{
  (void) &id;			// avoid warning "id defined but not used"

  return false;
}
