#include "cdrom.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/cdrom.h>

#ifndef CDC_CD_R
#define CDC_CD_R 0x2000
#endif
#ifndef CDC_CD_RW
#define CDC_CD_RW 0x4000
#endif
#ifndef CDC_DVD
#define CDC_DVD 0x8000
#endif
#ifndef CDC_DVD_R
#define CDC_DVD_R 0x10000
#endif
#ifndef CDC_DVD_RAM
#define CDC_DVD_RAM 0x20000
#endif

bool scan_cdrom(hwNode & n)
{
  if (n.getLogicalName() == "")
    return false;

  int fd = open(n.getLogicalName().c_str(), O_RDONLY | O_NONBLOCK);

  if (fd < 0)
    return false;

  int status = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
  if (status < 0)
  {
    close(fd);
    return false;
  }

  int capabilities = ioctl(fd, CDROM_GET_CAPABILITY);

  if (capabilities < 0)
  {
    close(fd);
    return false;
  }

  if (capabilities & CDC_PLAY_AUDIO)
    n.addCapability("audio");
  if (capabilities & CDC_CD_R)
  {
    n.addCapability("cd-r");
    n.setDescription("CD-R writer");
  }
  if (capabilities & CDC_CD_RW)
  {
    n.addCapability("cd-rw");
    n.setDescription("CD-R/CD-RW writer");
  }
  if (capabilities & CDC_DVD)
  {
    n.addCapability("dvd");
    n.setDescription("DVD reader");
  }
  if (capabilities & CDC_DVD_R)
  {
    n.addCapability("dvd-r");
    n.setDescription("DVD writer");
  }
  if (capabilities & CDC_DVD_RAM)
  {
    n.addCapability("dvd-ram");
    n.setDescription("DVD-RAM writer");
  }

  return true;
}

static char *id = "@(#) $Id: cdrom.cc,v 1.3 2003/02/17 17:43:12 ezix Exp $";
