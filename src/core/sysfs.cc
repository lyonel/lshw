/*
 * sysfs.cc
 *
 *
 */

#include "version.h"
#include "sysfs.h"
#include "osutils.h"
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>


__ID("@(#) $Id$");

using namespace sysfs;

struct sysfs::entry_i
{
  string devpath;
};

struct sysfs_t
{
  sysfs_t():path("/sys"),
    temporary(false),
    has_sysfs(false)
  {
    has_sysfs = exists(path + "/class/.");

    if (!has_sysfs)                               // sysfs doesn't seem to be mounted
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
          0000);                                  // to make clear it is a mount point
        mount("none",
          path.c_str(),
          "sysfs",
          0,
          NULL);
      }

      has_sysfs = exists(path + "/class/.");
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

static string sysfs_getbustype(const string & path)
{
  struct dirent **namelist;
  int i, n;
  string bustype = "";

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

  if (n <= 0)
    return "";

  for (i = 0; i < n; i++)
  {
    string devname =
      string(fs.path + "/bus/") + string(namelist[i]->d_name) +
      "/devices/" + shortname(path);

    if (samefile(devname, path))
    {
      bustype = string(namelist[i]->d_name);
      break;
    }
    free(namelist[i]);
  }

  for (int j = i; j < n; j++)
    free(namelist[j]);
  free(namelist);

  return bustype;
}


static string sysfstopci(const string & path)
{
  if (path.length() > strlen("XXXX:XX:XX.X"))
    return "pci@" + path.substr(path.length() - strlen("XXXX:XX:XX.X"));
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

  if (bustype == "usb")
  {
    string name = shortname(path);
    if (matches(name, "^[0-9]+-[0-9]+(\\.[0-9]+)*:[0-9]+\\.[0-9]+$"))
    {
      size_t colon = name.rfind(":");
      size_t dash = name.find("-");
      return "usb@" + name.substr(0, dash) + ":" + name.substr(dash+1, colon-dash-1);
    }
  }

  if (bustype == "virtio")
  {
    string name = shortname(path);
    if (name.compare(0, 6, "virtio") == 0)
      return "virtio@" + name.substr(6);
    else
      return "virtio@" + name;
  }

  if (bustype == "vio")
    return string("vio@") + shortname(path);

  if (bustype == "ccw")
    return string("ccw@") + shortname(path);

  if (bustype == "ccwgroup")
  {
    // just report businfo for the first device in the group
    // because the group doesn't really fit into lshw's tree model
    string firstdev = realpath(path + "/cdev0");
    return sysfstobusinfo(firstdev);
  }

  return "";
}


string entry::businfo() const
{
  if(!This) return "";
  string result = sysfstobusinfo(This->devpath);
  if (result.empty())
    result = sysfstobusinfo(dirname(This->devpath));
  return result;
}


static string finddevice(const string & name, const string & root = "")
{
  struct dirent **namelist;
  int n;
  string result = "";

  if(exists(name))
    return root + "/" + name;

  n = scandir(".", &namelist, selectdir, alphasort);

  for (int i = 0; i < n; i++)
  {
    pushd(namelist[i]->d_name);
    string findinchild = finddevice(name, root + "/" + string(namelist[i]->d_name));
    popd();

    free(namelist[i]);
    if(findinchild != "")
    {
      result = findinchild;
    }
  }
  free(namelist);

  return result;
}


string sysfs_finddevice(const string & name)
{
  string devices = fs.path + string("/devices/");
  string result = "";

  if(!pushd(devices))
    return "";
  result = finddevice(name);
  popd();

  return result;
}

entry entry::leaf() const
{
  if (hassubdir("device"))
    return entry(This->devpath+"/device");

  return entry(This->devpath);
}

string entry::driver() const
{
  string driverlink = This->devpath + "/driver";
  if (!exists(driverlink))
    return "";
  return shortname(readlink(driverlink));
}


entry entry::byBus(string devbus, string devname)
{
  entry e(fs.path + "/bus/" + devbus + "/devices/" + devname);
  return e;
}


entry entry::byClass(string devclass, string devname)
{
  entry e(fs.path + "/class/" + devclass + "/" + devname);
  return e;
}


entry entry::byPath(string path)
{
  entry e(fs.path + "/devices" + path);
  return e;
}


entry::entry(const string & devpath)
{
  This = new entry_i;
  This->devpath = realpath(devpath);
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

bool entry::hassubdir(const string & s) const
{
  return exists(This->devpath + "/" + s);
}


string entry::name_in_class(const string & classname) const
{
  string result = "";

  string classdir = This->devpath + "/" + classname;
  if (!pushd(classdir))
    return result;

  struct dirent **namelist = NULL;
  int count = scandir(".", &namelist, selectdir, alphasort);
  popd();

  if (count < 0)
    return result;

  // there should be at most one
  for (int i = 0; i < count; i++)
  {
    result = namelist[i]->d_name;
    free(namelist[i]);
  }
  free(namelist);

  return result;
}


string entry::name() const
{
  return shortname(This->devpath);
}


entry entry::parent() const
{
  entry e(dirname(This->devpath));
  return e;
}

string entry::classname() const
{
  return shortname(dirname(This->devpath));
}

string entry::subsystem() const
{
  return shortname(realpath(This->devpath+"/subsystem"));
}

bool entry::isvirtual() const
{
  return shortname(dirname(dirname(This->devpath))) == "virtual";
}

string entry::string_attr(const string & name, const string & def) const
{
  return hw::strip(get_string(This->devpath + "/" + name, def));
}


unsigned long long entry::hex_attr(const string & name, unsigned long long def) const
{
  string val = string_attr(name, "");
  if (val.empty())
    return def;
  return strtoull(val.c_str(), NULL, 16);
}


vector < string > entry::multiline_attr(const string & name) const
{
  vector < string > lines;
  loadfile(This->devpath + "/" + name, lines);
  return lines;
}


string entry::modalias() const
{
  return get_string(This->devpath+"/modalias");
}

string entry::device() const
{
  return get_string(This->devpath+"/device");
}

string entry::vendor() const
{
  return get_string(This->devpath+"/vendor");
}

vector < entry > entry::devices() const
{
  vector < entry > result;

  if (!pushd(This->devpath))
    return result;

  struct dirent **namelist;
  int count = scandir(".", &namelist, selectdir, alphasort);
  for (int i = 0; i < count; i ++)
  {
    entry e = sysfs::entry(This->devpath + "/" + string(namelist[i]->d_name));
    if(e.hassubdir("subsystem"))
	    result.push_back(e);
    free(namelist[i]);
  }
  if (namelist)
    free(namelist);

  if(pushd("block"))
  {
    int count = scandir(".", &namelist, selectdir, alphasort);
    for (int i = 0; i < count; i ++)
    {
      entry e = sysfs::entry(This->devpath + "/block/" + string(namelist[i]->d_name));
      if(e.hassubdir("subsystem"))
	      result.push_back(e);
      free(namelist[i]);
    }
    if (namelist)
      free(namelist);
    popd();
  }
  popd();
  return result;
}

vector < entry > sysfs::entries_by_bus(const string & busname)
{
  vector < entry > result;

  if (!pushd(fs.path + "/bus/" + busname + "/devices"))
    return result;

  struct dirent **namelist;
  int count;
  count = scandir(".", &namelist, selectlink, alphasort);
  for (int i = 0; i < count; i ++)
  {
    entry e = sysfs::entry::byBus(busname, namelist[i]->d_name);
    result.push_back(e);
    free(namelist[i]);
  }
  popd();
  if (namelist)
    free(namelist);
  return result;
}

vector < entry > sysfs::entries_by_class(const string & classname)
{
  vector < entry > result;

  if (!pushd(fs.path + "/class/" + classname))
    return result;

  struct dirent **namelist;
  int count;
  count = scandir(".", &namelist, selectlink, alphasort);
  for (int i = 0; i < count; i ++)
  {
    entry e = sysfs::entry::byClass(classname, namelist[i]->d_name);
    result.push_back(e);
    free(namelist[i]);
  }
  popd();
  if (namelist)
    free(namelist);
  return result;
}

bool scan_sysfs(hwNode & n)
{
  return false;
}
