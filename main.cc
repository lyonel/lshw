#include "hw.h"
#include "print.h"

#include "version.h"
#include "mem.h"
#include "dmi.h"
#include "cpuinfo.h"
#include "device-tree.h"
#include "pci.h"

#include <unistd.h>

int main(int argc,
	 char **argv)
{
  char hostname[80];

  if (gethostname(hostname, sizeof(hostname)) == 0)
  {
    hwNode computer(hostname,
		    hw::system);

    scan_dmi(computer);
    scan_device_tree(computer);
    scan_memory(computer);
    scan_cpuinfo(computer);
    scan_pci(computer);

    print(computer, false);
  }

  return 0;
}

static char *id = "@(#) $Id: main.cc,v 1.12 2003/01/29 21:30:07 ezix Exp $";
