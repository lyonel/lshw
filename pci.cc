#include "pci.h"
#include "osutils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define PROC_BUS_PCI "/proc/bus/pci"
#define PCIID_PATH "/usr/local/share/pci.ids:/usr/share/pci.ids:/etc/pci.ids:/usr/share/hwdata/pci.ids"

#define PCI_CLASS_REVISION      0x08	/* High 24 bits are class, low 8 revision */
#define PCI_REVISION_ID         0x08	/* Revision ID */
#define PCI_CLASS_PROG          0x09	/* Reg. Level Programming Interface */
#define PCI_CLASS_DEVICE        0x0a	/* Device class */
#define PCI_PRIMARY_BUS         0x18	/* Primary bus number */
#define PCI_SECONDARY_BUS       0x19	/* Secondary bus number */

/*
 * The PCI interface treats multi-function devices as independent
 * devices.  The slot/function address of each device is encoded
 * in a single byte as follows:
 *
 *	7:3 = slot
 *	2:0 = function
 */
#define PCI_DEVFN(slot,func)	((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_SLOT(devfn)		(((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)		((devfn) & 0x07)

/* Device classes and subclasses */

#define PCI_CLASS_NOT_DEFINED		0x0000
#define PCI_CLASS_NOT_DEFINED_VGA	0x0001

#define PCI_BASE_CLASS_STORAGE		0x01
#define PCI_CLASS_STORAGE_SCSI		0x0100
#define PCI_CLASS_STORAGE_IDE		0x0101
#define PCI_CLASS_STORAGE_FLOPPY	0x0102
#define PCI_CLASS_STORAGE_IPI		0x0103
#define PCI_CLASS_STORAGE_RAID		0x0104
#define PCI_CLASS_STORAGE_OTHER		0x0180

#define PCI_BASE_CLASS_NETWORK		0x02
#define PCI_CLASS_NETWORK_ETHERNET	0x0200
#define PCI_CLASS_NETWORK_TOKEN_RING	0x0201
#define PCI_CLASS_NETWORK_FDDI		0x0202
#define PCI_CLASS_NETWORK_ATM		0x0203
#define PCI_CLASS_NETWORK_OTHER		0x0280

#define PCI_BASE_CLASS_DISPLAY		0x03
#define PCI_CLASS_DISPLAY_VGA		0x0300
#define PCI_CLASS_DISPLAY_XGA		0x0301
#define PCI_CLASS_DISPLAY_OTHER		0x0380

#define PCI_BASE_CLASS_MULTIMEDIA	0x04
#define PCI_CLASS_MULTIMEDIA_VIDEO	0x0400
#define PCI_CLASS_MULTIMEDIA_AUDIO	0x0401
#define PCI_CLASS_MULTIMEDIA_OTHER	0x0480

#define PCI_BASE_CLASS_MEMORY		0x05
#define  PCI_CLASS_MEMORY_RAM		0x0500
#define  PCI_CLASS_MEMORY_FLASH		0x0501
#define  PCI_CLASS_MEMORY_OTHER		0x0580

#define PCI_BASE_CLASS_BRIDGE		0x06
#define  PCI_CLASS_BRIDGE_HOST		0x0600
#define  PCI_CLASS_BRIDGE_ISA		0x0601
#define  PCI_CLASS_BRIDGE_EISA		0x0602
#define  PCI_CLASS_BRIDGE_MC		0x0603
#define  PCI_CLASS_BRIDGE_PCI		0x0604
#define  PCI_CLASS_BRIDGE_PCMCIA	0x0605
#define  PCI_CLASS_BRIDGE_NUBUS		0x0606
#define  PCI_CLASS_BRIDGE_CARDBUS	0x0607
#define  PCI_CLASS_BRIDGE_OTHER		0x0680

#define PCI_BASE_CLASS_COMMUNICATION	0x07
#define PCI_CLASS_COMMUNICATION_SERIAL	0x0700
#define PCI_CLASS_COMMUNICATION_PARALLEL 0x0701
#define PCI_CLASS_COMMUNICATION_OTHER	0x0780

#define PCI_BASE_CLASS_SYSTEM		0x08
#define PCI_CLASS_SYSTEM_PIC		0x0800
#define PCI_CLASS_SYSTEM_DMA		0x0801
#define PCI_CLASS_SYSTEM_TIMER		0x0802
#define PCI_CLASS_SYSTEM_RTC		0x0803
#define PCI_CLASS_SYSTEM_OTHER		0x0880

#define PCI_BASE_CLASS_INPUT		0x09
#define PCI_CLASS_INPUT_KEYBOARD	0x0900
#define PCI_CLASS_INPUT_PEN		0x0901
#define PCI_CLASS_INPUT_MOUSE		0x0902
#define PCI_CLASS_INPUT_OTHER		0x0980

#define PCI_BASE_CLASS_DOCKING		0x0a
#define PCI_CLASS_DOCKING_GENERIC	0x0a00
#define PCI_CLASS_DOCKING_OTHER		0x0a01

#define PCI_BASE_CLASS_PROCESSOR	0x0b
#define PCI_CLASS_PROCESSOR_386		0x0b00
#define PCI_CLASS_PROCESSOR_486		0x0b01
#define PCI_CLASS_PROCESSOR_PENTIUM	0x0b02
#define PCI_CLASS_PROCESSOR_ALPHA	0x0b10
#define PCI_CLASS_PROCESSOR_POWERPC	0x0b20
#define PCI_CLASS_PROCESSOR_CO		0x0b40

#define PCI_BASE_CLASS_SERIAL		0x0c
#define PCI_CLASS_SERIAL_FIREWIRE	0x0c00
#define PCI_CLASS_SERIAL_ACCESS		0x0c01
#define PCI_CLASS_SERIAL_SSA		0x0c02
#define PCI_CLASS_SERIAL_USB		0x0c03
#define PCI_CLASS_SERIAL_FIBER		0x0c04

#define PCI_CLASS_OTHERS		0xff

typedef unsigned long long pciaddr_t;
typedef enum
{ pcidevice,
  pcisubdevice,
  pcisubsystem,
  pciclass,
  pcisubclass,
  pcivendor,
  pcisubvendor,
  pciprogif
}
catalog;

struct pci_dev
{
  u_int16_t bus;		/* Higher byte can select host bridges */
  u_int8_t dev, func;		/* Device and function */

  u_int16_t vendor_id, device_id;	/* Identity of the device */
  unsigned int irq;		/* IRQ number */
  pciaddr_t base_addr[6];	/* Base addresses */
  pciaddr_t size[6];		/* Region sizes */
  pciaddr_t rom_base_addr;	/* Expansion ROM base address */
  pciaddr_t rom_size;		/* Expansion ROM size */

  u_int8_t config[256];
};

struct pci_entry
{
  long ids[4];
  string description;

    pci_entry(const string & description,
	      long u1 = -1,
	      long u2 = -1,
	      long u3 = -1,
	      long u4 = -1);

  unsigned int matches(long u1 = -1,
		       long u2 = -1,
		       long u3 = -1,
		       long u4 = -1);
};

static vector < pci_entry > pci_devices;
static vector < pci_entry > pci_classes;

pci_entry::pci_entry(const string & d,
		     long u1,
		     long u2,
		     long u3,
		     long u4)
{
  description = d;
  ids[0] = u1;
  ids[1] = u2;
  ids[2] = u3;
  ids[3] = u4;
}

unsigned int pci_entry::matches(long u1,
				long u2,
				long u3,
				long u4)
{
  unsigned int result = 0;

  if (ids[0] == u1)
  {
    result++;
    if (ids[1] == u2)
    {
      result++;
      if (ids[2] == u3)
      {
	result++;
	if (ids[3] == u4)
	  result++;
      }
    }
  }

  return result;
}

static const char *get_class_name(unsigned int c)
{
  switch (c)
  {
  case PCI_CLASS_NOT_DEFINED_VGA:
    return "display";
  case PCI_CLASS_STORAGE_SCSI:
    return "scsi";
  case PCI_CLASS_STORAGE_IDE:
    return "ide";
  case PCI_CLASS_BRIDGE_HOST:
    return "host";
  case PCI_CLASS_BRIDGE_ISA:
    return "isa";
  case PCI_CLASS_BRIDGE_EISA:
    return "eisa";
  case PCI_CLASS_BRIDGE_MC:
    return "mc";
  case PCI_CLASS_BRIDGE_PCI:
    return "pci";
  case PCI_CLASS_BRIDGE_PCMCIA:
    return "pcmcia";
  case PCI_CLASS_BRIDGE_NUBUS:
    return "nubus";
  case PCI_CLASS_BRIDGE_CARDBUS:
    return "pcmcia";
  case PCI_CLASS_SERIAL_FIREWIRE:
    return "firewire";
  case PCI_CLASS_SERIAL_USB:
    return "usb";
  case PCI_CLASS_SERIAL_FIBER:
    return "fiber";
  }

  switch (c >> 8)
  {
  case PCI_BASE_CLASS_STORAGE:
    return "storage";
  case PCI_BASE_CLASS_NETWORK:
    return "network";
  case PCI_BASE_CLASS_DISPLAY:
    return "display";
  case PCI_BASE_CLASS_MULTIMEDIA:
    return "multimedia";
  case PCI_BASE_CLASS_MEMORY:
    return "memory";
  case PCI_BASE_CLASS_BRIDGE:
    return "bridge";
  case PCI_BASE_CLASS_COMMUNICATION:
    return "communication";
  case PCI_BASE_CLASS_SYSTEM:
    return "system";
  case PCI_BASE_CLASS_INPUT:
    return "input";
  case PCI_BASE_CLASS_DOCKING:
    return "docking";
  case PCI_BASE_CLASS_PROCESSOR:
    return "processor";
  case PCI_BASE_CLASS_SERIAL:
    return "serial";
  }

  return "generic";
}

static bool parse_pcidb(vector < string > &list)
{
  long u[4];
  string line = "";
  catalog current_catalog = pcivendor;
  int level = 0;

  memset(u, 0, sizeof(u));

  for (int i = 0; i < list.size(); i++)
  {
    line = hw::strip(list[i]);

    // ignore empty or commented-out lines
    if (line.length() == 0 || line[0] == '#')
      continue;

    level = 0;
    while ((level < list[i].length()) && (list[i][level] == '\t'))
      level++;

    switch (level)
    {
    case 0:
      if ((line[0] == 'C') && (line.length() > 1) && (line[1] == ' '))
      {
	current_catalog = pciclass;
	line = line.substr(2);	// get rid of 'C '

	if ((line.length() < 3) || (line[2] != ' '))
	  return false;
	if (sscanf(line.c_str(), "%x", &u[0]) != 1)
	  return false;
	line = line.substr(3);
	line = hw::strip(line);
      }
      else
      {
	current_catalog = pcivendor;

	if ((line.length() < 5) || (line[4] != ' '))
	  return false;
	if (sscanf(line.c_str(), "%x", &u[0]) != 1)
	  return false;
	line = line.substr(5);
	line = hw::strip(line);
      }
      u[1] = u[2] = u[3] = -1;
      break;
    case 1:
      if ((current_catalog == pciclass) || (current_catalog == pcisubclass)
	  || (current_catalog == pciprogif))
      {
	current_catalog = pcisubclass;

	if ((line.length() < 3) || (line[2] != ' '))
	  return false;
	if (sscanf(line.c_str(), "%x", &u[1]) != 1)
	  return false;
	line = line.substr(3);
	line = hw::strip(line);
      }
      else
      {
	current_catalog = pcidevice;

	if ((line.length() < 5) || (line[4] != ' '))
	  return false;
	if (sscanf(line.c_str(), "%x", &u[1]) != 1)
	  return false;
	line = line.substr(5);
	line = hw::strip(line);
      }
      u[2] = u[3] = -1;
      break;
    case 2:
      if ((current_catalog != pcidevice) && (current_catalog != pcisubvendor)
	  && (current_catalog != pcisubclass)
	  && (current_catalog != pciprogif))
	return false;
      if ((current_catalog == pcisubclass) || (current_catalog == pciprogif))
      {
	current_catalog = pciprogif;
	if ((line.length() < 3) || (line[2] != ' '))
	  return false;
	if (sscanf(line.c_str(), "%x", &u[2]) != 1)
	  return false;
	u[3] = -1;
	line = line.substr(2);
	line = hw::strip(line);
      }
      else
      {
	current_catalog = pcisubvendor;
	if ((line.length() < 10) || (line[4] != ' ') || (line[9] != ' '))
	  return false;
	if (sscanf(line.c_str(), "%x%x", &u[2], &u[3]) != 2)
	  return false;
	line = line.substr(9);
	line = hw::strip(line);
      }
      break;
    default:
      return false;
    }

    //printf("%04x %04x %04x %04x %s\n", u[0], u[1], u[2], u[3], line.c_str());
    if ((current_catalog == pciclass) ||
	(current_catalog == pcisubclass) || (current_catalog == pciprogif))
    {
      pci_classes.push_back(pci_entry(line, u[0], u[1], u[2], u[3]));
    }
    else
    {
      pci_devices.push_back(pci_entry(line, u[0], u[1], u[2], u[3]));
    }
  }
  return true;
}

static bool load_pcidb()
{
  vector < string > lines;
  vector < string > filenames;

  splitlines(PCIID_PATH, filenames, ':');
  for (int i = filenames.size() - 1; i >= 0; i--)
  {
    lines.clear();
    if (loadfile(filenames[i], lines))
      parse_pcidb(lines);
  }

  if (lines.size() == 0)
    return false;

  return true;
}

static string get_class_description(long c,
				    long pi = -1)
{
  for (int i = 0; i < pci_classes.size(); i++)
  {
    if (pci_classes[i].matches(c >> 8, c & 0xff, pi))
    {
      return pci_classes[i].description;
    }
  }
  return "";
}

static u_int16_t get_conf_word(struct pci_dev d,
			       unsigned int pos)
{
  if (pos > sizeof(d.config))
    return 0;

  return d.config[pos] | (d.config[pos + 1] << 8);
}

static u_int8_t get_conf_byte(struct pci_dev d,
			      unsigned int pos)
{
  if (pos > sizeof(d.config))
    return 0;

  return d.config[pos];
}

static string pci_bushandle(u_int8_t bus)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "%02x", bus);

  return "PCIBUS:" + string(buffer);
}

static string pci_handle(u_int16_t bus,
			 u_int8_t dev,
			 u_int8_t fct)
{
  char buffer[20];

  snprintf(buffer, sizeof(buffer), "PCI:%02x:%02x.%x", bus, dev, fct);

  return string(buffer);
}

static void add_pci(hwNode & n,
		    hwNode & core)
{
}

bool scan_pci(hwNode & n)
{
  FILE *f;
  hwNode host("pci",
	      hw::bridge);

  // always consider the host bridge as PCI bus 00:
  host.setHandle(pci_bushandle(0));

  load_pcidb();

  f = fopen(PROC_BUS_PCI "/devices", "r");
  if (f)
  {
    char buf[512];

    while (fgets(buf, sizeof(buf) - 1, f))
    {
      unsigned int dfn, vend, cnt, known;
      struct pci_dev d;
      int fd = -1;
      string devicepath = "";
      char devicename[20];
      char driver[50];
      hwNode *device = NULL;

      memset(&d, 0, sizeof(d));
      memset(driver, 0, sizeof(driver));
      cnt = sscanf(buf,
		   "%x %x %x %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %s",
		   &dfn,
		   &vend,
		   &d.irq,
		   &d.base_addr[0],
		   &d.base_addr[1],
		   &d.base_addr[2],
		   &d.base_addr[3],
		   &d.base_addr[4],
		   &d.base_addr[5],
		   &d.rom_base_addr,
		   &d.size[0],
		   &d.size[1],
		   &d.size[2],
		   &d.size[3], &d.size[4], &d.size[5], &d.rom_size, driver);

      if (cnt != 9 && cnt != 10 && cnt != 17 && cnt != 18)
	break;

      d.bus = dfn >> 8;
      d.dev = PCI_SLOT(dfn & 0xff);
      d.func = PCI_FUNC(dfn & 0xff);
      d.vendor_id = vend >> 16;
      d.device_id = vend & 0xffff;

      snprintf(devicename, sizeof(devicename), "%02x/%02x.%x", d.bus, d.dev,
	       d.func);
      devicepath = string(PROC_BUS_PCI) + "/" + string(devicename);

      fd = open(devicepath.c_str(), O_RDONLY);
      if (fd >= 0)
      {
	read(fd, d.config, sizeof(d.config));
	close(fd);
      }

      u_int16_t dclass = get_conf_word(d, PCI_CLASS_DEVICE);

      if (dclass == PCI_CLASS_BRIDGE_HOST)
      {
	host.setDescription(get_class_description(dclass));
	host.setHandle(pci_bushandle(d.bus));
      }
      else
      {
	hw::hwClass deviceclass = hw::generic;

	switch (dclass >> 8)
	{
	case PCI_BASE_CLASS_STORAGE:
	  deviceclass = hw::storage;
	  break;
	case PCI_BASE_CLASS_NETWORK:
	  deviceclass = hw::network;
	  break;
	case PCI_BASE_CLASS_MEMORY:
	  deviceclass = hw::memory;
	  break;
	case PCI_BASE_CLASS_BRIDGE:
	  deviceclass = hw::bridge;
	  break;
	case PCI_BASE_CLASS_MULTIMEDIA:
	  deviceclass = hw::multimedia;
	  break;
	case PCI_BASE_CLASS_DISPLAY:
	  deviceclass = hw::display;
	  break;
	case PCI_BASE_CLASS_COMMUNICATION:
	  deviceclass = hw::communication;
	  break;
	case PCI_BASE_CLASS_SYSTEM:
	  deviceclass = hw::system;
	  break;
	case PCI_BASE_CLASS_INPUT:
	  deviceclass = hw::input;
	  break;
	case PCI_BASE_CLASS_PROCESSOR:
	  deviceclass = hw::processor;
	  break;
	case PCI_BASE_CLASS_SERIAL:
	  deviceclass = hw::bus;
	  break;
	}

	device = new hwNode(get_class_name(dclass), deviceclass);

	if (device)
	{
	  if (dclass == PCI_CLASS_BRIDGE_PCI)
	    device->
	      setHandle(pci_bushandle(get_conf_byte(d, PCI_SECONDARY_BUS)));
	  else
	    device->setHandle(pci_handle(d.bus, d.dev, d.func));
	  device->setDescription(get_class_description(dclass));

	  hwNode *bus = host.findChildByHandle(pci_bushandle(d.bus));

	  if (bus)
	    bus->addChild(*device);
	  else
	    host.addChild(*device);
	  free(device);
	}
      }

    }
    fclose(f);

    hwNode *core = n.getChild("core");
    if (!core)
    {
      n.addChild(hwNode("core", hw::system));
      core = n.getChild("core");
    }

    if (core)
      core->addChild(host);
  }

  return false;
}

static char *id = "@(#) $Id: pci.cc,v 1.9 2003/01/28 09:43:47 ezix Exp $";
