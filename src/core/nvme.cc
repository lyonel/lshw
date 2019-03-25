#include "version.h"
#include "disk.h"
#include "osutils.h"
#include "sysfs.h"
#include "hw.h"
#include <glob.h>
#include <libgen.h>

#include <string>

__ID("@(#) $Id$");

#define CLASS_NVME "nvme"
#define NVMEX "/dev/nvme[0-9]*"
#define NVMEXNX "/dev/nvme[0-9]*n[0-9]*"

static void scan_controllers(hwNode & n)
{
  glob_t entries;
  size_t j;

  if(glob(NVMEX, 0, NULL, &entries) == 0)
  {
    for(j=0; j < entries.gl_pathc; j++)
    {
      if(matches(entries.gl_pathv[j], "^/dev/nvme[[:digit:]]+$"))
      {
        string businfo = "";
        string logicalname = "";
        hwNode *device = NULL;

        logicalname = basename(entries.gl_pathv[j]);
        if (logicalname.empty())
          continue;

        businfo = sysfs::entry::byClass(CLASS_NVME, logicalname).businfo();

        if (!businfo.empty())
          device = n.findChildByBusInfo(businfo);

        if (!device)
          device = n.findChildByLogicalName(logicalname);

        if (!device)
        {
          hwNode *core = n.getChild("core");

          if (core)
            device = n.getChild("nvme");

          if (core && !device)
            device = core->addChild(hwNode("nvme", hw::storage));
        }

        if (!device)
          device = n.addChild(hwNode("nvme", hw::storage));

        if (device)
        {
          if(device->getBusInfo().empty())
            device->setBusInfo(businfo);
          device->setLogicalName(logicalname);
          device->claim();
        }
      }
    }

    globfree(&entries);
  }
}

static void scan_namespaces(hwNode & n)
{
  glob_t entries;
  size_t j;

  if(glob(NVMEXNX, 0, NULL, &entries) == 0)
  {
    for(j=0; j < entries.gl_pathc; j++)
    {
      if(matches(entries.gl_pathv[j], "^/dev/nvme[[:digit:]]+n[[:digit:]]+$"))
      {
        // We get this information from sysfs rather than doing an NVMe Identify command
        // so they may not all be available from all kernels.
        string path = entries.gl_pathv[j];
        string logicalname = "";
        string parentlogicalname = "";
        string model;
        string serial;
        string firmware_rev;
        hwNode *parent = NULL;
        hwNode device = hwNode("disk", hw::disk);

        logicalname = basename(entries.gl_pathv[j]);
        if (logicalname.empty())
          continue;

        parentlogicalname = path.substr(0, path.find_last_of("n"));

        sysfs::entry e = sysfs::entry::byClass("block", logicalname);

        model = e.model();
        serial = e.serial();
        firmware_rev = e.firmware_rev();

        device.setDescription("NVMe disk");
        device.setLogicalName(logicalname);
        device.claim();

        if (!model.empty())
          device.setProduct(model);
        if (!serial.empty())
          device.setSerial(serial);
        if (!firmware_rev.empty())
          device.setVersion(firmware_rev);

        scan_disk(device);

        if (!parentlogicalname.empty())
          parent = n.findChildByLogicalName(parentlogicalname);

        if (!parent)
        {
          hwNode *core = n.getChild("core");

          if (core)
            parent = n.getChild("nvme");

          if (core && !parent)
            parent = core->addChild(hwNode("nvme", hw::storage));
        }

        if (!parent)
          parent = n.addChild(hwNode("nvme", hw::storage));

        if (parent)
        {
          parent->addChild(device);
          parent->claim();
        }
      }
    }

    globfree(&entries);
  }

}

bool scan_nvme(hwNode & n)
{
  scan_controllers(n);

  scan_namespaces(n);

  return false;
}
