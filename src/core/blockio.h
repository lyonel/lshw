#ifndef _BLOCKIO_H_
#define _BLOCKIO_H_

#include <stdint.h>
#include <unistd.h>
#include <string>

#define BLOCKSIZE 512

struct source
{
  std::string diskname;
  int fd;
  ssize_t blocksize;
  long long offset;
  long long size;
};

ssize_t readlogicalblocks(source & s,
void * buffer,
long long pos, long long count);
#endif
