#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

bool scan_memory(hwNode & n)
{
  struct stat buf;

  if (stat("/proc/kcore", &buf) == 0)
  {
    hwNode memory("memory",
		  hw::memory);

    memory.setSize(buf.st_size);

    return n.addChild(memory);
  }

  return false;
}
