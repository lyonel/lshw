/*
 * parisc.cc
 *
 * This module parses the PA-RISC device tree (published under /sys
 * by the kernel).
 *
 *
 *
 *
 */

#include "device-tree.h"
#include "osutils.h"
#include "heuristics.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

static char *id =
  "@(#) $Id: device-tree.cc 910 2005-01-23 00:02:58Z ezix $";

#define DEVICESPARISC "/tmp/sys/devices/parisc"

#define HWTYPE_CPU	0x00
#define HWTYPE_GRAPHICS	0x0a
#define HWTYPE_FBUS	0x0c

static long get_long(const string & path)
{
  long result = -1;
  FILE * in = fopen(path.c_str(), "r");

  if (in)
  {
    if(fscanf(in, "%lx", &result) != 1)
      result = -1;
    fclose(in);
  }

  return result;
}


static bool scan_device(hwNode & node, string name = "")
{
  struct dirent **namelist;
  int n;
  hwNode * curnode = NULL;

  if(name != "")
  {
    hwNode newnode("device");
    size_t colon = name.rfind(":");

    switch(get_long("hw_type"))
    {
      case HWTYPE_CPU:
        newnode = hwNode("cpu", hw::processor);
        newnode.setDescription("CPU");
        break;
      case HWTYPE_GRAPHICS:
        newnode = hwNode("display", hw::display);
        break;
      case HWTYPE_FBUS:
        newnode = hwNode("bus", hw::bus);
        break;
    }

    newnode.setBusInfo(guessBusInfo(name));
    if(exists("driver"))
    {
      string driver = readlink("driver");
      size_t slash = driver.rfind("/");
      newnode.setConfig("driver", driver.substr(slash==driver.npos?0:slash+1));
      newnode.claim();
    }
    if(colon!=name.npos)
      newnode.setPhysId(name.substr(colon+1));
    else
      newnode.setPhysId(name);
    curnode = node.addChild(newnode);
  }

  n = scandir(".", &namelist, selectdir, alphasort);
  if (n < 0)
    return false;
  else
  {
    for (int i = 0; i < n; i++)
      if(matches(namelist[i]->d_name, "^[0-9]+(:[0-9]+)*$"))
      {
        pushd(namelist[i]->d_name);
        scan_device(curnode?*curnode:node, namelist[i]->d_name);
        popd();
        free(namelist[i]);
      }
    free(namelist);
  }


  return true;
}

bool scan_parisc(hwNode & node)
{
  hwNode *core = node.getChild("core");

  if (!core)
  {
    core = node.addChild(hwNode("core", hw::bus));
  }

  if (!core)
    return false;

  pushd(DEVICESPARISC);
  scan_device(*core);
  popd();

  (void) &id;			// avoid warning "id declared but not used"

  return true;
}
