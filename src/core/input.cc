#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "osutils.h"
#include "input.h"
#include "disk.h"
#include "heuristics.h"

#include <vector>
#include <iostream>

__ID("@(#) $Id$");

#define BUS_PCI                 0x01
#define BUS_ISAPNP              0x02
#define BUS_USB                 0x03
#define BUS_HIL                 0x04
#define BUS_BLUETOOTH           0x05
#define BUS_VIRTUAL             0x06

#define BUS_ISA                 0x10
#define BUS_I8042               0x11
#define BUS_XTKBD               0x12
#define BUS_RS232               0x13
#define BUS_GAMEPORT            0x14
#define BUS_PARPORT             0x15
#define BUS_AMIGA               0x16
#define BUS_ADB                 0x17
#define BUS_I2C                 0x18
#define BUS_HOST                0x19
#define BUS_GSC                 0x1A
#define BUS_ATARI               0x1B
#define BUS_SPI                 0x1C
#define BUS_RMI                 0x1D
#define BUS_CEC                 0x1E
#define BUS_INTEL_ISHTP         0x1F

using namespace std;

bool scan_input(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_class("input");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    if(!e.hassubdir("id")) continue;

    hwNode *device = n.findChildByBusInfo(e.leaf().businfo());
    if(!device)
      device = n.addChild(hwNode("input", hw::input));
    else
    {
      if(device->getClass() == hw::generic)
        device->setClass(hw::input);
    }
    device->claim();
    device->setLogicalName("input/"+e.name());
    device->setProduct(e.string_attr("name"));
    device->setModalias(e.modalias());
    switch(e.hex_attr("id/bustype"))
    {
      case BUS_PCI:
	      device->addCapability("pci", "PCI");
	      break;
      case BUS_ISAPNP:
	      device->addCapability("isa", "ISA");
	      device->addCapability("pnp", "PnP");
	      break;
      case BUS_USB:
	      device->addCapability("usb", "USB");
	      break;
      case BUS_HIL:
	      device->addCapability("hil", "HP-HIL");
	      break;
      case BUS_BLUETOOTH:
	      device->addCapability("bt", "Bluetooth");
	      break;
      case BUS_VIRTUAL:
	      device->addCapability("virtual");
	      break;
      case BUS_ISA:
	      device->addCapability("isa", "ISA bus");
	      break;
      case BUS_I8042:
	      device->addCapability("i8042", "i8042 PC AT keyboard controller");
	      break;
      case BUS_XTKBD:
	      device->addCapability("xtkbd", "XT keyboard controller");
	      break;
      case BUS_RS232:
	      device->addCapability("rs232", "RS-232 serial");
	      break;
      case BUS_GAMEPORT:
	      device->addCapability("gameport", "game port");
	      break;
      case BUS_PARPORT:
	      device->addCapability("parport", "parallel port");
	      break;
      case BUS_AMIGA:
	      device->addCapability("amiga", "Amiga bus");
	      break;
      case BUS_ADB:
	      device->addCapability("adb", "Apple Desktop Bus");
	      break;
      case BUS_I2C:
	      device->addCapability("i2c", "I²C bus");
	      break;
      case BUS_HOST:
	      device->addCapability("host");
	      break;
      case BUS_GSC:
	      device->addCapability("gsc", "GSC bus");
	      break;
      case BUS_ATARI:
	      device->addCapability("atari", "Atari bus");
	      break;
      case BUS_SPI:
	      device->addCapability("spi", "SPI");
	      break;
      case BUS_RMI:
	      device->addCapability("rmi", "RMI");
	      break;
      case BUS_CEC:
	      device->addCapability("cec", "CEC");
	      break;
      case BUS_INTEL_ISHTP:
	      device->addCapability("intelishtp", "Intel Integrated Sensor Hub");
	      break;
    }

    vector < sysfs::entry > events = e.devices();
    for(vector < sysfs::entry >::iterator i = events.begin(); i != events.end(); ++i)
    {
      const sysfs::entry & d = *i;
      device->setLogicalName("input/"+d.name());
    }
  }

  return true;
}
