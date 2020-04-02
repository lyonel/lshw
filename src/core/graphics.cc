#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "osutils.h"
#include "graphics.h"
#include "heuristics.h"

#include <vector>
#include <iostream>

__ID("@(#) $Id$");

using namespace std;

bool scan_graphics(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_class("graphics");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;
    string dev = e.string_attr("dev");
    if(dev!="")
    {
      hwNode *device = n.findChildByBusInfo(e.leaf().businfo());
      if(!device)
        device = n.addChild(hwNode("graphics", hw::display));
      device->claim();
      device->setLogicalName(e.name());
      device->addCapability("fb", "framebuffer");
      if(device->getProduct() == "") device->setProduct(e.string_attr("name"));
      string resolution = e.string_attr("virtual_size");
      string depth = e.string_attr("bits_per_pixel");
      if(resolution != "") device->setConfig("resolution", resolution);
      if(depth != "") device->setConfig("depth", depth);
    }
  }

  return true;
}
