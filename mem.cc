#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

bool scan_memory(hwNode & n)
{
  struct stat buf;
  hwNode *memory = n.getChild("core/memory");

  if (memory)
  {
    unsigned long long size = 0;

    memory->claim();

    for (int i = 0; i < memory->countChildren(); i++)
      if (memory->getChild(i)->getClass() == hw::memory)
	size += memory->getChild(i)->getSize();

    if ((size > 0) && (memory->getSize() == 0))
    {
      memory->setSize(size);
      return true;
    }
  }

  if (stat("/proc/kcore", &buf) == 0)
  {
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
	core->addChild(hwNode("memory", hw::memory));
	memory = core->getChild("memory");
      }
    }

    if (memory)
    {
      memory->claim();
      memory->setSize(buf.st_size);
      return true;
    }
  }

  return false;
}

static char *id = "@(#) $Id: mem.cc,v 1.12 2003/02/08 11:18:41 ezix Exp $";
