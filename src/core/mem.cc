/*
 * mem.cc
 *
 * This scan tries to guess the size of system memory by looking at several
 * sources:
 * - the size of /proc/kcore
 * - the value returned by the sysconf libc function
 * - the sum of sizes of individual memory banks
 *
 * NOTE: Except in last case, this guess can be widely inaccurate, as the
 * kernel itself limits the memory addressable by userspace processes.
 * Because of that, this module reports the biggest value found if it can't
 * access the size of individual memory banks (information filled in by other
 * scans like DMI (on PCs) or OpenFirmare device-tree (on Macs).
 *
 */

#include "version.h"
#include "config.h"
#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

__ID("@(#) $Id$");

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

    memory->claim(true);                          // claim memory and all its children

    for (unsigned int i = 0; i < memory->countChildren(); i++)
      if (memory->getChild(i)->getClass() == hw::memory)
        size += memory->getChild(i)->getSize();

    memory->setSize(size);
    return size;
  }
  else
    return 0;
}


static void claim_memory(hwNode & n)
{
  hwNode *core = n.getChild("core");

  if (core)
  {
    for (unsigned int i = 0; i < core->countChildren(); i++)
      if (core->getChild(i)->getClass() == hw::memory)
        if(core->getChild(i)->claimed())
          core->getChild(i)->claim(true);         // claim memory and all its children
  }
}


bool scan_memory(hwNode & n)
{
  hwNode *memory = n.getChild("core/memory");
  unsigned long long logicalmem = 0;
  unsigned long long kcore = 0;

  logicalmem = get_sysconf_size();
  kcore = get_kcore_size();
  count_memorybanks_size(n);
  claim_memory(n);

  if (!memory)
  {
    hwNode *core = n.getChild("core");

    if (!core)
    {
      n.addChild(hwNode("core", hw::bus));
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
    memory->addHint("icon", string("memory"));

    if (memory->getDescription() == "")
      memory->setDescription(_("System memory"));

    if (memory->getSize() > logicalmem)           // we already have a value
      return true;

    if ((logicalmem == 0)
      || ((kcore > logicalmem) && (kcore < 2 * logicalmem)))
      memory->setSize(kcore);
    else
      memory->setSize(logicalmem);

    return true;
  }

  return false;
}
