#include "disk.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fs.h>

#ifndef BLKROGET
#define BLKROGET   _IO(0x12,94)	/* get read-only status (0 = read_write) */
#endif
#ifndef BLKGETSIZE
#define BLKGETSIZE _IO(0x12,96)	/* return device size */
#endif
#ifndef BLKSSZGET
#define BLKSSZGET  _IO(0x12,104)	/* get block device sector size */
#endif

bool scan_disk(hwNode & n)
{
  long size = 0;
  int sectsize = 0;

  if (n.getLogicalName() == "")
    return false;

  int fd = open(n.getLogicalName().c_str(), O_RDONLY | O_NONBLOCK);

  if (fd < 0)
    return false;

  if (!n.isCapable("removable") && (n.getSize() == 0))
  {
    if (ioctl(fd, BLKGETSIZE, &size) != 0)
      size = 0;
    if (ioctl(fd, BLKSSZGET, &sectsize) != 0)
      sectsize = 0;

    if ((size > 0) && (sectsize > 0))
      n.setSize((unsigned long long) size * (unsigned long long) sectsize);
  }

  close(fd);

  return true;
}

static char *id = "@(#) $Id: disk.cc,v 1.1 2003/02/25 08:57:32 ezix Exp $";
