#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <unistd.h>

bool scan_memory(hwNode & n)
{
  struct stat buf;
  struct sysinfo info;
  hwNode *memory = n.getChild("core/memory");

  if (sysinfo(&info) != 0)
    memset(&info, 0, sizeof(info));

  if (memory)
  {
    unsigned long long size = 0;

    memory->claim(true);	// claim memory and all its children

    for (int i = 0; i < memory->countChildren(); i++)
      if (memory->getChild(i)->getClass() == hw::memory)
	size += memory->getChild(i)->getSize();

    if ((size > 0) && (memory->getSize() < size))
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

static char *id = "@(#) $Id: mem.cc,v 1.14 2003/04/02 16:24:44 ezix Exp $";
