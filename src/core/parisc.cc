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

static bool scan_device(hwNode & node, string name = "")
{
  struct dirent **namelist;
  int n;
  hwNode * curnode = NULL;

  if(name != "")
  {
    size_t colon = name.rfind(":");
    curnode = node.addChild(hwNode("device"));
    curnode->setBusInfo(guessBusInfo(name));
    if(exists("driver")) curnode->claim();
    if(colon!=name.npos)
      curnode->setPhysId(name.substr(colon+1));
    else
      curnode->setPhysId(name);
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
