#include "device-tree.h"
#include "osutils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

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

static unsigned long get_long(const string & path)
{
  unsigned long result = 0;
  int fd = open(path.c_str(), O_RDONLY);

  if (fd >= 0)
  {
    read(fd, &result, sizeof(result));

    close(fd);
  }

  return result;
}

static bool exists(const string & path)
{
  return access(path.c_str(), F_OK) == 0;
}

static void scan_devtree_root(hwNode & core)
{
  core.setClock(get_long(DEVICETREE "/clock-frequency"));
}

static void scan_devtree_bootrom(hwNode & core)
{
  if (exists(DEVICETREE "/rom/boot-rom"))
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

  if (exists(DEVICETREE "/openprom"))
  {
    hwNode openprom("firmware",
		    hw::memory);

    openprom.setProduct(get_string(DEVICETREE "/openprom/model"));

    if (exists(DEVICETREE "/openprom/supports-bootinfo"))
      openprom.addCapability("bootinfo");

    core.addChild(openprom);
  }
}

static int selectdir(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (stat(d->d_name, &buf) != 0)
    return 0;

  return S_ISDIR(buf.st_mode);
}

static void scan_devtree_cpu(hwNode & core)
{
  struct dirent **namelist;
  int n;

  pushd(DEVICETREE "/cpus");
  n = scandir(".", &namelist, selectdir, NULL);
  popd();
  if (n < 0)
    return;
  else
  {
    for (int i = 0; i < n; i++)
    {
      string basepath =
	string(DEVICETREE "/cpus/") + string(namelist[i]->d_name);
      unsigned long version = 0;
      unsigned long cachesize = 0;
      hwNode cpu("cpu",
		 hw::processor);
      struct dirent **cachelist;
      int ncache;

      if (hw::strip(get_string(basepath + "/device_type")) != "cpu")
	break;			// oops, not a CPU!

      cpu.setProduct(get_string(basepath + "/name"));
      cpu.setSize(get_long(basepath + "/clock-frequency"));
      cpu.setClock(get_long(basepath + "/bus-frequency"));
      if (exists(basepath + "/altivec"))
	cpu.addCapability("altivec");

      version = get_long(basepath + "/cpu-version");
      if (version != 0)
      {
	int minor = version & 0x00ff;
	int major = (version & 0xff00) >> 8;
	char buffer[20];

	snprintf(buffer, sizeof(buffer), "%x.%d.%d",
		 (version & 0xffff0000) >> 16, major, minor);
	cpu.setVersion(buffer);

      }
      if (hw::strip(get_string(basepath + "/state")) != "running")
	cpu.disable();

      if (exists(basepath + "/performance-monitor"))
	cpu.addCapability("performance-monitor");

      if (exists(basepath + "/d-cache-size"))
      {
	hwNode cache("cache",
		     hw::memory);

	cache.setDescription("L1 Cache");
	cache.setSize(get_long(basepath + "/d-cache-size"));
	if (cache.getSize() > 0)
	  cpu.addChild(cache);
      }

      pushd(basepath);
      ncache = scandir(".", &cachelist, selectdir, NULL);
      popd();
      if (ncache > 0)
      {
	for (int j = 0; j < ncache; j++)
	{
	  hwNode cache("cache",
		       hw::memory);
	  string cachebase = basepath + "/" + cachelist[j]->d_name;

	  if (hw::strip(get_string(cachebase + "/device_type")) != "cache" &&
	      hw::strip(get_string(cachebase + "/device_type")) != "l2-cache")
	    break;		// oops, not a cache!

	  cache.setDescription("L2 Cache");
	  cache.setSize(get_long(cachebase + "/d-cache-size"));
	  cache.setClock(get_long(cachebase + "/clock-frequency"));

	  if (exists(cachebase + "/cache-unified"))
	    cache.setDescription(cache.getDescription() + " (unified)");
	  else
	  {
	    hwNode icache = cache;
	    cache.setDescription(cache.getDescription() + " (data)");
	    icache.setDescription(icache.getDescription() + " (instruction)");
	    icache.setSize(get_long(cachebase + "/i-cache-size"));

	    if (icache.getSize() > 0)
	      cpu.addChild(icache);
	  }

	  if (cache.getSize() > 0)
	    cpu.addChild(cache);

	  free(cachelist[j]);
	}
	free(cachelist);
      }

      core.addChild(cpu);

      free(namelist[i]);
    }
    free(namelist);
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

  if (!exists(DEVICETREE))
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
    scan_devtree_cpu(*core);
    core->addCapability(get_string(DEVICETREE "/compatible"));
  }

  return true;
}
