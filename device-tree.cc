#include "device-tree.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define DEVICETREE "/proc/device-tree"

static void scan_devtree_memory(hwNode & core)
{
  struct stat buf;
  unsigned int currentmc = 0;	// current memory controller
  hwNode *memory = core.getChild("memory");

  while (true)
  {
    char buffer[10];

    snprintf(buffer, sizeof(buffer), "%d", currentmc);
    string mcbase = string(DEVICETREE "/memory@") + string(buffer);
    string devtreeslotnames = mcbase + string("/slot-names");
    string reg = mcbase + string("/reg");

    if (stat(devtreeslotnames.c_str(), &buf) != 0)
      break;

    if (!memory || (currentmc != 0))
    {
      memory = core.addChild(hwNode("memory", hw::memory));
    }

    if (memory)
    {
      unsigned long bitmap = 0;
      char *slotnames = NULL;
      char *slotname = NULL;
      int fd = open(devtreeslotnames.c_str(), O_RDONLY);
      int fd2 = open(reg.c_str(), O_RDONLY);

      if ((fd >= 0) && (fd2 >= 0))
      {
	unsigned long slot = 1;
	slotnames = (char *) malloc(buf.st_size + 1);
	slotname = slotnames;
	memset(slotnames, 0, buf.st_size + 1);
	read(fd, &bitmap, sizeof(bitmap));
	read(fd, slotnames, buf.st_size + 1);

	while (strlen(slotname) > 0)
	{
	  unsigned long base = 0;
	  unsigned long size = 0;
	  if (bitmap & slot)	// slot is active
	  {
	    hwNode bank("bank",
			hw::memory);

	    read(fd2, &base, sizeof(base));
	    read(fd2, &size, sizeof(size));

	    bank.setSlot(slotname);
	    bank.setSize(size);
	    memory->addChild(bank);
	  }
	  slot *= 2;
	  slotname += strlen(slotname) + 1;
	}
	close(fd);
	close(fd2);

	free(slotnames);
      }

      currentmc++;
    }
    else
      break;

    memory = NULL;
  }
}

bool scan_device_tree(hwNode & n)
{
  hwNode *core = n.getChild("core");
  struct stat buf;

  if (stat(DEVICETREE, &buf) != 0)
    return false;

  if (!core)
  {
    n.addChild(hwNode("core", hw::system));
    core = n.getChild("core");
  }

  if (core)
  {
    scan_devtree_memory(*core);
  }

  return true;
}
