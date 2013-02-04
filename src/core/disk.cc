#include "version.h"
#include "disk.h"
#include "osutils.h"
#include "heuristics.h"
#include "partitions.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

//#include <linux/fs.h>

__ID("@(#) $Id$");

#ifndef BLKROGET
#define BLKROGET   _IO(0x12,94)                   /* get read-only status (0 = read_write) */
#endif
#ifndef BLKGETSIZE
#define BLKGETSIZE _IO(0x12,96)                   /* return device size */
#endif
#ifndef BLKGETSIZE64
#define BLKGETSIZE64 _IOR(0x12,114,size_t)        /* size in bytes */
#endif
#ifndef BLKSSZGET
#define BLKSSZGET  _IO(0x12,104)                  /* get block device sector size */
#endif
#ifndef BLKPBSZGET
#define BLKPBSZGET _IO(0x12,123)
#endif

bool scan_disk(hwNode & n)
{
  long size = 0;
  unsigned long long bytes = 0;
  int sectsize = 0;
  int physsectsize = 0;

  if (n.getLogicalName() == "")
    return false;

  int fd = open(n.getLogicalName().c_str(), O_RDONLY | O_NONBLOCK);

  if (fd < 0)
    return false;

  if (ioctl(fd, BLKPBSZGET, &physsectsize) != 0)
    physsectsize = 0;
  if(physsectsize)
    n.setConfig("sectorsize", physsectsize);

  if (ioctl(fd, BLKSSZGET, &sectsize) < 0)
    sectsize = 0;
  if (sectsize)
    n.setConfig("logicalsectorsize", sectsize);

  if (n.getSize() == 0)
  {
    if(ioctl(fd, BLKGETSIZE64, &bytes) == 0)
    {
      n.setSize(bytes);
    }
    else
    {
      if (ioctl(fd, BLKGETSIZE, &size) != 0)
        size = 0;
      
      if ((size > 0) && (sectsize > 0)){
        n.setSize((unsigned long long) size * (unsigned long long) sectsize);
      }
    }
  }

  close(fd);

  if(n.getSize()>=0)
  {
    n.addHint("icon", string("disc"));
    scan_partitions(n);
  }

  return true;
}
