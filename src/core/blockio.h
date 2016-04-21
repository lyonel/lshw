#ifndef _BLOCKIO_H_
#define _BLOCKIO_H_

#include <stdint.h>
#include <unistd.h>
#include <string>
#include <vector>

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

template <typename T>
ssize_t readlogicalblocks(source & s,
std::vector<T> & buffer,
long long pos, long long count) {
  typedef char T_must_be_byte[sizeof(T) == 1? 1: -1];
  buffer.resize(s.blocksize * count);
  return readlogicalblocks(s, &buffer[0], pos, count);
}
#endif
