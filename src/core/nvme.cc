#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "osutils.h"
#include "nvme.h"
#include "disk.h"
#include "heuristics.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

__ID("@(#) $Id$");

using namespace std;

bool scan_nvme(hwNode & n)
{
  struct stat buf;
  string value;
  vector < sysfs::entry > entries = sysfs::entries_by_class("nvme");

  if (entries.empty())
    return false;

  ifstream file("/sys/module/nvme_core/parameters/multipath");
  if (file.is_open()) {
     file >> value;
     file.close();
  }

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

      /*
       * If native nvme multipath is enabled and the node name starts with "nvme",
       * then find the position of the first occurrence of "c" in the node name, and
       * then find the position of the first occurrence of "n" after "c" position.
       * The logical name is then extracted by erasing the characters between the
       * two positions from the node name and appending /dev/ to the beginning. If
       * the logical name exists, then set it as logical name.
       */
      if (value == "Y" && n.name().find("nvme") == 0) {
	      size_t index1 = n.name().find("c");
	      size_t index2 = n.name().find("n", index1);
	      string logical_name = "/dev/" + n.name().erase(index1, index2 - index1);
	      if (stat(logical_name.c_str(), &buf) == 0)
		      ns.setLogicalName(logical_name);
      }
      else
             ns.setLogicalName(n.name());

      ns.setConfig("wwid",n.string_attr("wwid"));
      scan_disk(ns);
      device->addChild(ns);
    }
  }

  return true;
}
