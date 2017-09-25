#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "vio.h"

#include <vector>

__ID("@(#) $Id$");

using namespace std;


bool scan_vio(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_bus("vio");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    if (e.name() == "vio")
      continue; // skip root device

    string name = e.string_attr("name");
    if (name.empty())
      name = e.name();

    hwNode device(name);
    device.setDescription("Virtual I/O device (" + name + ")");

    string businfo = e.businfo();
    if (!businfo.empty())
      device.setBusInfo(businfo);

    string driver = e.driver();
    if (!driver.empty())
      device.setConfig("driver", driver);

    string devicetree_node = e.string_attr("devspec");
    if (!devicetree_node.empty() && devicetree_node[0] == '/')
      device.setLogicalName("/proc/device-tree" + devicetree_node);

    n.addChild(device);
  }

  return true;
}
