#include "hw.h"
#include "print.h"

#include "version.h"
#include "mem.h"
#include "dmi.h"
#include "cpuinfo.h"
#include "device-tree.h"

#include <unistd.h>

int main(int argc,
	 char **argv)
{
  char hostname[80];

  cout << argv[0] << " version " << getpackageversion() << endl << endl;

  if (gethostname(hostname, sizeof(hostname)) == 0)
  {
    hwNode computer(hostname,
		    hw::system);

    scan_dmi(computer);
    scan_device_tree(computer);
    scan_memory(computer);
    scan_cpuinfo(computer);

    print(computer);
  }

  return 0;
}

static char *id = "@(#) $Id: main.cc,v 1.9 2003/01/25 10:03:23 ezix Exp $";
