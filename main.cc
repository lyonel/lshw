#include "hw.h"
#include "print.h"

#include "mem.h"

#include <unistd.h>

int main(int argc,
	 char **argv)
{
  char hostname[80];

  if (gethostname(hostname, sizeof(hostname)) == 0)
  {
    hwNode computer(hostname);

    scan_memory(computer);

    computer.addChild(hwNode("cpu"));

    print(computer);
  }

  return 0;
}
