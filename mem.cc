#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

bool scan_memory(hwNode & n)
{
  hwNode memory("memory",
		hw::memory);
  struct stat buf;

  if (stat("/proc/kcore", &buf) == 0)
  {
    memory.setSize(buf.st_size);

    if (stat("/proc/device-tree/memory@0/slot-names", &buf) == 0)
    {
      unsigned long bitmap = 0;
      char *slotnames = NULL;
      char *slotname = NULL;
      int fd = open("/proc/device-tree/memory@0/slot-names", O_RDONLY);
      int fd2 = open("/proc/device-tree/memory@0/reg", O_RDONLY);

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
	    hwNode bank(slotname,
			hw::memory);

	    read(fd2, &base, sizeof(base));
	    read(fd2, &size, sizeof(size));

	    bank.setSlot(slotname);
	    bank.setSize(size);
	    memory.addChild(bank);
	  }
	  slot *= 2;
	  slotname += strlen(slotname) + 1;
	}
	close(fd);
	close(fd2);

	free(slotnames);
      }
    }

  }

  return n.addChild(memory);
}
