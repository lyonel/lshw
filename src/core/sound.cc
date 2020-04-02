#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "osutils.h"
#include "sound.h"
#include "heuristics.h"

#include <vector>
#include <iostream>

__ID("@(#) $Id$");

using namespace std;

bool scan_sound(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_class("sound");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;
    string id = e.string_attr("id");
    if(id!="")
    {
      hwNode *device = n.findChildByBusInfo(e.leaf().businfo());
      if(!device)
        device = n.addChild(hwNode("sound", hw::multimedia));
      device->claim();
      if(device->getDescription() == "") device->setDescription(id);
      //device->setPhysId(e.hex_attr("number"));
      //device->setBusInfo("sound@"+e.string_attr("number"));
      device->setLogicalName("snd/"+e.name());
      if(device->getProduct() == "") device->setProduct(e.string_attr("name"));
      device->setModalias(e.modalias());

      vector < sysfs::entry > events = e.devices();
      for(vector < sysfs::entry >::iterator i = events.begin(); i != events.end(); ++i)
      {
        const sysfs::entry & d = *i;
	if(d.subsystem() == "sound")
	{
          device->setLogicalName("snd/"+d.name());
	}
      }
    }
  }

  return true;
}
