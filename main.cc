#include "hw.h"
#include "print.h"

#include "mem.h"
#include "dmi.h"

#include <unistd.h>

int main(int argc,
	 char **argv)
{
  char hostname[80];

  if (gethostname(hostname, sizeof(hostname)) == 0)
  {
    hwNode computer(hostname,
		    hw::system);

    scan_memory(computer);
    scan_dmi(computer);

    print(computer);
  }

  return 0;
}
