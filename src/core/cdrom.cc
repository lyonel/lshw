/*
 * cdrom.cc
 *
 * This scan reports optical media-specific capabilities:
 * - audio CD playback
 * - CD-R burning
 * - CD-RW burning
 * - DVD reading
 * - DVD-R burning
 * - DVD-RAM burning
 *
 * NOTE: DVD+R/RW are not covered by the ioctls used in this code and, therefore,
 * not reported.
 *
 *
 */

#include "version.h"
#include "cdrom.h"
#include "partitions.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <linux/cdrom.h>

__ID("@(#) $Id$");

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

  n.addHint("icon", string("cd"));

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
    n.addCapability("audio", "Audio CD playback");
  if (capabilities & CDC_CD_R)
  {
    n.addCapability("cd-r", "CD-R burning");
    n.setDescription("CD-R writer");
  }
  if (capabilities & CDC_CD_RW)
  {
    n.addCapability("cd-rw", "CD-RW burning");
    n.setDescription("CD-R/CD-RW writer");
  }
  if (capabilities & CDC_DVD)
  {
    n.addCapability("dvd", "DVD playback");
    n.setDescription("DVD reader");
  }
  if (capabilities & CDC_DVD_R)
  {
    n.addCapability("dvd-r", "DVD-R burning");
    n.setDescription("DVD writer");
  }
  if (capabilities & CDC_DVD_RAM)
  {
    n.addCapability("dvd-ram", "DVD-RAM burning");
    n.setDescription("DVD-RAM writer");
  }

  switch(ioctl(fd, CDROM_DRIVE_STATUS, 0))
  {
    case CDS_NO_INFO:
    case CDS_NO_DISC:
      n.setConfig("status", "nodisc");
      break;
    case CDS_TRAY_OPEN:
      n.setConfig("status", "open");
      break;
    case CDS_DRIVE_NOT_READY:
      n.setConfig("status", "busy");
      break;
    case CDS_DISC_OK:
      n.setConfig("status", "ready");
      scan_partitions(n);
      break;
  }
  close(fd);

  return true;
}
