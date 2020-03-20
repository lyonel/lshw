#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "osutils.h"
#include "mmc.h"
#include "disk.h"
#include "heuristics.h"

#include <vector>
#include <iostream>

__ID("@(#) $Id$");

using namespace std;

static string strip0x(string s)
{
  if(s.length()<2) return s;
  if(s.substr(0, 2) == "0x") return s.substr(2, string::npos);
  return s;
}

static string manufacturer(unsigned long n)
{
  switch(n)
  {
    case 0x0:
            return "";
    case 0x2:
	    return "Kingston";
    case 0x3:
	    return "SanDisk";
    default:
            return "Unknown ("+tostring(n)+")";
  }
}

bool scan_mmc(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_class("mmc_host");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    hwNode *device = n.findChildByBusInfo(e.leaf().businfo());
    if(!device)
      device = n.addChild(hwNode(e.name(), hw::bus));
    else
      device->setClass(hw::bus);

    device->claim();
    device->setLogicalName(e.name());
    device->setDescription("MMC Host");
    device->setModalias(e.modalias());

    vector < sysfs::entry > namespaces = e.devices();
    for(vector < sysfs::entry >::iterator i = namespaces.begin(); i != namespaces.end(); ++i)
    {
      const sysfs::entry & d = *i;

      hwNode card("card");
      card.claim();
      string prod = d.string_attr("name");
      if(prod != "00000") card.setProduct(prod);
      card.setVendor(manufacturer(d.hex_attr("manfid")));
      card.setBusInfo(guessBusInfo(d.name()));
      card.setPhysId(strip0x(d.string_attr("rca")));
      card.setSerial(tostring(d.hex_attr("serial")));
      if(unsigned long hwrev = d.hex_attr("hwrev")) {
	card.setVersion(tostring(hwrev)+"."+tostring(d.hex_attr("fwrev")));
      }
      card.setDate(d.string_attr("date"));
      card.setDescription("SD/MMC Device");
      if(d.string_attr("scr")!="")
      {
	card.setDescription("SD Card");
        card.setClass(hw::disk);
      }

      vector < sysfs::entry > devices = d.devices();
      for(unsigned long i=0;i<devices.size();i++)
      {
        hwNode *subdev;
        if(devices.size()>1)
          subdev = card.addChild(hwNode("device"));
        else
          subdev = &card;

        subdev->setLogicalName(devices[i].name());
        subdev->setModalias(devices[i].modalias());
        subdev->setBusInfo(guessBusInfo(devices[i].name()));
        if(devices[i].hex_attr("removable"))
          subdev->addCapability("removable");
        scan_disk(*subdev);
      }

      device->addChild(card);
    }

  }

  return true;
}
