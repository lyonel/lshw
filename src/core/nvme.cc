#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "osutils.h"
#include "nvme.h"
#include "disk.h"
#include "heuristics.h"

#include <vector>
#include <iostream>

__ID("@(#) $Id$");

using namespace std;

bool scan_nvme(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_class("nvme");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    hwNode *device = n.findChildByBusInfo(e.leaf().businfo());
    if(!device) {
      device = n.addChild(hwNode(e.name(), hw::storage));
    }

    device->claim();
    device->setLogicalName(e.name());
    device->setDescription("NVMe device");
    device->setProduct(e.string_attr("model"));
    device->setSerial(e.string_attr("serial"));
    device->setVersion(e.string_attr("firmware_rev"));
    device->setConfig("nqn",e.string_attr("subsysnqn"));
    device->setConfig("state",e.string_attr("state"));
    device->setModalias(e.modalias());

    vector < sysfs::entry > namespaces = e.devices();
    for(vector < sysfs::entry >::iterator i = namespaces.begin(); i != namespaces.end(); ++i)
    {
      const sysfs::entry & n = *i;

      hwNode ns("namespace", hw::disk);
      ns.claim();
      ns.setBusInfo(guessBusInfo(n.name()));
      ns.setPhysId(n.string_attr("nsid"));
      ns.setDescription("NVMe disk");
      // try to guess correct logical name when native NVMe multipath is enabled for NVMe devices
      if(!exists("/dev/"+n.name()) &&
		      uppercase(hw::strip(get_string("/sys/module/nvme_core/parameters/multipath")))=="Y" &&
		      matches(n.name(), "^nvme[0-9]+c[0-9]+n[0-9]+$")) {
	      size_t indexc = n.name().find("c");
	      size_t indexn = n.name().find("n", indexc);
	      ns.setLogicalName(n.name().erase(indexc, indexn - indexc));
      } else
	      ns.setLogicalName(n.name());
      ns.setConfig("wwid",n.string_attr("wwid"));
      scan_disk(ns);
      device->addChild(ns);
    }
  }

  return true;
}
