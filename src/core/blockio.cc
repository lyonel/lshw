/*
 * blockio.cc
 *
 *
 */

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS	64

#include "blockio.h"
#include "osutils.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

static const char rcsid[] = "@(#) $Id$";

ssize_t readlogicalblocks(source & s,
			void * buffer,
			long long pos, long long count)
{
  long long result = 0;

  memset(buffer, 0, count*s.blocksize);

  if((s.size>0) && ((pos+count)*s.blocksize>s.size)) return 0;	/* attempt to read past the end of the section */

  result = lseek(s.fd, s.offset + pos*s.blocksize, SEEK_SET);

  if(result == -1) return 0;

  result = read(s.fd, buffer, count*s.blocksize);

  if(result!=count*s.blocksize)
    return 0;
  else
    return count;
}


