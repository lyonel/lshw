#include "cdrom.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/cdrom.h>

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
    n.addCapability("cd-r");
  if (capabilities & CDC_CD_RW)
    n.addCapability("cd-rw");
  if (capabilities & CDC_DVD)
    n.addCapability("dvd");
  if (capabilities & CDC_DVD_R)
    n.addCapability("dvd-r");
  if (capabilities & CDC_DVD_RAM)
    n.addCapability("dvd-ram");

  return true;
}

static char *id = "@(#) $Id: cdrom.cc,v 1.1 2003/02/06 23:19:16 ezix Exp $";
