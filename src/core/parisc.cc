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

bool scan_parisc(hwNode & n)
{
  hwNode *core = n.getChild("core");

  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  (void) &id;			// avoid warning "id declared but not used"

  return true;
}
