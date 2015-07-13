#include "hw.h"
#include "sysfs.h"
#include "disk.h"
#include "s390.h"
#include <string.h>

using namespace std;

struct ccw_device_def {
  const char *cutype;
  const char *devtype;
  const char *description;
  hw::hwClass hwClass;
};
struct ccw_device_def ccw_device_defs[] =
{
  {"1403", "", "Line printer", hw::printer},
  {"1731/01", "1732/01", "OSA-Express QDIO channel", hw::network},
  {"1731/02", "1732/02", "OSA-Express intraensemble data network (IEDN) channel", hw::network},
  {"1731/02", "1732/03", "OSA-Express intranode management network (NMN) channel", hw::network},
  {"1731/05", "1732/05", "HiperSockets network", hw::network},
  {"1731/06", "1732/06", "OSA-Express Network Control Program channel", hw::network},
  {"1731", "1732", "Network adapter", hw::network},
  {"1750", "3380", "Direct attached storage device (ECKD mode)", hw::disk},
  {"1750", "3390", "Direct attached storage device (ECKD mode)", hw::disk},
  {"2105", "3380", "Direct attached storage device (ECKD mode)", hw::disk},
  {"2105", "3390", "Direct attached storage device (ECKD mode)", hw::disk},
  {"2107", "3380", "Direct attached storage device (ECKD mode)", hw::disk},
  {"2107", "3390", "Direct attached storage device (ECKD mode)", hw::disk},
  {"2540", "", "Card reader/punch", hw::generic},
  {"3088/01", "", "P/390 LAN channel station card", hw::communication},
  {"3088/08", "", "Channel-to-Channel device", hw::network},
  {"3088/1e", "", "Channel-to-Channel FICON adapter", hw::network},
  {"3088/1f", "", "Channel-to-Channel ESCON adapter", hw::network},
  {"3088/60", "", "LAN channel station OSA-2 card", hw::network},
  {"3174", "", "3174 Establishment Controller", hw::generic},
  {"3215", "", "3215 terminal", hw::communication},
  {"3270", "", "3270 terminal", hw::communication},
  {"3271", "", "3270 terminal", hw::communication},
  {"3272", "", "3270 terminal", hw::communication},
  {"3273", "", "3270 terminal", hw::communication},
  {"3274", "", "3270 terminal", hw::communication},
  {"3275", "", "3270 terminal", hw::communication},
  {"3276", "", "3270 terminal", hw::communication},
  {"3277", "", "3270 terminal", hw::communication},
  {"3278", "", "3270 terminal", hw::communication},
  {"3279", "", "3270 terminal", hw::communication},
  {"3480", "3480", "3480 tape drive", hw::storage},
  {"3490", "3490", "3490 tape drive", hw::storage},
  {"3590", "3590", "3590 tape drive", hw::storage},
  {"3592", "3592", "3592 tape drive", hw::storage},
  {"3832", "", "Virtual network device", hw::network},
  {"3880", "3370", "Direct attached storage device (FBA mode)", hw::disk},
  {"3880", "3380", "Direct attached storage device (ECKD mode)", hw::disk},
  {"3990", "3380", "Direct attached storage device (ECKD mode)", hw::disk},
  {"3990", "3390", "Direct attached storage device (ECKD mode)", hw::disk},
  {"6310", "9336", "Direct attached storage device (FBA mode)", hw::disk},
  {"9343", "9345", "Direct attached storage device (ECKD mode)", hw::disk},
};

static void ccw_devtype(hwNode & device, string cutype, string devtype)
{
  for (size_t i = 0; i < sizeof(ccw_device_defs) / sizeof(ccw_device_def); i ++)
  {
    ccw_device_def d = ccw_device_defs[i];
    if (cutype.compare(0, strlen(d.cutype), d.cutype) == 0 &&
        devtype.compare(0, strlen(d.devtype), d.devtype) == 0)
    {
      device.setClass(d.hwClass);
      device.setDescription(d.description);
      break;
    }
  }
  if (!devtype.empty() && devtype != "n/a")
    device.setProduct(devtype);
}

static bool scan_ccw(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_bus("ccw");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    hwNode device("device");

    ccw_devtype(device, e.string_attr("cutype"), e.string_attr("devtype"));
    string vendor = hw::strip(e.string_attr("vendor"));
    if (!vendor.empty())
      device.setVendor(vendor);

    string businfo = e.businfo();
    if (!businfo.empty())
      device.setBusInfo(businfo);

    if (e.string_attr("online") != "1")
      device.disable();

    string driver = e.driver();
    if (!driver.empty())
      device.setConfig("driver", driver);

    string devname = e.name_in_class("block");
    if (!devname.empty())
    {
      device.setLogicalName(devname);
      scan_disk(device);
    }

    device.claim();
    n.addChild(device);
  }

  return true;
}

static bool scan_iucv(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_bus("iucv");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    hwNode device(e.name());

    string driver = e.driver();
    if (!driver.empty())
      device.setConfig("driver", driver);
    if (driver == "hvc_iucv")
      device.setDescription("z/VM IUCV hypervisor console");
    else
      device.setDescription("z/VM IUCV device");

    device.claim();
    n.addChild(device);
  }

  return true;
}

bool scan_s390_devices(hwNode & n)
{
  scan_ccw(n);
  scan_iucv(n);
  return true;
}
