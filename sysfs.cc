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

static char *id = "@(#) $Id: sysfs.cc,v 1.1 2004/01/19 16:15:15 ezix Exp $";

#define SYS		"/sys"

static string sysfs_getbustype(const string & path)
{
/*
  to determine to which kind of bus a device is connected:
  - for each subdirectory of /sys/bus,
  - look in ./devices/ for a link with the same basename as 'path'
  - check if this link and 'path' point to the same inode
  - if they do, the bus type is the name of the current directory
 */
  return "pci";
}

static string sysfstobusinfo(const string & path)
{
  string bustype = sysfs_getbustype(path);

  if (bustype == "pci")
  {
    if (path.length() > 7)
      return path.substr(path.length() - 7);
    else
      return "";
  }

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
