#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "disk.h"
#include "virtio.h"

#include <vector>

__ID("@(#) $Id$");

using namespace std;

// driver seems like the only way to guess the class
static hw::hwClass virtio_class(const string & driver)
{
  if (driver == "virtio_net")
    return hw::network;
  if (driver == "virtio_blk")
    return hw::disk;
  return hw::generic;
}

static void scan_virtio_block(hwNode & device, const sysfs::entry & entry)
{
  string devname = entry.name_in_class("block");
  if (devname.empty())
    return;
  device.setLogicalName(devname);
  scan_disk(device);
  device.claim();
}

bool scan_virtio(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_bus("virtio");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    hwNode device(e.name());
    device.setDescription("Virtual I/O device");

    string businfo = e.businfo();
    if (!businfo.empty())
      device.setBusInfo(businfo);

    string driver = e.driver();
    device.setClass(virtio_class(driver));
    if (!driver.empty())
      device.setConfig("driver", driver);

    // virtio_net devices will be claimed during network interface scanning,
    // but we handle virtio_blk devices here because nothing else will
    scan_virtio_block(device, e);

    hwNode *parent = NULL;
    string parent_businfo = e.parent().businfo();
    if (!parent_businfo.empty())
      parent = n.findChildByBusInfo(parent_businfo);
    if (!parent)
      parent = n.getChild("core");
    if (!parent)
      parent = n.addChild(hwNode("core", hw::bus));
    parent->addChild(device);
  }

  return true;
}
