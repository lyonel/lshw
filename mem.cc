#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

bool scan_memory(hwNode & n)
{
  struct stat buf;

  if (stat("/proc/kcore", &buf) == 0)
  {
    hwNode *memory = n.getChild("core/ram");

    if (!memory)
    {
      hwNode *core = n.getChild("core");

      if (!core)
      {
	n.addChild(hwNode("core", hw::system));
	core = n.getChild("core");
      }

      if (core)
      {
	core->addChild(hwNode("ram", hw::memory));
	memory = core->getChild("ram");
      }
    }

    if (memory)
    {
      memory->setSize(buf.st_size);
      memory->setSlot("logical");
      return true;
    }
  }

  return false;
}
