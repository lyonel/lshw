/*
 * parisc.cc
 *
 * This module parses the PA-RISC device tree (published under /sys
 * by the kernel).
 *
 *
 *
 *
 */

#include "device-tree.h"
#include "osutils.h"
#include "heuristics.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

static char *id =
  "@(#) $Id$";

#define DEVICESPARISC "/sys/devices/parisc"

#define TP_NPROC	0x00
#define TP_MEMORY	0x01
#define TP_B_DMA	0x02
#define TP_OBSOLETE	0x03
#define TP_A_DMA	0x04
#define TP_A_DIRECT	0x05
#define TP_OTHER	0x06
#define TP_BCPORT	0x07
#define TP_CIO		0x08
#define TP_CONSOLE	0x09
#define TP_FIO		0x0a
#define TP_BA		0x0b
#define TP_IOA		0x0c
#define TP_BRIDGE	0x0d
#define TP_FABRIC	0x0e
#define TP_MC		0x0f
#define TP_FAULTY	0x1f

struct parisc_device
{
  long sversion;
  const char * id;
  hw::hwClass device_class;
  const char * description;
};

static struct parisc_device parisc_device_list[] = {
	{0x00004, "cpu", hw::processor, "Processor"},
	{0x0000d, "mux", hw::communication, "MUX"},
	{0x0000e, "serial", hw::communication, "RS-232"},
	{0x0000f, "display", hw::display, "Graphics"},
	{0x00014, "input", hw::input, "HIL"},
	{0x00015, "display", hw::display, "Graphics"},
	{0x0003a, "printer", hw::printer, "Centronics"},
	{0x000a8, "input", hw::input, "Keyboard"},
	{0x00039, "scsi", hw::storage, "Core SCSI"},
	{0x0003b, "scsi", hw::storage, "FW-SCSI"},
	{0x0005e, "network", hw::network, "Token Ring"},
	{0x00089, "scsi", hw::storage, "FW-SCSI"},
	{0x00091, "fc", hw::storage, "Fibre Channel"},
	{0x0009a, "network", hw::network, "ATM"},
	{0x000a7, "fc", hw::storage, "Fibre Channel"},
	{0x00070, "core", hw::bus, "Core Bus"},
	{0x00076, "eisa", hw::bus, "EISA Bus"},
	{0x00078, "vme", hw::bus, "VME Bus"},
	{0x00081, "core", hw::bus, "Core Bus"},
	{0x0008e, "wax", hw::bus, "Wax Bus"},
	{0x00090, "eisa", hw::bus, "Wax EISA Bus"},
	{0x00093, "bus", hw::bus, "TIMI Bus"},
	{0x0000c, "bridge", hw::bridge, "Bus Converter"},
	{0x0000a, "pci", hw::bridge, "PCI Bridge"},
	{0x000a5, "pci", hw::bridge, "PCI Bridge"},
	{0x00052, "lanconsole", hw::network, "LAN/Console"},
	{0x00060, "lanconsole", hw::network, "LAN/Console"},
	{0x00071, "scsi", hw::storage, "Core SCSI"},
	{0x00072, "network", hw::network, "Core Ethernet"},
	{0x00072, "input", hw::input, "Core HIL"},
	{0x00074, "printer", hw::printer, "Core Centronics"},
	{0x00075, "serial", hw::communication, "Core RS-232"},
	{0x00077, "display", hw::display, "Graphics"},
	{0x0007a, "audio", hw::multimedia, "Audio"},
	{0x0007b, "audio", hw::multimedia, "Audio"},
	{0x0007c, "scsi", hw::storage, "FW-SCSI"},
	{0x0007d, "network", hw::network, "FDDI"},
	{0x0007e, "audio", hw::multimedia, "Audio"},
	{0x0007f, "audio", hw::multimedia, "Audio"},
	{0x00082, "scsi", hw::storage, "SCSI"},
	{0x00083, "floppy", hw::storage, "Floppy"},
	{0x00084, "input", hw::input, "PS/2 port"},
	{0x00085, "display", hw::display, "Graphics"},
	{0x00086, "network", hw::network, "Token Ring"},
	{0x00087, "communication", hw::communication, "ISDN"},
	{0x00088, "network", hw::network, "VME Networking"},
	{0x0008a, "network", hw::network, "Core Ethernet"},
	{0x0008c, "serial", hw::communication, "RS-232"},
	{0x0008d, "unknown", hw::communication, "RJ-16"},
	{0x0008f, "firmware", hw::memory, "Boot ROM"},
	{0x00096, "input", hw::input, "PS/2 port"},
	{0x00097, "network", hw::network, "100VG LAN"},
	{0x000a2, "network", hw::network, "10/100BT LAN"},
	{0x000a3, "scsi", hw::storage, "Ultra2 SCSI"},
	{0x00000, "generic", hw::generic, NULL},
};

static long get_long(const string & path)
{
  long result = -1;
  FILE * in = fopen(path.c_str(), "r");

  if (in)
  {
    if(fscanf(in, "%lx", &result) != 1)
      result = -1;
    fclose(in);
  }

  return result;
}

static string cpubusinfo(int cpu)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "cpu@%d", cpu);

  return string(buffer);
}

static int currentcpu = 0;
                                                                                
static hwNode get_device(long hw_type, long sversion)
{
  hwNode newnode("generic");

  for(unsigned i=0; parisc_device_list[i].description!=NULL; i++)
  {
    if(sversion == parisc_device_list[i].sversion)
    {
      newnode = hwNode(parisc_device_list[i].id, parisc_device_list[i].device_class);
      newnode.setDescription(parisc_device_list[i].description);
      return newnode;
    }
  }

    switch(hw_type)
    {
      case TP_NPROC:
        newnode = hwNode("cpu", hw::processor);
        newnode.setDescription("Processor");
        newnode.setBusInfo(cpubusinfo(currentcpu++));
        break;
      case TP_MEMORY:
        newnode = hwNode("memory", hw::memory);
        newnode.setDescription("Memory");
        break;
      case TP_B_DMA:
        newnode.addCapability("b-dma", "Type B DMA I/O");
        break;
      case TP_A_DMA:
        newnode.addCapability("a-dma", "Type A DMA I/O");
        break;
      case TP_A_DIRECT:
        switch (sversion)
        {
          case 0x0D:
            newnode = hwNode("serial", hw::communication);
            newnode.setDescription("MUX port");
            break;
          case 0x0E:
            newnode = hwNode("serial", hw::communication);
            newnode.setDescription("RS-232 port");
            break;
        }

        newnode.addCapability("a-direct", "Type A Direct I/O");
        break;
      case TP_BCPORT:
        newnode = hwNode("busconverter", hw::bridge);
        newnode.setDescription("Bus converter port");
        break;
      case TP_CIO:
        newnode.setDescription("HP-CIO adapter");
        break;
      case TP_CONSOLE:
        newnode = hwNode("console", hw::input);
        newnode.setDescription("Console");
        break;
      case TP_FIO:
        newnode.setDescription("Foreign I/O module");
        break;
      case TP_BA:
        newnode = hwNode("bus", hw::bus);
        newnode.setDescription("Bus adapter");
        break;
      case TP_IOA:
        newnode.setDescription("I/O adapter");
        break;
      case TP_BRIDGE:
        newnode = hwNode("bridge", hw::bridge);
        newnode.setDescription("Bus bridge to foreign bus");
        break;
      case TP_FABRIC:
        newnode.setDescription("Fabric ASIC");
        break;
      case TP_FAULTY:
        newnode.disable();
        newnode.setDescription("Faulty module");
        break;
    }

  return newnode;
}

static bool scan_device(hwNode & node, string name = "")
{
  struct dirent **namelist;
  int n;
  hwNode * curnode = NULL;

  if(name != "")
  {
    size_t colon = name.rfind(":");
    long hw_type = get_long("hw_type");
    long sversion = get_long("sversion");
    long hversion = get_long("hversion");
    long rev = get_long("rev");
    hwNode newnode = get_device(hw_type, sversion);

    if(newnode.getBusInfo()=="")
      newnode.setBusInfo(guessBusInfo(name));
    if(exists("driver"))
    {
      string driver = readlink("driver");
      size_t slash = driver.rfind("/");
      newnode.setConfig("driver", driver.substr(slash==driver.npos?0:slash+1));
      newnode.claim();
    }
    if(colon!=name.npos)
      newnode.setPhysId(name.substr(colon+1));
    else
      newnode.setPhysId(name);
    curnode = node.addChild(newnode);
  }

  n = scandir(".", &namelist, selectdir, alphasort);
  if (n < 0)
    return false;
  else
  {
    for (int i = 0; i < n; i++)
      if(matches(namelist[i]->d_name, "^[0-9]+(:[0-9]+)*$"))
      {
        pushd(namelist[i]->d_name);
        scan_device(curnode?*curnode:node, namelist[i]->d_name);
        popd();
        free(namelist[i]);
      }
    free(namelist);
  }


  return true;
}

bool scan_parisc(hwNode & node)
{
  hwNode *core = node.getChild("core");

  currentcpu = 0;

  if (!core)
  {
    core = node.addChild(hwNode("core", hw::bus));
  }

  if (!core)
    return false;

  pushd(DEVICESPARISC);
  scan_device(*core);
  popd();

  (void) &id;			// avoid warning "id declared but not used"

  return true;
}
