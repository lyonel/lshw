/*
 * mounts.cc
 *
 *
 */

#include "version.h"
#include "mounts.h"
#include "osutils.h"
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


__ID("@(#) $Id$");

#define MOUNTS "/proc/mounts"

static bool has_device(const string & dev, hwNode & n)
{
  string devid = get_devid(dev);
  vector <string> lnames;
  size_t i;

  if(devid == "")
    return false;

  if(get_devid(dev) == n.getDev())
    return true;

  lnames = n.getLogicalNames();
  for(i=0; i<lnames.size(); i++)
  {
    if(get_devid(lnames[i]) == devid)
      return true;
  }
  return false;
}

// unescape octal-encoded "special" characters:
//  "/this\040is\040a\040test" --> "/this is a test"

static string unescape(string s)
{
  size_t backslash = 0;
  string result = s;
  
  while((backslash=result.find('\\', backslash)) != string::npos)
  {
    string code = result.substr(backslash+1,3);
    if(matches(code, "^[0-9][0-9][0-9]$"))
    {
      result[backslash] = (char)strtol(code.c_str(), NULL, 8); // value is octal
      result.erase(backslash+1,3);
    }
    backslash++;
  }

  return result;
}

static void update_mount_status(hwNode & n, const vector <string> & mount)
{
  unsigned i;

  if(has_device(mount[0], n))
  {
    n.setConfig("state", "mounted");
    n.setLogicalName(mount[1]);		// mountpoint
    n.setConfig("mount.fstype", mount[2]);
    n.setConfig("mount.options", mount[3]);
  }

  for(i=0; i<n.countChildren(); i++)
    update_mount_status(*n.getChild(i), mount);
}

static bool process_mount(const string & s, hwNode & n)
{
  vector <string> mount;
  struct stat buf;

  // entries' format is
  // devicenode mountpoint fstype mountoptions dumpfrequency pass
  if(splitlines(s, mount, ' ') != 6)
  {
    mount.clear();
    if(splitlines(s, mount, '\t') != 6)
      return false;
  }

  mount[0] = unescape(mount[0]);
  mount[1] = unescape(mount[1]);

  if(mount[0][0] != '/')	// devicenode isn't a full path
    return false;

  if(stat(mount[0].c_str(), &buf) != 0)
    return false;

  if(!S_ISBLK(buf.st_mode))	// we're only interested in block devices
    return false;

  update_mount_status(n, mount);

  return true;
}

bool scan_mounts(hwNode & n)
{
  vector <string> mounts;
  size_t i;

  if(!loadfile(MOUNTS, mounts))
    return false;

  for(i=0; i<mounts.size(); i++)
    process_mount(mounts[i], n);

  return true;
}
