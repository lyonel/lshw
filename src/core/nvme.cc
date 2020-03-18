#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "osutils.h"
#include "nvme.h"
#include "partitions.h"

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

    hwNode device(e.name(), hw::storage);
    device.claim();
    device.setLogicalName(e.name());
    device.setDescription("NVMe device");
    device.setProduct(e.string_attr("model"));
    device.setSerial(e.string_attr("serial"));
    device.setVersion(e.string_attr("firmware_rev"));
    device.setConfig("nqn",e.string_attr("subsysnqn"));
    device.setConfig("state",e.string_attr("state"));
    device.setModalias(e.modalias());

    for(unsigned long long i=1; e.hassubdir(e.name()+"n"+tostring(i)); i++) {
	    hwNode ns("namespace", hw::disk);
	    ns.claim();
	    ns.setPhysId(i);
	    ns.setLogicalName(e.name()+"n"+tostring(i));
	    scan_partitions(ns);
	    device.addChild(ns);
    }

    if(hwNode *child = n.findChildByBusInfo(e.leaf().businfo())) {
	    child->addChild(device);
    } else
	    n.addChild(device);
  }

  return true;
}
