#include "device-tree.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define DEVICETREE "/proc/device-tree"

static string get_string(const string & path)
{
  int fd = open(path.c_str(), O_RDONLY);
  string result = "";

  if (fd >= 0)
  {
    struct stat buf;
    void *buffer = NULL;

    memset(&buf, 0, sizeof(buf));
    fstat(fd, &buf);

    buffer = malloc(buf.st_size);
    if (buffer)
    {
      read(fd, buffer, buf.st_size);
      result = string((char *) buffer, buf.st_size);
      free(buffer);
    }

    close(fd);
  }

  return result;
}

static void scan_devtree_root(hwNode & core)
{
  unsigned long frequency = 0;
  int fd = open(DEVICETREE "/clock-frequency", O_RDONLY);

  if (fd >= 0)
  {
    read(fd, &frequency, sizeof(frequency));

    core.setClock(frequency);
    close(fd);
  }
}

static void scan_devtree_bootrom(hwNode & core)
{
  struct stat buf;

  if (stat(DEVICETREE "/rom/boot-rom", &buf) == 0)
  {
    hwNode bootrom("firmware",
		   hw::memory);
    string upgrade = "";
    unsigned long base = 0;
    unsigned long size = 0;

    bootrom.setProduct(get_string(DEVICETREE "/rom/boot-rom/model"));
    bootrom.setDescription("BootROM");
    bootrom.
      setVersion(get_string(DEVICETREE "/rom/boot-rom/BootROM-version"));

    if ((upgrade =
	 get_string(DEVICETREE "/rom/boot-rom/write-characteristic")) != "")
    {
      bootrom.addCapability("upgrade");
      bootrom.addCapability(upgrade);
    }

    int fd = open(DEVICETREE "/rom/boot-rom/reg", O_RDONLY);
    if (fd >= 0)
    {
      read(fd, &base, sizeof(base));
      read(fd, &size, sizeof(size));

      bootrom.setSize(size);
      close(fd);
    }

    core.addChild(bootrom);
  }

  if (stat(DEVICETREE "/openprom", &buf) == 0)
  {
    hwNode openprom("firmware",
		    hw::memory);

    openprom.setProduct(get_string(DEVICETREE "/openprom/model"));

    if (stat(DEVICETREE "/openprom/supports-bootinfo", &buf) == 0)
      openprom.addCapability("bootinfo");

    core.addChild(openprom);
  }
}

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

  n.setProduct(get_string(DEVICETREE "/model"));
  n.setSerial(get_string(DEVICETREE "/system-id"));
  n.setVendor(get_string(DEVICETREE "/copyright"));

  if (core)
  {
    scan_devtree_root(*core);
    scan_devtree_bootrom(*core);
    scan_devtree_memory(*core);
    core->addCapability(get_string(DEVICETREE "/compatible"));
  }

  return true;
}
