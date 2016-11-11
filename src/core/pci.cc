#include "version.h"
#include "config.h"
#include "pci.h"
#include "device-tree.h"
#include "osutils.h"
#include "options.h"
#include "sysfs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <libgen.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

__ID("@(#) $Id$");

#define PROC_BUS_PCI "/proc/bus/pci"
#define SYS_BUS_PCI "/sys/bus/pci"
#define PCIID_PATH DATADIR"/pci.ids:/usr/share/lshw/pci.ids:/usr/local/share/pci.ids:/usr/share/pci.ids:/etc/pci.ids:/usr/share/hwdata/pci.ids:/usr/share/misc/pci.ids"

#define PCI_CLASS_REVISION      0x08              /* High 24 bits are class, low 8 revision */
#define PCI_VENDOR_ID           0x00              /* 16 bits */
#define PCI_DEVICE_ID           0x02              /* 16 bits */
#define PCI_COMMAND             0x04              /* 16 bits */
#define PCI_REVISION_ID         0x08              /* Revision ID */
#define PCI_CLASS_PROG          0x09              /* Reg. Level Programming Interface */
#define PCI_CLASS_DEVICE        0x0a              /* Device class */
#define PCI_HEADER_TYPE         0x0e              /* 8 bits */
#define PCI_HEADER_TYPE_NORMAL     0
#define PCI_HEADER_TYPE_BRIDGE     1
#define PCI_HEADER_TYPE_CARDBUS    2
#define PCI_PRIMARY_BUS         0x18              /* Primary bus number */
#define PCI_SECONDARY_BUS       0x19              /* Secondary bus number */
#define PCI_STATUS              0x06              /* 16 bits */
#define PCI_LATENCY_TIMER       0x0d              /* 8 bits */
#define PCI_SEC_LATENCY_TIMER   0x1b              /* Latency timer for secondary interface */
#define PCI_CB_LATENCY_TIMER    0x1b              /* CardBus latency timer */
#define PCI_STATUS_66MHZ        0x20              /* Support 66 Mhz PCI 2.1 bus */
#define PCI_STATUS_CAP_LIST     0x10              /* Support Capability List */
#define PCI_COMMAND_IO          0x01              /* Enable response in I/O space */
#define PCI_COMMAND_MEMORY      0x02              /* Enable response in Memory space */
#define PCI_COMMAND_MASTER      0x04              /* Enable bus mastering */
#define PCI_COMMAND_SPECIAL     0x08              /* Enable response to special cycles */
#define PCI_COMMAND_INVALIDATE  0x10              /* Use memory write and invalidate */
#define PCI_COMMAND_VGA_PALETTE 0x20              /* Enable palette snooping */
#define PCI_COMMAND_PARITY      0x40              /* Enable parity checking */
#define PCI_COMMAND_WAIT        0x80              /* Enable address/data stepping */
#define PCI_COMMAND_SERR        0x100             /* Enable SERR */
#define PCI_COMMAND_FAST_BACK   0x200             /* Enable back-to-back writes */

#define PCI_MIN_GNT             0x3e              /* 8 bits */
#define PCI_MAX_LAT             0x3f              /* 8 bits */

#define PCI_CAPABILITY_LIST     0x34              /* Offset of first capability list entry */
#define PCI_CAP_LIST_ID            0              /* Capability ID */
#define PCI_CAP_ID_PM           0x01              /* Power Management */
#define PCI_CAP_ID_AGP          0x02              /* Accelerated Graphics Port */
#define PCI_CAP_ID_VPD          0x03              /* Vital Product Data */
#define PCI_CAP_ID_SLOTID       0x04              /* Slot Identification */
#define PCI_CAP_ID_MSI          0x05              /* Message Signalled Interrupts */
#define PCI_CAP_ID_CHSWP        0x06              /* CompactPCI HotSwap */
#define PCI_CAP_ID_PCIX         0x07              /* PCI-X */
#define PCI_CAP_ID_HT           0x08              /* HyperTransport */
#define PCI_CAP_ID_VNDR         0x09              /* Vendor specific */
#define PCI_CAP_ID_DBG          0x0A              /* Debug port */
#define PCI_CAP_ID_CCRC         0x0B              /* CompactPCI Central Resource Control */
#define PCI_CAP_ID_AGP3         0x0E              /* AGP 8x */
#define PCI_CAP_ID_EXP          0x10              /* PCI Express */
#define PCI_CAP_ID_MSIX         0x11              /* MSI-X */
#define PCI_CAP_LIST_NEXT          1              /* Next capability in the list */
#define PCI_CAP_FLAGS              2              /* Capability defined flags (16 bits) */
#define PCI_CAP_SIZEOF             4
#define PCI_FIND_CAP_TTL          48

#define PCI_SID_ESR                2              /* Expansion Slot Register */
#define PCI_SID_ESR_NSLOTS      0x1f              /* Number of expansion slots available */


/*
 * The PCI interface treats multi-function devices as independent
 * devices.  The slot/function address of each device is encoded
 * in a single byte as follows:
 *
 *	7:3 = slot
 *	2:0 = function
 */
#define PCI_DEVFN(slot,func)  ((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_SLOT(devfn)   (((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)   ((devfn) & 0x07)

/* Device classes and subclasses */

#define PCI_CLASS_NOT_DEFINED        0x0000
#define PCI_CLASS_NOT_DEFINED_VGA    0x0001

#define PCI_BASE_CLASS_STORAGE       0x01
#define PCI_CLASS_STORAGE_SCSI       0x0100
#define PCI_CLASS_STORAGE_IDE        0x0101
#define PCI_CLASS_STORAGE_FLOPPY     0x0102
#define PCI_CLASS_STORAGE_IPI        0x0103
#define PCI_CLASS_STORAGE_RAID       0x0104
#define PCI_CLASS_STORAGE_OTHER      0x0180

#define PCI_BASE_CLASS_NETWORK       0x02
#define PCI_CLASS_NETWORK_ETHERNET   0x0200
#define PCI_CLASS_NETWORK_TOKEN_RING 0x0201
#define PCI_CLASS_NETWORK_FDDI       0x0202
#define PCI_CLASS_NETWORK_ATM        0x0203
#define PCI_CLASS_NETWORK_OTHER      0x0280

#define PCI_BASE_CLASS_DISPLAY       0x03
#define PCI_CLASS_DISPLAY_VGA        0x0300
#define PCI_CLASS_DISPLAY_XGA        0x0301
#define PCI_CLASS_DISPLAY_OTHER      0x0380

#define PCI_BASE_CLASS_MULTIMEDIA    0x04
#define PCI_CLASS_MULTIMEDIA_VIDEO   0x0400
#define PCI_CLASS_MULTIMEDIA_AUDIO   0x0401
#define PCI_CLASS_MULTIMEDIA_OTHER   0x0480

#define PCI_BASE_CLASS_MEMORY        0x05
#define PCI_CLASS_MEMORY_RAM         0x0500
#define PCI_CLASS_MEMORY_FLASH       0x0501
#define PCI_CLASS_MEMORY_OTHER       0x0580

#define PCI_BASE_CLASS_BRIDGE        0x06
#define PCI_CLASS_BRIDGE_HOST        0x0600
#define PCI_CLASS_BRIDGE_ISA         0x0601
#define PCI_CLASS_BRIDGE_EISA        0x0602
#define PCI_CLASS_BRIDGE_MC          0x0603
#define PCI_CLASS_BRIDGE_PCI         0x0604
#define PCI_CLASS_BRIDGE_PCMCIA      0x0605
#define PCI_CLASS_BRIDGE_NUBUS       0x0606
#define PCI_CLASS_BRIDGE_CARDBUS     0x0607
#define PCI_CLASS_BRIDGE_OTHER       0x0680

#define PCI_BASE_CLASS_COMMUNICATION     0x07
#define PCI_CLASS_COMMUNICATION_SERIAL   0x0700
#define PCI_CLASS_COMMUNICATION_PARALLEL 0x0701
#define PCI_CLASS_COMMUNICATION_MODEM    0x0703
#define PCI_CLASS_COMMUNICATION_OTHER    0x0780

#define PCI_BASE_CLASS_SYSTEM        0x08
#define PCI_CLASS_SYSTEM_PIC         0x0800
#define PCI_CLASS_SYSTEM_DMA         0x0801
#define PCI_CLASS_SYSTEM_TIMER       0x0802
#define PCI_CLASS_SYSTEM_RTC         0x0803
#define PCI_CLASS_SYSTEM_OTHER       0x0880

#define PCI_BASE_CLASS_INPUT         0x09
#define PCI_CLASS_INPUT_KEYBOARD     0x0900
#define PCI_CLASS_INPUT_PEN          0x0901
#define PCI_CLASS_INPUT_MOUSE        0x0902
#define PCI_CLASS_INPUT_OTHER        0x0980

#define PCI_BASE_CLASS_DOCKING       0x0a
#define PCI_CLASS_DOCKING_GENERIC    0x0a00
#define PCI_CLASS_DOCKING_OTHER      0x0a01

#define PCI_BASE_CLASS_PROCESSOR     0x0b
#define PCI_CLASS_PROCESSOR_386      0x0b00
#define PCI_CLASS_PROCESSOR_486      0x0b01
#define PCI_CLASS_PROCESSOR_PENTIUM  0x0b02
#define PCI_CLASS_PROCESSOR_ALPHA    0x0b10
#define PCI_CLASS_PROCESSOR_POWERPC  0x0b20
#define PCI_CLASS_PROCESSOR_CO       0x0b40

#define PCI_BASE_CLASS_SERIAL        0x0c
#define PCI_CLASS_SERIAL_FIREWIRE    0x0c00
#define PCI_CLASS_SERIAL_ACCESS      0x0c01
#define PCI_CLASS_SERIAL_SSA         0x0c02
#define PCI_CLASS_SERIAL_USB         0x0c03
#define PCI_CLASS_SERIAL_FIBER       0x0c04

#define PCI_CLASS_OTHERS             0xff

#define PCI_ADDR_MEM_MASK (~(pciaddr_t) 0xf)
#define PCI_BASE_ADDRESS_0              0x10              /* 32 bits */
#define PCI_BASE_ADDRESS_SPACE          0x01              /* 0 = memory, 1 = I/O */
#define PCI_BASE_ADDRESS_SPACE_IO       0x01
#define PCI_BASE_ADDRESS_SPACE_MEMORY   0x00
#define PCI_BASE_ADDRESS_MEM_TYPE_MASK  0x06
#define PCI_BASE_ADDRESS_MEM_TYPE_32    0x00              /* 32 bit address */
#define PCI_BASE_ADDRESS_MEM_TYPE_1M    0x02              /* Below 1M [obsolete] */
#define PCI_BASE_ADDRESS_MEM_TYPE_64    0x04              /* 64 bit address */
#define PCI_BASE_ADDRESS_MEM_PREFETCH   0x08              /* prefetchable? */
#define PCI_BASE_ADDRESS_MEM_MASK       (~0x0fUL)
#define PCI_BASE_ADDRESS_IO_MASK        (~0x03UL)

#define PCI_SUBSYSTEM_VENDOR_ID      0x2c
#define PCI_SUBSYSTEM_ID             0x2e

#define PCI_CB_SUBSYSTEM_VENDOR_ID   0x40
#define PCI_CB_SUBSYSTEM_ID          0x42

bool pcidb_loaded = false;

typedef unsigned long long pciaddr_t;
typedef enum
{
  pcidevice,
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
  u_int16_t domain;                               /* PCI domain (host bridge) */
  u_int16_t bus;                                  /* Higher byte can select host bridges */
  u_int8_t dev, func;                             /* Device and function */

  u_int16_t vendor_id, device_id;                 /* Identity of the device */
  unsigned int irq;                               /* IRQ number */
  pciaddr_t base_addr[6];                         /* Base addresses */
  pciaddr_t size[6];                              /* Region sizes */
  pciaddr_t rom_base_addr;                        /* Expansion ROM base address */
  pciaddr_t rom_size;                             /* Expansion ROM size */

  u_int8_t config[256];                           /* non-root users can only use first 64 bytes */
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


static bool find_best_match(vector < pci_entry > &list,
pci_entry & result,
long u1 = -1,
long u2 = -1,
long u3 = -1,
long u4 = -1)
{
  int lastmatch = -1;
  unsigned int lastscore = 0;

  for (unsigned int i = 0; i < list.size(); i++)
  {
    unsigned int currentscore = list[i].matches(u1, u2, u3, u4);

    if (currentscore > lastscore)
    {
      lastscore = currentscore;
      lastmatch = i;
    }
  }

  if (lastmatch >= 0)
  {
    result = list[lastmatch];
    return true;
  }

  return false;
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
      return "generic";
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
  unsigned int level = 0;

  memset(u, 0, sizeof(u));

  for (unsigned int i = 0; i < list.size(); i++)
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
          line = line.substr(2);                  // get rid of 'C '

          if ((line.length() < 3) || (line[2] != ' '))
            return false;
          if (sscanf(line.c_str(), "%lx", &u[0]) != 1)
            return false;
          line = line.substr(3);
          line = hw::strip(line);
        }
        else
        {
          current_catalog = pcivendor;

          if ((line.length() < 5) || (line[4] != ' '))
            return false;
          if (sscanf(line.c_str(), "%lx", &u[0]) != 1)
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
          if (sscanf(line.c_str(), "%lx", &u[1]) != 1)
            return false;
          line = line.substr(3);
          line = hw::strip(line);
        }
        else
        {
          current_catalog = pcidevice;

          if ((line.length() < 5) || (line[4] != ' '))
            return false;
          if (sscanf(line.c_str(), "%lx", &u[1]) != 1)
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
          if (sscanf(line.c_str(), "%lx", &u[2]) != 1)
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
          if (sscanf(line.c_str(), "%lx%lx", &u[2], &u[3]) != 2)
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

  return (pci_devices.size() > 0);
}


static string get_class_description(long c,
long pi = -1)
{
  pci_entry result("");

  if (find_best_match(pci_classes, result, c >> 8, c & 0xff, pi))
    return result.description;
  else
    return "";
}


static string get_device_description(long u1,
long u2 = -1,
long u3 = -1,
long u4 = -1)
{
  pci_entry result("");

  if (find_best_match(pci_devices, result, u1, u2, u3, u4))
    return result.description;
  else
    return "";
}


static u_int32_t get_conf_long(struct pci_dev d,
unsigned int pos)
{
  if (pos > sizeof(d.config))
    return 0;

  return d.config[pos] | (d.config[pos + 1] << 8) |
    (d.config[pos + 2] << 16) | (d.config[pos + 3] << 24);
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


static string pci_bushandle(u_int8_t bus, u_int16_t domain = 0)
{
  char buffer[20];

  if(domain == (u_int16_t)(-1))
    snprintf(buffer, sizeof(buffer), "%02x", bus);
  else
    snprintf(buffer, sizeof(buffer), "%04x:%02x", domain, bus);

  return "PCIBUS:" + string(buffer);
}


static string pci_handle(u_int16_t bus,
u_int8_t dev,
u_int8_t fct,
u_int16_t domain = 0)
{
  char buffer[30];

  if(domain == (u_int16_t)(-1))
    snprintf(buffer, sizeof(buffer), "PCI:%02x:%02x.%x", bus, dev, fct);
  else
    snprintf(buffer, sizeof(buffer), "PCI:%04x:%02x:%02x.%x", domain, bus, dev, fct);

  return string(buffer);
}


static bool scan_resources(hwNode & n,
struct pci_dev &d)
{
  u_int16_t cmd = get_conf_word(d, PCI_COMMAND);

  n.setWidth(32);

  for (int i = 0; i < 6; i++)
  {
    u_int32_t flg = get_conf_long(d, PCI_BASE_ADDRESS_0 + 4 * i);
    u_int32_t pos = d.base_addr[i];
    u_int32_t len = d.size[i];

    if (flg == 0xffffffff)
      flg = 0;

    if (!pos && !flg && !len)
      continue;

    if (pos && !flg)                              /* Reported by the OS, but not by the device */
    {
//printf("[virtual] ");
      flg = pos;
    }
    if (flg & PCI_BASE_ADDRESS_SPACE_IO)
    {
      u_int32_t a = pos & PCI_BASE_ADDRESS_IO_MASK;
      if ((a != 0) && (cmd & PCI_COMMAND_IO) != 0)
        n.addResource(hw::resource::ioport(a, a + len - 1));
    }
    else                                          // resource is memory
    {
      int t = flg & PCI_BASE_ADDRESS_MEM_TYPE_MASK;
      u_int64_t a = pos & PCI_ADDR_MEM_MASK;
      u_int64_t z = 0;

      if (t == PCI_BASE_ADDRESS_MEM_TYPE_64)
      {
        n.setWidth(64);
        if (i < 5)
        {
          i++;
          z = get_conf_long(d, PCI_BASE_ADDRESS_0 + 4 * i);
          a += z << 4;
        }
      }
      if (a)
        n.addResource(hw::resource::iomem(a, a + len - 1));
    }
  }

  return true;
}

static bool scan_capabilities(hwNode & n, struct pci_dev &d)
{
  unsigned int where = get_conf_byte(d, PCI_CAPABILITY_LIST) & ~3;
  string buffer;
  unsigned int ttl = PCI_FIND_CAP_TTL;

  while(where && ttl--)
  {
    unsigned int id, next, cap;

    id = get_conf_byte(d, where + PCI_CAP_LIST_ID);
    next = get_conf_byte(d, where + PCI_CAP_LIST_NEXT) & ~3;
    cap = get_conf_word(d, where + PCI_CAP_FLAGS);

    if(!id || id == 0xff)
      return false;

    switch(id)
    {
      case PCI_CAP_ID_PM:
        n.addCapability("pm", "Power Management");
        break;
      case PCI_CAP_ID_AGP:
        n.addCapability("agp", "AGP");
        buffer = hw::asString((cap >> 4) & 0x0f) + "." + hw::asString(cap & 0x0f);
        n.addCapability("agp-"+buffer, "AGP "+buffer);
        break;
      case PCI_CAP_ID_VPD:
        n.addCapability("vpd", "Vital Product Data");
        break;
      case PCI_CAP_ID_SLOTID:
        n.addCapability("slotid", "Slot Identification");
        n.setSlot(hw::asString(cap & PCI_SID_ESR_NSLOTS)+", chassis "+hw::asString(cap>>8));
        break;
      case PCI_CAP_ID_MSI:
        n.addCapability("msi", "Message Signalled Interrupts");
        break;
      case PCI_CAP_ID_CHSWP:
        n.addCapability("hotswap", "Hot-swap");
        break;
      case PCI_CAP_ID_PCIX:
        n.addCapability("pcix", "PCI-X");
        break;
      case PCI_CAP_ID_HT:
        n.addCapability("ht", "HyperTransport");
        break;
      case PCI_CAP_ID_DBG:
        n.addCapability("debug", "Debug port");
        break;
      case PCI_CAP_ID_CCRC:
        n.addCapability("ccrc", "CompactPCI Central Resource Control");
        break;
      case PCI_CAP_ID_AGP3:
        n.addCapability("agp8x", "AGP 8x");
        break;
      case PCI_CAP_ID_EXP:
        n.addCapability("pciexpress", _("PCI Express"));
        break;
      case PCI_CAP_ID_MSIX:
        n.addCapability("msix", "MSI-X");
        break;
    }

    where = next;
  }

  return true;
}


static void addHints(hwNode & n,
long _vendor,
long _device,
long _subvendor,
long _subdevice,
long _class)
{
  n.addHint("pci.vendor", _vendor);
  n.addHint("pci.device", _device);
  if(_subvendor && (_subvendor != 0xffff))
  {
    n.addHint("pci.subvendor", _subvendor);
    n.addHint("pci.subdevice", _subdevice);
  }
  n.addHint("pci.class", _class);
}

static hwNode *scan_pci_dev(struct pci_dev &d, hwNode & n)
{
  hwNode *result = NULL;
  hwNode *core = n.getChild("core");
  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  if(!pcidb_loaded)
    pcidb_loaded = load_pcidb();

      d.vendor_id = get_conf_word(d, PCI_VENDOR_ID);
      d.device_id = get_conf_word(d, PCI_DEVICE_ID);
      u_int16_t dclass = get_conf_word(d, PCI_CLASS_DEVICE);
      u_int16_t cmd = get_conf_word(d, PCI_COMMAND);
      u_int16_t status = get_conf_word(d, PCI_STATUS);
      u_int8_t latency = get_conf_byte(d, PCI_LATENCY_TIMER);
      u_int8_t min_gnt = get_conf_byte(d, PCI_MIN_GNT);
      u_int8_t max_lat = get_conf_byte(d, PCI_MAX_LAT);
      u_int8_t progif = get_conf_byte(d, PCI_CLASS_PROG);
      u_int8_t rev = get_conf_byte(d, PCI_REVISION_ID);
      u_int8_t htype = get_conf_byte(d, PCI_HEADER_TYPE) & 0x7f;
      u_int16_t subsys_v = 0, subsys_d = 0;

      char revision[10];
      snprintf(revision, sizeof(revision), "%02x", rev);
      string moredescription = get_class_description(dclass, progif);

      switch (htype)
      {
        case PCI_HEADER_TYPE_NORMAL:
          subsys_v = get_conf_word(d, PCI_SUBSYSTEM_VENDOR_ID);
          subsys_d = get_conf_word(d, PCI_SUBSYSTEM_ID);
          break;
        case PCI_HEADER_TYPE_BRIDGE:
          subsys_v = subsys_d = 0;
          latency = get_conf_byte(d, PCI_SEC_LATENCY_TIMER);
          break;
        case PCI_HEADER_TYPE_CARDBUS:
          subsys_v = get_conf_word(d, PCI_CB_SUBSYSTEM_VENDOR_ID);
          subsys_d = get_conf_word(d, PCI_CB_SUBSYSTEM_ID);
          latency = get_conf_byte(d, PCI_CB_LATENCY_TIMER);
          break;
      }

      if (dclass == PCI_CLASS_BRIDGE_HOST)
      {
        hwNode host("pci",
          hw::bridge);

        host.setDescription(get_class_description(dclass, progif));
        host.setVendor(get_device_description(d.vendor_id)+(enabled("output:numeric")?" ["+tohex(d.vendor_id)+"]":""));
        host.setProduct(get_device_description(d.vendor_id, d.device_id)+(enabled("output:numeric")?" ["+tohex(d.vendor_id)+":"+tohex(d.device_id)+"]":""));
        host.setHandle(pci_bushandle(d.bus, d.domain));
        host.setVersion(revision);
        addHints(host, d.vendor_id, d.device_id, subsys_v, subsys_d, dclass);
        host.claim();
        if(latency)
          host.setConfig("latency", latency);
        if (d.size[0] > 0)
          host.setPhysId(0x100 + d.domain);

        if (moredescription != "" && moredescription != host.getDescription())
        {
          host.addCapability(moredescription);
          host.setDescription(host.getDescription() + " (" +
            moredescription + ")");
        }

        if (status & PCI_STATUS_66MHZ)
          host.setClock(66000000UL);              // 66MHz
        else
          host.setClock(33000000UL);              // 33MHz

        scan_resources(host, d);

        if (core)
          result = core->addChild(host);
        else
          result = n.addChild(host);
      }
      else
      {
        hw::hwClass deviceclass = hw::generic;
        string devicename = "generic";
        string deviceicon = "";

        switch (dclass >> 8)
        {
          case PCI_BASE_CLASS_STORAGE:
            deviceclass = hw::storage;
            deviceicon = "disc";
            if(dclass == PCI_CLASS_STORAGE_SCSI)
              deviceicon = "scsi";
            if(dclass == PCI_CLASS_STORAGE_RAID)
              deviceicon = "raid";
            break;
          case PCI_BASE_CLASS_NETWORK:
            deviceclass = hw::network;
            deviceicon = "network";
            break;
          case PCI_BASE_CLASS_MEMORY:
            deviceclass = hw::memory;
            deviceicon = "memory";
            break;
          case PCI_BASE_CLASS_BRIDGE:
            deviceclass = hw::bridge;
            break;
          case PCI_BASE_CLASS_MULTIMEDIA:
            deviceclass = hw::multimedia;
            if(dclass == PCI_CLASS_MULTIMEDIA_AUDIO)
              deviceicon = "audio";
            break;
          case PCI_BASE_CLASS_DISPLAY:
            deviceclass = hw::display;
            deviceicon = "display";
            break;
          case PCI_BASE_CLASS_COMMUNICATION:
            deviceclass = hw::communication;
            if(dclass == PCI_CLASS_COMMUNICATION_SERIAL)
              deviceicon = "serial";
            if(dclass == PCI_CLASS_COMMUNICATION_PARALLEL)
              deviceicon = "parallel";
            if(dclass == PCI_CLASS_COMMUNICATION_MODEM)
              deviceicon = "modem";
            break;
          case PCI_BASE_CLASS_SYSTEM:
            deviceclass = hw::generic;
            break;
          case PCI_BASE_CLASS_INPUT:
            deviceclass = hw::input;
            break;
          case PCI_BASE_CLASS_PROCESSOR:
            deviceclass = hw::processor;
            break;
          case PCI_BASE_CLASS_SERIAL:
            deviceclass = hw::bus;
            if(dclass == PCI_CLASS_SERIAL_USB)
              deviceicon = "usb";
            if(dclass == PCI_CLASS_SERIAL_FIREWIRE)
              deviceicon = "firewire";
            break;
        }

        devicename = get_class_name(dclass);
        hwNode *device = new hwNode(devicename, deviceclass);

        if (device)
        {
          if(deviceicon != "") device->addHint("icon", deviceicon);
          addHints(*device, d.vendor_id, d.device_id, subsys_v, subsys_d, dclass);

          if (deviceclass == hw::bridge || deviceclass == hw::storage)
            device->addCapability(devicename);

          if(device->isCapable("isa") ||
            device->isCapable("pci") ||
            device->isCapable("agp"))
            device->claim();

          scan_resources(*device, d);
          scan_capabilities(*device, d);

          if (deviceclass == hw::display)
            for (int j = 0; j < 6; j++)
              if ((d.size[j] != 0xffffffff)
            && (d.size[j] > device->getSize()))
                device->setSize(d.size[j]);

          if (dclass == PCI_CLASS_BRIDGE_PCI)
          {
            device->setHandle(pci_bushandle(get_conf_byte(d, PCI_SECONDARY_BUS), d.domain));
            device->claim();
          }
          else
          {
            char irq[10];

            snprintf(irq, sizeof(irq), "%d", d.irq);
            device->setHandle(pci_handle(d.bus, d.dev, d.func, d.domain));
            device->setConfig("latency", latency);
            if(max_lat)
              device->setConfig("maxlatency", max_lat);
            if(min_gnt)
              device->setConfig("mingnt", min_gnt);
            if (d.irq != 0)
            {
//device->setConfig("irq", irq);
              device->addResource(hw::resource::irq(d.irq));
            }
          }
          device->setDescription(get_class_description(dclass));

          if (moredescription != ""
            && moredescription != device->getDescription())
          {
            device->addCapability(moredescription);
          }
          device->setVendor(get_device_description(d.vendor_id)+(enabled("output:numeric")?" ["+tohex(d.vendor_id)+"]":""));
          device->setVersion(revision);
          device->setProduct(get_device_description(d.vendor_id, d.device_id)+(enabled("output:numeric")?" ["+tohex(d.vendor_id)+":"+tohex(d.device_id)+"]":""));

          if (cmd & PCI_COMMAND_MASTER)
            device->addCapability("bus master", "bus mastering");
          if (cmd & PCI_COMMAND_VGA_PALETTE)
            device->addCapability("VGA palette", "VGA palette");
          if (status & PCI_STATUS_CAP_LIST)
            device->addCapability("cap list", "PCI capabilities listing");
          if (status & PCI_STATUS_66MHZ)
            device->setClock(66000000UL);         // 66MHz
          else
            device->setClock(33000000UL);         // 33MHz

          device->setPhysId(d.dev, d.func);

          hwNode *bus = NULL;

          bus = n.findChildByHandle(pci_bushandle(d.bus, d.domain));

          device->describeCapability("vga", "VGA graphical framebuffer");
          device->describeCapability("pcmcia", "PC-Card (PCMCIA)");
          device->describeCapability("generic", "Generic interface");
          device->describeCapability("ohci", "Open Host Controller Interface");
          device->describeCapability("uhci", "Universal Host Controller Interface (USB1)");
          device->describeCapability("ehci", "Enhanced Host Controller Interface (USB2)");
          if (bus)
            result = bus->addChild(*device);
          else
          {
            if (core)
              result = core->addChild(*device);
            else
              result = n.addChild(*device);
          }
          delete device;

        }
      }
  return result;
}

bool scan_pci_legacy(hwNode & n)
{
  FILE *f;
  hwNode *core = n.getChild("core");
  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  if(!pcidb_loaded)
    pcidb_loaded = load_pcidb();

  f = fopen(PROC_BUS_PCI "/devices", "r");
  if (f)
  {
    char buf[512];

    while (fgets(buf, sizeof(buf) - 1, f))
    {
      unsigned int dfn, vend, cnt;
      struct pci_dev d;
      int fd = -1;
      string devicepath = "";
      char devicename[20];
      char businfo[20];
      char driver[50];
      hwNode *device = NULL;

      memset(&d, 0, sizeof(d));
      memset(driver, 0, sizeof(driver));
      cnt = sscanf(buf,
        "%x %x %x %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %llx %[ -z]s",
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
      snprintf(businfo, sizeof(businfo), "%02x:%02x.%x", d.bus, d.dev,
        d.func);

      fd = open(devicepath.c_str(), O_RDONLY);
      if (fd >= 0)
      {
        if(read(fd, d.config, sizeof(d.config)) != sizeof(d.config))
          memset(&d.config, 0, sizeof(d.config));
        close(fd);
      }

      device = scan_pci_dev(d, n);
      if(device)
      {
        device->setBusInfo(businfo);
      }

    }
    fclose(f);
  }

  return false;
}

bool scan_pci(hwNode & n)
{
  bool result = false;
  dirent **devices = NULL;
  int count = 0;
  hwNode *core = n.getChild("core");

  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  pcidb_loaded = load_pcidb();

  if(!pushd(SYS_BUS_PCI"/devices"))
    return false;
  count = scandir(".", &devices, selectlink, alphasort);
  if(count>=0)
  {
    int i = 0;
    for(i=0; i<count; i++)
    if(matches(devices[i]->d_name, "^[[:xdigit:]]+:[[:xdigit:]]+:[[:xdigit:]]+\\.[[:xdigit:]]+$"))
    {
      string devicepath = string(devices[i]->d_name)+"/config";
      struct pci_dev d;
      int fd = open(devicepath.c_str(), O_RDONLY);
      if (fd >= 0)
      {
        memset(&d, 0, sizeof(d));
        if(read(fd, d.config, 64) == 64)
        {
          if(read(fd, d.config+64, sizeof(d.config)-64) != sizeof(d.config)-64)
            memset(d.config+64, 0, sizeof(d.config)-64);
        }
        close(fd);
      }

      sscanf(devices[i]->d_name, "%hx:%hx:%hhx.%hhx", &d.domain, &d.bus, &d.dev, &d.func);
      hwNode *device = scan_pci_dev(d, n);

      if(device)
      {
        string resourcename = string(devices[i]->d_name)+"/resource";

        device->setBusInfo(devices[i]->d_name);
        if(exists(string(devices[i]->d_name)+"/driver"))
        {
          string drivername = readlink(string(devices[i]->d_name)+"/driver");
          string modulename = readlink(string(devices[i]->d_name)+"/driver/module");

          device->setConfig("driver", basename(const_cast<char *>(drivername.c_str())));
          if(exists(modulename))
            device->setConfig("module", basename(const_cast<char *>(modulename.c_str())));

          if(exists(string(devices[i]->d_name)+"/rom"))
          {
            device->addCapability("rom", "extension ROM");
          }

          if(exists(string(devices[i]->d_name)+"/irq"))
          {
            long irq = get_number(string(devices[i]->d_name)+"/irq", -1);
            if(irq>=0)
              device->addResource(hw::resource::irq(irq));
          }
          device->claim();
        }

	device->setModalias(sysfs::entry::byBus("pci", devices[i]->d_name).modalias());

        if(exists(resourcename))
        {
            FILE*resource = fopen(resourcename.c_str(), "r");

            if(resource)
            {
              while(!feof(resource))
              {
                unsigned long long start, end, flags;

                start = end = flags = 0;

                if(fscanf(resource, "%llx %llx %llx", &start, &end, &flags) != 3)
                  break;

                if(flags & 0x101)
                  device->addResource(hw::resource::ioport(start, end));
                else
                if(flags & 0x100)
                  device->addResource(hw::resource::iomem(start, end));
                else
                if(flags & 0x200)
                  device->addResource(hw::resource::mem(start, end, flags & 0x1000));
              }
              fclose(resource);
            }
        }
	add_device_tree_info(*device, devices[i]->d_name);

        result = true;
      }

      free(devices[i]);
    }

    free(devices);
  }
  popd();
  return result;
}
