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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

static char *id =
  "@(#) $Id: device-tree.cc 910 2005-01-23 00:02:58Z ezix $";

#define PARISCDEVICES "/sys/bus/parisc/devices"

bool scan_parisc(hwNode & n)
{
  struct dirent **namelist;
  int n;
  hwNode *core = n.getChild("core");

  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  pushd(DEVICETREE "/cpus");
  n = scandir(".", &namelist, selectdir, alphasort);
  popd();
  if (n < 0)
    return;
  else
  {
    for (int i = 0; i < n; i++)
    {
      free(namelist[i]);
    }
    free(namelist);
  }

  (void) &id;			// avoid warning "id declared but not used"

  return true;
}
