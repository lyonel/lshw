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
#define TP_A_DMA	0x04
#define TP_A_DIRECT	0x05
#define TP_BCPORT	0x07
#define TP_CIO		0x08
#define TP_CONSOLE	0x09
#define TP_FIO		0x0a
#define TP_BA		0x0b
#define TP_IOA		0x0c
#define TP_BRIDGE	0x0d
#define TP_FABRIC	0x0e
#define TP_FAULTY	0x1f

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
                                                                                
static bool scan_device(hwNode & node, string name = "")
{
  struct dirent **namelist;
  int n;
  hwNode * curnode = NULL;

  if(name != "")
  {
    hwNode newnode("device");
    size_t colon = name.rfind(":");

    switch(get_long("hw_type"))
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
