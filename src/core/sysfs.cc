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

static char *id = "@(#) $Id$";

using namespace sysfs;

struct sysfs::entry_i
{
  string devclass;
  string devbus;
  string devname;
};

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

static sysfs_t fs;

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
  pushd(fs.path + "/bus");
  n = scandir(".", &namelist, selectdir, alphasort);
  popd();

  for (i = 0; i < n; i++)
  {
    devname =
      string(fs.path + "/bus/") + string(namelist[i]->d_name) +
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
  if (path.substr(0, 3) == "ide")
    return "ide@" + path.substr(path.length() - 3);
  else
    return "ide@" + path;
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

static string sysfs_getbusinfo_byclass(const string & devclass, const string & devname)
{
  string basename =
    fs.path + string("/class/") + devclass + string("/") + devname + "/";
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

static string sysfs_getbusinfo_bybus(const string & devbus, const string & devname)
{
  string device =
    fs.path + string("/bus/") + devbus + string("/devices/") + devname;
  char buffer[PATH_MAX + 1];

  if (!realpath(device.c_str(), buffer))
    return "";

  return sysfstobusinfo(hw::strip(buffer));
}

string sysfs_getbusinfo(const entry & e)
{
  if(e.This->devclass != "")
    return sysfs_getbusinfo_byclass(e.This->devclass, e.This->devname);
  if(e.This->devbus != "")
    return sysfs_getbusinfo_bybus(e.This->devbus, e.This->devname);
  return "";
}

string sysfs_getdriver(const string & devclass,
		       const string & devname)
{
  string driverpath =
    fs.path + string("/class/") + devclass + string("/") + devname + "/";
  string driver = driverpath + "/driver";
  char buffer[PATH_MAX + 1];
  int namelen = 0;

  if ((namelen = readlink(driver.c_str(), buffer, sizeof(buffer))) < 0)
    return "";

  return string(basename(buffer));
}

entry entry::byBus(string devbus, string devname)
{
  entry e;

  e.This->devbus = devbus;
  e.This->devname = devname;

  return e;
}

entry entry::byClass(string devclass, string devname)
{
  entry e;

  e.This->devclass = devclass;
  e.This->devname = devname;

  return e;
}

entry::entry()
{
  This = new entry_i;
}

entry & entry::operator =(const entry & e)
{
  
  *This = *(e.This);
  return *this;
}

entry::entry(const entry & e)
{
  This = new entry_i;

  *This = *(e.This);
}

entry::~entry()
{
  delete This;
}

bool scan_sysfs(hwNode & n)
{
  (void) &id;			// avoid warning "id defined but not used"

  return false;
}
