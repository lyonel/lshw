#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static char *id = "@(#) $Id: mem.cc,v 1.19 2003/04/29 07:56:10 ezix Exp $";

static unsigned long long get_kcore_size()
{
  struct stat buf;

  if (stat("/proc/kcore", &buf) != 0)
    return 0;
  else
    return buf.st_size;
}

static unsigned long long get_sysconf_size()
{
  long pagesize = 0;
  long physpages = 0;
  unsigned long long logicalmem = 0;

  pagesize = sysconf(_SC_PAGESIZE);
  physpages = sysconf(_SC_PHYS_PAGES);
  if ((pagesize > 0) && (physpages > 0))
    logicalmem =
      (unsigned long long) pagesize *(unsigned long long) physpages;

  return logicalmem;
}

static unsigned long long count_memorybanks_size(hwNode & n)
{
  hwNode *memory = n.getChild("core/memory");

  if (memory)
  {
    unsigned long long size = 0;

    memory->claim(true);	// claim memory and all its children

    for (unsigned int i = 0; i < memory->countChildren(); i++)
      if (memory->getChild(i)->getClass() == hw::memory)
	size += memory->getChild(i)->getSize();

    memory->setSize(size);
    return size;
  }
  else
    return 0;
}

bool scan_memory(hwNode & n)
{
  hwNode *memory = n.getChild("core/memory");
  unsigned long long logicalmem = 0;
  unsigned long long kcore = 0;

  logicalmem = get_sysconf_size();
  kcore = get_kcore_size();
  count_memorybanks_size(n);

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

    if (memory->getSize() > logicalmem)	// we already have a value
      return true;

    if ((logicalmem == 0)
	|| ((kcore > logicalmem) && (kcore < 2 * logicalmem)))
      memory->setSize(kcore);
    else
      memory->setSize(logicalmem);

    return true;
  }

  (void) &id;			// avoid warning "id defined but not used"

  return false;
}
