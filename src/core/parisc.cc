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

#include "version.h"
#include "device-tree.h"
#include "osutils.h"
#include "heuristics.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

__ID("@(#) $Id$");

#define DEVICESPARISC "/sys/devices/parisc"

#define TP_NPROC  0x00
#define TP_MEMORY 0x01
#define TP_B_DMA  0x02
#define TP_OBSOLETE 0x03
#define TP_A_DMA  0x04
#define TP_A_DIRECT 0x05
#define TP_OTHER  0x06
#define TP_BCPORT 0x07
#define TP_CIO    0x08
#define TP_CONSOLE  0x09
#define TP_FIO    0x0a
#define TP_BA   0x0b
#define TP_IOA    0x0c
#define TP_BRIDGE 0x0d
#define TP_FABRIC 0x0e
#define TP_MC   0x0f
#define TP_FAULTY 0x1f

struct parisc_device
{
  long sversion;
  const char * id;
  hw::hwClass device_class;
  const char * description;
};

static struct parisc_device parisc_device_list[] =
{
  {0x00004, "cpu", hw::processor, "Processor"},
  {0x0000d, "mux", hw::communication, "MUX"},
  {0x0000e, "serial", hw::communication, "RS-232 Serial Interface"},
  {0x0000f, "display", hw::display, "Graphics Display"},
  {0x00014, "input", hw::input, "HIL"},
  {0x00015, "display", hw::display, "Graphics Display"},
  {0x0003a, "printer", hw::printer, "Centronics Parallel interface"},
  {0x000a8, "input", hw::input, "Keyboard"},
  {0x00039, "scsi", hw::storage, "Core SCSI controller"},
  {0x0003b, "scsi", hw::storage, "FW-SCSI controller"},
  {0x0005e, "network", hw::network, "Token Ring Interface"},
  {0x00089, "scsi", hw::storage, "FW-SCSI controller"},
  {0x00091, "fc", hw::storage, "Fibre Channel controller"},
  {0x0009a, "network", hw::network, "ATM Interface"},
  {0x000a7, "fc", hw::storage, "Fibre Channel controller"},
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
  {0x00071, "scsi", hw::storage, "Core SCSI controller"},
  {0x00072, "network", hw::network, "Core Ethernet Interface"},
  {0x00072, "input", hw::input, "Core HIL"},
  {0x00074, "printer", hw::printer, "Core Centronics Parallel interface"},
  {0x00075, "serial", hw::communication, "Core RS-232 Serial Interface"},
  {0x00077, "display", hw::display, "Graphics Display"},
  {0x0007a, "audio", hw::multimedia, "Audio"},
  {0x0007b, "audio", hw::multimedia, "Audio"},
  {0x0007c, "scsi", hw::storage, "FW-SCSI controller"},
  {0x0007d, "network", hw::network, "FDDI Interface"},
  {0x0007e, "audio", hw::multimedia, "Audio"},
  {0x0007f, "audio", hw::multimedia, "Audio"},
  {0x00082, "scsi", hw::storage, "SCSI controller"},
  {0x00083, "floppy", hw::storage, "Floppy"},
  {0x00084, "input", hw::input, "PS/2 port"},
  {0x00085, "display", hw::display, "Graphics Display"},
  {0x00086, "network", hw::network, "Token Ring Interface"},
  {0x00087, "communication", hw::communication, "ISDN"},
  {0x00088, "network", hw::network, "VME Networking Interface"},
  {0x0008a, "network", hw::network, "Core Ethernet Interface"},
  {0x0008c, "serial", hw::communication, "RS-232 Serial Interface"},
  {0x0008d, "unknown", hw::communication, "RJ-16"},
  {0x0008f, "firmware", hw::memory, "Boot ROM"},
  {0x00096, "input", hw::input, "PS/2 port"},
  {0x00097, "network", hw::network, "100VG LAN Interface"},
  {0x000a2, "network", hw::network, "10/100BT LAN Interface"},
  {0x000a3, "scsi", hw::storage, "Ultra2 SCSI controller"},
  {0x00000, "generic", hw::generic, NULL},
};

enum cpu_type
{
  pcx     = 0,                                    /* pa7000          pa 1.0  */
  pcxs    = 1,                                    /* pa7000          pa 1.1a */
  pcxt    = 2,                                    /* pa7100          pa 1.1b */
  pcxt_   = 3,                                    /* pa7200  (t')    pa 1.1c */
  pcxl    = 4,                                    /* pa7100lc        pa 1.1d */
  pcxl2   = 5,                                    /* pa7300lc        pa 1.1e */
  pcxu    = 6,                                    /* pa8000          pa 2.0  */
  pcxu_   = 7,                                    /* pa8200  (u+)    pa 2.0  */
  pcxw    = 8,                                    /* pa8500          pa 2.0  */
  pcxw_   = 9,                                    /* pa8600  (w+)    pa 2.0  */
  pcxw2   = 10,                                   /* pa8700         pa 2.0  */
  mako    = 11                                    /* pa8800         pa 2.0  */
};

static struct hp_cpu_type_mask
{
  unsigned short model;
  unsigned short mask;
  enum cpu_type cpu;
}


hp_cpu_type_mask_list[] =
{

  {                                               /* 0x0000 - 0x000f */
    0x0000, 0x0ff0, pcx
  },
  {                                               /* 0x0040 - 0x004f */
    0x0048, 0x0ff0, pcxl
  },
  {                                               /* 0x0080 - 0x008f */
    0x0080, 0x0ff0, pcx
  },
  {                                               /* 0x0100 - 0x010f */
    0x0100, 0x0ff0, pcx
  },
  {                                               /* 0x0182 - 0x0183 */
    0x0182, 0x0ffe, pcx
  },
  {                                               /* 0x0182 - 0x0183 */
    0x0182, 0x0ffe, pcxt
  },
  {                                               /* 0x0184 - 0x0184 */
    0x0184, 0x0fff, pcxu
  },
  {                                               /* 0x0200 - 0x0201 */
    0x0200, 0x0ffe, pcxs
  },
  {                                               /* 0x0202 - 0x0202 */
    0x0202, 0x0fff, pcxs
  },
  {                                               /* 0x0203 - 0x0203 */
    0x0203, 0x0fff, pcxt
  },
  {                                               /* 0x0204 - 0x0207 */
    0x0204, 0x0ffc, pcxt
  },
  {                                               /* 0x0280 - 0x0283 */
    0x0280, 0x0ffc, pcxs
  },
  {                                               /* 0x0284 - 0x0287 */
    0x0284, 0x0ffc, pcxt
  },
  {                                               /* 0x0288 - 0x0288 */
    0x0288, 0x0fff, pcxt
  },
  {                                               /* 0x0300 - 0x0303 */
    0x0300, 0x0ffc, pcxs
  },
  {                                               /* 0x0310 - 0x031f */
    0x0310, 0x0ff0, pcxt
  },
  {                                               /* 0x0320 - 0x032f */
    0x0320, 0x0ff0, pcxt
  },
  {                                               /* 0x0400 - 0x040f */
    0x0400, 0x0ff0, pcxt
  },
  {                                               /* 0x0480 - 0x048f */
    0x0480, 0x0ff0, pcxl
  },
  {                                               /* 0x0500 - 0x050f */
    0x0500, 0x0ff0, pcxl2
  },
  {                                               /* 0x0510 - 0x051f */
    0x0510, 0x0ff0, pcxl2
  },
  {                                               /* 0x0580 - 0x0587 */
    0x0580, 0x0ff8, pcxt_
  },
  {                                               /* 0x0588 - 0x058b */
    0x0588, 0x0ffc, pcxt_
  },
  {                                               /* 0x058c - 0x058d */
    0x058c, 0x0ffe, pcxt_
  },
  {                                               /* 0x058e - 0x058e */
    0x058e, 0x0fff, pcxt_
  },
  {                                               /* 0x058f - 0x058f */
    0x058f, 0x0fff, pcxu
  },
  {                                               /* 0x0590 - 0x0591 */
    0x0590, 0x0ffe, pcxu
  },
  {                                               /* 0x0592 - 0x0592 */
    0x0592, 0x0fff, pcxt_
  },
  {                                               /* 0x0593 - 0x0593 */
    0x0593, 0x0fff, pcxu
  },
  {                                               /* 0x0594 - 0x0597 */
    0x0594, 0x0ffc, pcxu
  },
  {                                               /* 0x0598 - 0x0599 */
    0x0598, 0x0ffe, pcxu_
  },
  {                                               /* 0x059a - 0x059b */
    0x059a, 0x0ffe, pcxu
  },
  {                                               /* 0x059c - 0x059c */
    0x059c, 0x0fff, pcxu
  },
  {                                               /* 0x059d - 0x059d */
    0x059d, 0x0fff, pcxu_
  },
  {                                               /* 0x059e - 0x059e */
    0x059e, 0x0fff, pcxt_
  },
  {                                               /* 0x059f - 0x059f */
    0x059f, 0x0fff, pcxu
  },
  {                                               /* 0x05a0 - 0x05a1 */
    0x05a0, 0x0ffe, pcxt_
  },
  {                                               /* 0x05a2 - 0x05a3 */
    0x05a2, 0x0ffe, pcxu
  },
  {                                               /* 0x05a4 - 0x05a7 */
    0x05a4, 0x0ffc, pcxu
  },
  {                                               /* 0x05a8 - 0x05ab */
    0x05a8, 0x0ffc, pcxu
  },
  {                                               /* 0x05ad - 0x05ad */
    0x05ad, 0x0fff, pcxu_
  },
  {                                               /* 0x05ae - 0x05af */
    0x05ae, 0x0ffe, pcxu_
  },
  {                                               /* 0x05b0 - 0x05b1 */
    0x05b0, 0x0ffe, pcxu_
  },
  {                                               /* 0x05b2 - 0x05b2 */
    0x05b2, 0x0fff, pcxu_
  },
  {                                               /* 0x05b3 - 0x05b3 */
    0x05b3, 0x0fff, pcxu
  },
  {                                               /* 0x05b4 - 0x05b4 */
    0x05b4, 0x0fff, pcxw
  },
  {                                               /* 0x05b5 - 0x05b5 */
    0x05b5, 0x0fff, pcxu_
  },
  {                                               /* 0x05b6 - 0x05b7 */
    0x05b6, 0x0ffe, pcxu_
  },
  {                                               /* 0x05b8 - 0x05b9 */
    0x05b8, 0x0ffe, pcxu_
  },
  {                                               /* 0x05ba - 0x05ba */
    0x05ba, 0x0fff, pcxu_
  },
  {                                               /* 0x05bb - 0x05bb */
    0x05bb, 0x0fff, pcxw
  },
  {                                               /* 0x05bc - 0x05bf */
    0x05bc, 0x0ffc, pcxw
  },
  {                                               /* 0x05c0 - 0x05c3 */
    0x05c0, 0x0ffc, pcxw
  },
  {                                               /* 0x05c4 - 0x05c5 */
    0x05c4, 0x0ffe, pcxw
  },
  {                                               /* 0x05c6 - 0x05c6 */
    0x05c6, 0x0fff, pcxw
  },
  {                                               /* 0x05c7 - 0x05c7 */
    0x05c7, 0x0fff, pcxw_
  },
  {                                               /* 0x05c8 - 0x05cb */
    0x05c8, 0x0ffc, pcxw
  },
  {                                               /* 0x05cc - 0x05cd */
    0x05cc, 0x0ffe, pcxw
  },
  {                                               /* 0x05ce - 0x05cf */
    0x05ce, 0x0ffe, pcxw_
  },
  {                                               /* 0x05d0 - 0x05d3 */
    0x05d0, 0x0ffc, pcxw_
  },
  {                                               /* 0x05d4 - 0x05d5 */
    0x05d4, 0x0ffe, pcxw_
  },
  {                                               /* 0x05d6 - 0x05d6 */
    0x05d6, 0x0fff, pcxw
  },
  {                                               /* 0x05d7 - 0x05d7 */
    0x05d7, 0x0fff, pcxw_
  },
  {                                               /* 0x05d8 - 0x05db */
    0x05d8, 0x0ffc, pcxw_
  },
  {                                               /* 0x05dc - 0x05dd */
    0x05dc, 0x0ffe, pcxw2
  },
  {                                               /* 0x05de - 0x05de */
    0x05de, 0x0fff, pcxw_
  },
  {                                               /* 0x05df - 0x05df */
    0x05df, 0x0fff, pcxw2
  },
  {                                               /* 0x05e0 - 0x05e3 */
    0x05e0, 0x0ffc, pcxw2
  },
  {                                               /* 0x05e4 - 0x05e4 */
    0x05e4, 0x0fff, pcxw2
  },
  {                                               /* 0x05e5 - 0x05e5 */
    0x05e5, 0x0fff, pcxw_
  },
  {                                               /* 0x05e6 - 0x05e7 */
    0x05e6, 0x0ffe, pcxw2
  },
  {                                               /* 0x05e8 - 0x05ef */
    0x05e8, 0x0ff8, pcxw2
  },
  {                                               /* 0x05f0 - 0x05ff */
    0x05f0, 0x0ff0, pcxw2
  },
  {                                               /* 0x0600 - 0x061f */
    0x0600, 0x0fe0, pcxl
  },
  {                                               /* 0x0880 - 0x088f */
    0x0880, 0x0ff0, mako
  },
  {                                               /* terminate table */
    0x0000, 0x0000, pcx
  }
};

static const char *cpu_name_version[][2] =
{
  { "PA7000 (PCX)",     "1.0" },
  { "PA7000 (PCX-S)",   "1.1a" },
  { "PA7100 (PCX-T)",   "1.1b" },
  { "PA7200 (PCX-T')",  "1.1c" },
  { "PA7100LC (PCX-L)", "1.1d" },
  { "PA7300LC (PCX-L2)",        "1.1e" },
  { "PA8000 (PCX-U)",   "2.0" },
  { "PA8200 (PCX-U+)",  "2.0" },
  { "PA8500 (PCX-W)",   "2.0" },
  { "PA8600 (PCX-W+)",  "2.0" },
  { "PA8700 (PCX-W2)",  "2.0" },
  { "PA8800 (Mako)",    "2.0" }
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


enum cpu_type parisc_get_cpu_type(long hversion)
{
  struct hp_cpu_type_mask *ptr;

  for (ptr = hp_cpu_type_mask_list; 0 != ptr->mask; ptr++)
  {
    if (ptr->model == (hversion & ptr->mask))
      return ptr->cpu;
  }

  return pcx;                                     /* not reached: */
}


static int currentcpu = 0;

static hwNode get_device(long hw_type, long sversion, long hversion)
{
  hwNode newnode("generic");

  for(unsigned i=0; parisc_device_list[i].description!=NULL; i++)
  {
    if(sversion == parisc_device_list[i].sversion)
    {
      newnode = hwNode(parisc_device_list[i].id, parisc_device_list[i].device_class);
      newnode.setDescription(parisc_device_list[i].description);
      if(hw_type == TP_NPROC)
      {
        enum cpu_type cpu = parisc_get_cpu_type(hversion);
        newnode.setBusInfo(cpubusinfo(currentcpu++));
        newnode.setProduct(cpu_name_version[cpu][0]);
        newnode.setVersion(cpu_name_version[cpu][1]);
      }

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
    hwNode newnode = get_device(hw_type, sversion, hversion);

    if((rev>0) && (newnode.getVersion() == ""))
    {
      char buffer[20];
      snprintf(buffer, sizeof(buffer), "%ld", rev);
      newnode.setVersion(buffer);
    }
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

  if(core->getDescription()=="")
    core->setDescription("Motherboard");
  pushd(DEVICESPARISC);
  scan_device(*core);
  popd();

  return true;
}
