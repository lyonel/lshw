#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

bool scan_memory(hwNode & n)
{
  struct stat buf;
  hwNode *memory = n.getChild("core/memory");
  long pagesize = 0;
  unsigned long long logicalmem = 0;

  pagesize = sysconf(_SC_PAGESIZE);
  if (pagesize > 0)
    logicalmem = pagesize * sysconf(_SC_PHYS_PAGES);

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
      if ((logicalmem == 0) || (logicalmem > buf.st_size / 2))
	memory->setSize(buf.st_size);
      else
	memory->setSize(logicalmem);
      return true;
    }
  }

  return false;
}

static char *id = "@(#) $Id: mem.cc,v 1.15 2003/04/10 17:05:29 ezix Exp $";
