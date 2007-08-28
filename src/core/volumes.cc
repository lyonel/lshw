/*
 * volumes.cc
 *
 */

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include "version.h"
#include "volumes.h"
#include "blockio.h"
#include "lvm.h"
#include "osutils.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

__ID("@(#) $Id$");

struct fstypes
{
  const char * id;
  const char * description;
  const char * capabilities;
  bool (*detect)(hwNode & n, source & s);
};

static bool detect_luks(hwNode & n, source & s);

static struct fstypes fs_types[] =
{
  {"blank", "Blank", "", NULL},
  {"fat", "MS-DOS FAT derivatives (FAT12, FAT16, FAT32)", "", NULL},
  {"ntfs", "Windows NTFS", "secure", NULL},
  {"hpfs", "OS/2 HPFS", "secure", NULL},
  {"ext2", "Linux EXT2", "secure", NULL},
  {"ext3", "Linux EXT3", "secure,journaled", NULL},
  {"reiserfs", "Linux ReiserFS", "secure,journaled", NULL},
  {"romfs", "Linux ROMFS", "ro", NULL},
  {"squashfs", "Linux SquashFS", "ro", NULL},
  {"cramfs", "Linux CramFS", "ro", NULL},
  {"minixfs", "MinixFS", "secure", NULL},
  {"sysvfs", "System V FS", "secure", NULL},
  {"jfs", "Linux JFS", "secure,journaled", NULL},
  {"xfs", "Linux XFS", "secure,journaled", NULL},
  {"iso9660", "ISO-9660", "secure,ro", NULL},
  {"xboxdvd", "X-Box DVD", "ro", NULL},
  {"udf", "UDF", "secure,ro", NULL},
  {"ufs", "UFS", "secure", NULL},
  {"hphfs", "HP-UX HFS", "secure", NULL},
  {"vxfs", "VxFS", "secure,journaled", NULL},
  {"ffs", "FFS", "secure", NULL},
  {"befs", "BeOS BFS", "journaled", NULL},
  {"qnxfs", "QNX FS", "", NULL},
  {"mfs", "MacOS MFS", "", NULL},
  {"hfs", "MacOS HFS", "", NULL},
  {"hfsplus", "MacOS HFS+", "secure,journaled", NULL},
  {"luks", "Linux Unified Key Setup", "encrypted", detect_luks},
  { NULL, NULL, NULL, NULL }
};

static bool detect_luks(hwNode & n, source & s)
{
  static char buffer[BLOCKSIZE];
  source luksvolume;
  unsigned luks_version = 0;

  luksvolume = s;
  luksvolume.blocksize = BLOCKSIZE;

                                                  // read the first block
  if(readlogicalblocks(luksvolume, buffer, 0, 1)!=1)
    return false;

  if(memcmp(buffer, "LUKS", 4) != 0)                    // wrong magic number
    return false;
  if(be_short(buffer+4) != 0xbabe)
    return false;

  luks_version = be_short(buffer+6);
  if(luks_version<1) return false;                 // weird LUKS version

  n.addCapability("encrypted", "Encrypted volume");
  n.setConfig("version", luks_version);
  n.setConfig("cipher", hw::strip(std::string(buffer+8, 32)));
  n.setConfig("mode", hw::strip(std::string(buffer+40, 32)));
  n.setConfig("hash", hw::strip(std::string(buffer+72, 32)));
  n.setConfig("bits", 8*be_long(buffer+108));
  n.setWidth(8*be_long(buffer+108));
  n.setSerial(hw::strip(std::string(buffer+168, 40)));
  n.setCapacity(luksvolume.size);
  n.setSize(luksvolume.size);
  return true;
}

bool scan_volume(hwNode & n, source & s)
{
  int i = 0;

  while(fs_types[i].id)
  {
    if(fs_types[i].detect && fs_types[i].detect(n, s))
    {
      n.addCapability(string("initialized"), "initialized volume");
      n.addCapability(string("initialized:") + string(fs_types[i].id), string(fs_types[i].description));
      n.setDescription(string(fs_types[i].description) + " volume");
      break;
    }
    i++;
  }

  return scan_lvm(n,s);
}
