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
#include <stdio.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>

static char *id = "@(#) $Id: sysfs.cc,v 1.4 2004/04/14 20:04:44 ezix Exp $";

struct sysfs_t
{
  sysfs_t():path("/sys"),
  temporary(false),
  has_sysfs(false)
  {
    has_sysfs = exists(path + "/classes/.");

    if (!has_sysfs)		// sysfs doesn't seem to be mounted
      // try to mount it in a temporary directory
    {
      char buffer[50];
      char *tmpdir = NULL;

        strncpy(buffer,
		"/var/tmp/sys-XXXXXX",
		sizeof(buffer));
        tmpdir = mkdtemp(buffer);

      if (tmpdir)
      {
	temporary = true;
	path = string(tmpdir);
	chmod(tmpdir,
	      0000);		// to make clear it is a mount point
	mount("none",
	      path.c_str(),
	      "sysfs",
	      0,
	      NULL);
      }

      has_sysfs = exists(path + "/classes/.");
    }
  }

  ~sysfs_t()
  {
    if (temporary)
    {
      umount(path.c_str());
      rmdir(path.c_str());
    }
  }

  string path;
  bool temporary;
  bool has_sysfs;
};

static sysfs_t sysfs;

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
  pushd(sysfs.path + "/bus");
  n = scandir(".", &namelist, selectdir, alphasort);
  popd();

  for (i = 0; i < n; i++)
  {
    devname =
      string(sysfs.path + "/bus/") + string(namelist[i]->d_name) +
      "/devices/" + basename((char *) path.c_str());

    if (samefile(devname, path))
      return string(namelist[i]->d_name);
  }

  return "";
}

static string sysfstopci(const string & path)
{
  if (path.length() > 7)
    return "pci@" + path.substr(path.length() - 7);
  else
    return "";
}

static string sysfstoide(const string & path)
{
  if (path.length() > 3)
    return "ide@" + path.substr(path.length() - 3);
  else
    return "";
}

static string sysfstobusinfo(const string & path)
{
  string bustype = sysfs_getbustype(path);

  if (bustype == "pci")
    return sysfstopci(path);

  if (bustype == "ide")
    return sysfstoide(path);

  return "";
}

string sysfs_getbusinfo(const string & devclass,
			const string & devname)
{
  string basename =
    sysfs.path + string("/class/") + devclass + string("/") + devname + "/";
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

string sysfs_getdriver(const string & devclass,
		       const string & devname)
{
  string driverpath =
    sysfs.path + string("/class/") + devclass + string("/") + devname + "/";
  string driver = driverpath + "/driver";
  char buffer[PATH_MAX + 1];
  int namelen = 0;

  if ((namelen = readlink(driver.c_str(), buffer, sizeof(buffer))) < 0)
    return "";

  return string(basename(buffer));
}

bool scan_sysfs(hwNode & n)
{
  (void) &id;			// avoid warning "id defined but not used"

  return false;
}
