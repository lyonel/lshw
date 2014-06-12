#include "disk.h"
#include "osutils.h"
#include "dasd.h"
#include <glob.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <map>

using namespace std;

/*Read only block devices, not partitions*/
#define DEV_PATTERN "/dev/dasd[a-z]"
#define SYSFS_PREFIX "/sys/block/"

bool scan_dasd(hwNode & n)
{
  size_t dev_num;
  char *dev_name;
  glob_t devices;
  uint64_t capacity;

  /* These correspond to filenames in the device/ sub-directory
     of each device
     To read other attributes, simply add the attribute here and modify the device object
     appropriately.
  */
  const char* attribs[] = {"devtype", "vendor"};

  std::vector<std::string> sysfs_attribs(attribs, attribs+2);
  std::map<std::string, std::string> dasd_attribs;

  hwNode *core = n.getChild("core");

  /* Probe the sysfs for this device*/
  if(glob(DEV_PATTERN, 0, NULL, &devices)!=0)
    return false;
  else
  {
    for(dev_num=0;dev_num<devices.gl_pathc;dev_num++)
    {
      dev_name = basename(devices.gl_pathv[dev_num]);
      for (std::vector<std::string>::iterator it = sysfs_attribs.begin(); it != sysfs_attribs.end(); ++it)
      {
        std::string attrib_fname = std::string(SYSFS_PREFIX) + dev_name + "/device/" + *it;
        std::vector<std::string> lines;
        if (loadfile(attrib_fname, lines))
        {
          dasd_attribs[*it] = lines[0];
        }
        else
        {
          dasd_attribs[*it] = " ";
        }
      }

      hwNode device = hwNode("disk", hw::disk);
      device.setDescription("DASD Device");
      device.setVendor(dasd_attribs["vendor"]);
      device.setProduct(dasd_attribs["devtype"]);
      device.setLogicalName(devices.gl_pathv[dev_num]);

      /* Get the capacity*/
      int fd = open(devices.gl_pathv[dev_num], O_RDONLY|O_NONBLOCK);
      if (ioctl(fd, BLKGETSIZE64, &capacity) != 0)
        capacity=0;
      close(fd);

      device.setSize(capacity);

      /* Non-determinant*/
      device.setPhysId(0);
      scan_disk(device);

      hwNode *parent = core->addChild(device);
      parent->claim();
    }
  }
  return true;
}
