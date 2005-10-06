/*
 * partitions.cc
 *
 *
 */

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS	64

#include "partitions.h"
#include "osutils.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static char *id = "@(#) $Id$";

#define BLOCKSIZE 512
#define LIFBLOCKSIZE 256

struct source
{
	int fd;
	ssize_t blocksize;
	unsigned long long offset;
	unsigned long long size;
};

struct maptypes
{
	const char * id;
	const char * description;
	bool (*detect)(source & s, hwNode & n);
};

static bool detect_dosmap(source & s, hwNode & n);
static bool detect_macmap(source & s, hwNode & n);
static bool detect_lif(source & s, hwNode & n);

static struct maptypes map_types[] = {
	{"mac", "Apple Macintosh partition map", detect_macmap},
	{"bsd", "BSD disklabel", NULL},
	{"dos", "MS-DOS partition table", detect_dosmap},
	{"lif", "HP-UX LIF", detect_lif},
	{"solaris-x86", "Solaris disklabel", NULL},
	{"solaris-sparc", "Solaris disklabel", NULL},
	{"raid", "Linux RAID", NULL},
	{"lvm", "Linux LVM", NULL},
	{"atari", "Atari ST", NULL},
	{"amiga", "Amiga", NULL},
	{ NULL, NULL, NULL }
};

struct fstypes
{
	const char * id;
	const char * description;
	const char * capabilities;
};

static struct fstypes fs_types[] = {
	{"blank", "Blank", ""},
	{"fat", "MS-DOS FAT derivatives (FAT12, FAT16, FAT32)", ""},
	{"ntfs", "Windows NTFS", "secure"},
	{"hpfs", "OS/2 HPFS", "secure"},
	{"ext2", "Linux EXT2", "secure"},
	{"ext3", "Linux EXT3", "secure,journaled"},
	{"reiserfs", "Linux ReiserFS", "secure,journaled"},
	{"romfs", "Linux ROMFS", "ro"},
	{"squashfs", "Linux SquashFS", "ro"},
	{"cramfs", "Linux CramFS", "ro"},
	{"minixfs", "MinixFS", "secure"},
	{"sysvfs", "System V FS", "secure"},
	{"jfs", "Linux JFS", "secure,journaled"},
	{"xfs", "Linux XFS", "secure,journaled"},
	{"iso9660", "ISO-9660", "secure,ro"},
	{"xboxdvd", "X-Box DVD", "ro"},
	{"udf", "UDF", "secure,ro"},
	{"ufs", "UFS", "secure"},
	{"hphfs", "HP-UX HFS", "secure"},
	{"vxfs", "VxFS", "secure,journaled"},
	{"ffs", "FFS", "secure"},
	{"befs", "BeOS BFS", "journaled"},
	{"qnxfs", "QNX FS", ""},
	{"mfs", "MacOS MFS", ""},
	{"hfs", "MacOS HFS", ""},
	{"hfsplus", "MacOS HFS+", "secure,journaled"},
	{ NULL, NULL, NULL }
};

/* DOS partition types (from util-linux's fdisk)*/
struct systypes
{
	const unsigned char type;
	const char * description;
	const char * id;
	const char * capabilities;
	const char * icon;
};


static struct systypes dos_sys_types[] = {
	{0x00, "Empty", "empty", "nofs", ""},
	{0x01, "FAT12", "fat12", "", ""},
	{0x02, "XENIX root", "xenix", "", ""},
	{0x03, "XENIX usr", "xenix", "", ""},
	{0x04, "FAT16 <32M", "fat16", "", ""},
	{0x05, "Extended", "extended", "multi", ""},		/* DOS 3.3+ extended partition */
	{0x06, "FAT16", "fat16", "", ""},		/* DOS 16-bit >=32M */
	{0x07, "HPFS/NTFS", "ntfs", "", ""},	/* OS/2 IFS, eg, HPFS or NTFS or QNX */
	{0x08, "AIX", "", "", ""},		/* AIX boot (AIX -- PS/2 port) or SplitDrive */
	{0x09, "AIX bootable", "", "boot", ""},	/* AIX data or Coherent */
	{0x0a, "OS/2 Boot Manager", "", "boot,nofs", ""},/* OS/2 Boot Manager */
	{0x0b, "W95 FAT32", "fat32", "", ""},
	{0x0c, "W95 FAT32 (LBA)", "fat32", "", ""},/* LBA really is `Extended Int 13h' */
	{0x0e, "W95 FAT16 (LBA)", "fat32", "", ""},
	{0x0f, "W95 Ext'd (LBA)", "fat32", "", ""},
	{0x10, "OPUS", "", "", ""},
	{0x11, "Hidden FAT12", "fat12", "hidden", ""},
	{0x12, "Compaq diagnostics", "", "boot", ""},
	{0x14, "Hidden FAT16 <32M", "", "hidden", ""},
	{0x16, "Hidden FAT16", "", "hidden", ""},
	{0x17, "Hidden HPFS/NTFS", "ntfs", "hidden", ""},
	{0x18, "AST SmartSleep", "", "nofs", ""},
	{0x1b, "Hidden W95 FAT32", "", "hidden", ""},
	{0x1c, "Hidden W95 FAT32 (LBA)", "", "hidden", ""},
	{0x1e, "Hidden W95 FAT16 (LBA)", "", "hidden", ""},
	{0x24, "NEC DOS", "", "", ""},
	{0x39, "Plan 9", "plan9", "", ""},
	{0x3c, "PartitionMagic recovery", "", "nofs", ""},
	{0x40, "Venix 80286", "", "", ""},
	{0x41, "PPC PReP Boot", "", "boot", ""},
	{0x42, "SFS", "", "", ""},
	{0x4d, "QNX4.x", "", "", ""},
	{0x4e, "QNX4.x 2nd part", "", "", ""},
	{0x4f, "QNX4.x 3rd part", "", "", ""},
	{0x50, "OnTrack DM", "", "", ""},
	{0x51, "OnTrack DM6 Aux1", "", "", ""},	/* (or Novell) */
	{0x52, "CP/M", "cpm", "", ""},		/* CP/M or Microport SysV/AT */
	{0x53, "OnTrack DM6 Aux3", "", "", ""},
	{0x54, "OnTrackDM6", "", "", ""},
	{0x55, "EZ-Drive", "", "", ""},
	{0x56, "Golden Bow", "", "", ""},
	{0x5c, "Priam Edisk", "", "", ""},
	{0x61, "SpeedStor", "", "", ""},
	{0x63, "GNU HURD or SysV", "", "", ""},	/* GNU HURD or Mach or Sys V/386 (such as ISC UNIX) */
	{0x64, "Novell Netware 286", "", "", ""},
	{0x65, "Novell Netware 386", "", "", ""},
	{0x70, "DiskSecure Multi-Boot", "", "boot", ""},
	{0x75, "PC/IX", "", "", ""},
	{0x80, "Old Minix", "minix", "", ""},	/* Minix 1.4a and earlier */
	{0x81, "Minix / old Linux", "minix", "", ""},/* Minix 1.4b and later */
	{0x82, "Linux swap / Solaris", "swap", "nofs", ""},
	{0x83, "Linux filesystem", "linux", "", ""},
	{0x84, "OS/2 hidden C: drive", "", "hidden", ""},
	{0x85, "Linux extended", "", "multi", ""},
	{0x86, "NTFS volume set", "", "multi", "md"},
	{0x87, "NTFS volume set", "", "multi", "md"},
	{0x88, "Linux plaintext", "", "", ""},
	{0x8e, "Linux LVM", "lvm", "multi", "md"},
	{0x93, "Amoeba", "", "", ""},
	{0x94, "Amoeba BBT", "", "", ""},	/* (bad block table) */
	{0x9f, "BSD/OS", "bsdos", "", ""},		/* BSDI */
	{0xa0, "IBM Thinkpad hibernation", "", "nofs", ""},
	{0xa5, "FreeBSD", "freebsd", "", ""},		/* various BSD flavours */
	{0xa6, "OpenBSD", "openbsd", "", ""},
	{0xa7, "NeXTSTEP", "nextstep", "", ""},
	{0xa8, "Darwin UFS", "darwin", "", ""},
	{0xa9, "NetBSD", "netbsd", "", ""},
	{0xab, "Darwin boot", "", "boot,nofs", ""},
	{0xb7, "BSDI fs", "", "", ""},
	{0xb8, "BSDI swap", "", "nofs", ""},
	{0xbb, "Boot Wizard hidden", "", "boot,nofs", ""},
	{0xbe, "Solaris boot", "", "boot,nofs", ""},
	{0xbf, "Solaris", "", "", ""},
	{0xc1, "DRDOS/sec (FAT-12)", "", "", ""},
	{0xc4, "DRDOS/sec (FAT-16 < 32M)", "", "", ""},
	{0xc6, "DRDOS/sec (FAT-16)", "", "", ""},
	{0xc7, "Syrinx", "", "", ""},
	{0xda, "Non-FS data", "", "nofs", ""},
	{0xdb, "CP/M / CTOS / ...", "", "", ""},/* CP/M or Concurrent CP/M or
					   Concurrent DOS or CTOS */
	{0xde, "Dell Utility", "", "", ""},	/* Dell PowerEdge Server utilities */
	{0xdf, "BootIt", "", "boot,nofs", ""},		/* BootIt EMBRM */
	{0xe1, "DOS access", "", "", ""},	/* DOS access or SpeedStor 12-bit FAT
					   extended partition */
	{0xe3, "DOS R/O", "", "", ""},		/* DOS R/O or SpeedStor */
	{0xe4, "SpeedStor", "", "", ""},	/* SpeedStor 16-bit FAT extended
					   partition < 1024 cyl. */
	{0xeb, "BeOS fs", "", "", ""},
	{0xee, "EFI GPT", "", "nofs", ""},		/* Intel EFI GUID Partition Table */
	{0xef, "EFI (FAT-12/16/32)", "", "", ""},/* Intel EFI System Partition */
	{0xf0, "Linux/PA-RISC boot", "", "boot", ""},/* Linux/PA-RISC boot loader */
	{0xf1, "SpeedStor", "", "", ""},
	{0xf4, "SpeedStor", "", "", ""},	/* SpeedStor large partition */
	{0xf2, "DOS secondary", "", "", ""},	/* DOS 3.3+ secondary */
	{0xfd, "Linux raid autodetect", "", "multi", "md"},/* New (2.2.x) raid partition with
					       autodetect using persistent
					       superblock */
	{0xfe, "LANstep", "", "", ""},		/* SpeedStor >1024 cyl. or LANstep */
	{0xff, "BBT", "", "", ""},		/* Xenix Bad Block Table */
	{ 0, NULL, NULL, NULL }
};

static ssize_t readlogicalblocks(source & s,
			void * buffer,
			off_t pos, ssize_t count)
{
  off_t result = 0;

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

static bool guess_logicalname(source & s, const hwNode & disk, unsigned int n, hwNode & partition)
{
  struct stat buf;
  char name[10];
  int dev = 0;

  snprintf(name, sizeof(name), "%d", n);
  if(disk.getBusInfo()!="")
    partition.setBusInfo(disk.getBusInfo()+string(",")+string(name));

  if(fstat(s.fd, &buf)!=0) return false;
  if(!S_ISBLK(buf.st_mode)) return false;

  if((dev = open_dev(buf.st_rdev + n, disk.getLogicalName()+string(name)))>=0)
  {
    source spart = s;
    unsigned char buffer1[BLOCKSIZE], buffer2[BLOCKSIZE];

    spart.offset = 0;
    spart.fd = dev;

    if((readlogicalblocks(s, buffer1, 0, 1)!=1) ||
       (readlogicalblocks(spart, buffer2, 0, 1)!=1))     // read the first sector
    {
      close(dev);
      return false;
    }

    close(dev);

    if(memcmp(buffer1, buffer2, BLOCKSIZE)==0)
    {
      partition.claim();
      partition.setLogicalName(disk.getLogicalName()+string(name));
      return true;
    }
  }
  
  return false;
}

static bool analyse_dosextpart(source & s,
		unsigned char flags,
		unsigned char type,
		hwNode & partition)
{
  int i = 0;

  if(flags!=0 && flags!=0x80)	// inconsistency: partition is either bootable or non-bootable
    return false;

  if(s.offset==0 || s.size==0) // unused entry
    return false;

  partition.setDescription("Extended partition");
  partition.addCapability("extended", "Extended partition");
  partition.addCapability("partitioned", "Partitioned disk");
  partition.addCapability("partitioned:extended", "Extended partition");
  partition.setCapacity(s.size);

  partition.describeCapability("nofs", "No filesystem");

  if(flags == 0x80)
    partition.addCapability("bootable", "Active partition (bootable)");

  return true;
}

static bool analyse_dospart(source & s,
		unsigned char flags,
		unsigned char type,
		hwNode & partition)
{
  int i = 0;

  if(type == 0x5 || type == 0x85)
    return analyse_dosextpart(s, flags, type, partition);

  if(flags!=0 && flags!=0x80)	// inconsistency: partition is either bootable or non-bootable
    return false;

  if(s.offset==0 || s.size==0) // unused entry
    return false;

  partition.setDescription("Primary partition");
  partition.addCapability("primary", "Primary partition");
  partition.setCapacity(s.size);

  if(flags == 0x80)
    partition.addCapability("bootable", "Active partition (bootable)");

  while(dos_sys_types[i].id)
  {
    if(dos_sys_types[i].type == type)
    {
      partition.setDescription(dos_sys_types[i].description);
      if(lowercase(dos_sys_types[i].capabilities) != "")
      {
        vector<string> capabilities;
        splitlines(dos_sys_types[i].capabilities, capabilities, ',');

        for(unsigned j=0; j<capabilities.size(); j++)
           partition.addCapability(capabilities[j]);
      }
      if(lowercase(dos_sys_types[i].icon) != "")
        partition.addHint("icon", string(dos_sys_types[i].icon));
      else
        partition.addHint("icon", string("disc"));
      break;
    }
    i++;
  }

  partition.describeCapability("nofs", "No filesystem");
  partition.describeCapability("boot", "Contains boot code");
  partition.describeCapability("multi", "Multi-volumes");
  partition.describeCapability("hidden", "Hidden partition");

  return true;
}

static bool detect_dosmap(source & s, hwNode & n)
{
  static unsigned char buffer[BLOCKSIZE];
  int i = 0;
  unsigned char flags;
  unsigned char type;
  unsigned long long start, size;

  if(s.offset!=0)
    return false;	// partition tables must be at the beginning of the disk

  if(readlogicalblocks(s, buffer, 0, 1)!=1)	// read the first sector
    return false;

  if(le_short(buffer+510)!=0xaa55)		// wrong magic number
    return false;

  for(i=0; i<4; i++)
  {
    source spart = s;
    hwNode partition("volume", hw::disk);

    flags = buffer[446 + i*16];
    type = buffer[446 + i*16 + 4];
    start = le_long(buffer + 446 + i*16 + 8);
    size = le_long(buffer + 446 + i*16 + 12);

    if(flags!=0 && flags!=0x80)	// inconsistency: partition is either bootable or non-bootable
      return false;

    spart.blocksize = s.blocksize;
    spart.offset = s.offset + start*spart.blocksize;
    spart.size = size*spart.blocksize;
 
    partition.setPhysId(i+1);
    partition.setCapacity(spart.size);

    if(analyse_dospart(spart, flags, type, partition))
    {
      guess_logicalname(spart, n, i+1, partition);
      n.addChild(partition);
    }
  }

  return true;
}

static bool detect_macmap(source & s, hwNode & n)
{
  static unsigned char buffer[BLOCKSIZE];
  unsigned long count = 0, i = 0;
  unsigned long long start = 0, size = 0;
  string type = "";

  if(s.offset!=0)
    return false;	// partition maps must be at the beginning of the disk

  if(readlogicalblocks(s, buffer, 1, 1)!=1)	// read the second sector
    return false;

  if(be_short(buffer)!=0x504d)			// wrong magic number
    return false;

  count = be_long(buffer+4);

  for (i = 1; i <= count; i++)
  {
    hwNode partition("volume", hw::disk);

    if((i>1) && readlogicalblocks(s, buffer, i, 1)!=1)
      return false;

    if(be_short(buffer)!=0x504d) continue;	// invalid map entry

    start = be_long(buffer + 8);
    size = be_long(buffer + 12);
    type = hw::strip(string((char*)buffer +  48, 32));

    partition.setPhysId(i);
    partition.setCapacity(size * s.blocksize);
    if(lowercase(type) == "apple_bootstrap")
      partition.addCapability("bootable", "Bootstrap partition");

    if(lowercase(type) == "linux_lvm")
      partition.addHint("icon", string("md"));
    else
      partition.addHint("icon", string("disc"));

    for(unsigned int j=0; j<type.length(); j++)
      if(type[j] == '_') type[j] = ' ';
    partition.setDescription(type);

    if(true /*analyse_macpart(flags, type, start, size, partition)*/)
    {
      source spart = s;

      spart.blocksize = s.blocksize;
      spart.offset = s.offset + start*spart.blocksize;
      spart.size = size*spart.blocksize;

      guess_logicalname(spart, n, i, partition);
      n.addChild(partition);
    }
  }

  return true;
}

static bool detect_lif(source & s, hwNode & n)
{
  static unsigned char buffer[LIFBLOCKSIZE];
  source lifvolume;
  unsigned long dir_start = 0, dir_length = 0;
  unsigned lif_version = 0;
  unsigned long ipl_addr = 0, ipl_length = 0, ipl_entry = 0;

  if(s.offset!=0)
    return false;	// LIF boot volumes must be at the beginning of the disk

  lifvolume = s;
  lifvolume.blocksize = LIFBLOCKSIZE;	// LIF blocks are 256 bytes long

  if(readlogicalblocks(lifvolume, buffer, 0, 1)!=1)	// read the first block
    return false;

  if(be_short(buffer)!=0x8000)			// wrong magic number
    return false;

  dir_start = be_long(buffer+8);
  dir_length = be_long(buffer+16);
  lif_version = be_short(buffer+20);

  if(dir_start<2) return false;		// blocks 0 and 1 are reserved
  if(dir_length<1) return false;	// no directory to read from
  if(lif_version<1) return false;	// weird LIF version

  ipl_addr = be_long(buffer+240);	// byte address of IPL on media
  ipl_length = be_long(buffer+244);	// size of boot code
  ipl_entry = be_long(buffer+248);	// boot code entry point

  fprintf(stderr, "system: %x\n", be_short(buffer+12));
  fprintf(stderr, "start of directory: %ld\n", dir_start);
  fprintf(stderr, "length of directory: %ld\n", dir_length);
  fprintf(stderr, "lif version: %x\n", lif_version);
  fprintf(stderr, "tracks per surface: %ld\n", be_long(buffer+24));
  fprintf(stderr, "number of surfaces: %ld\n", be_long(buffer+28));
  fprintf(stderr, "blocks per track: %ld\n", be_long(buffer+32));
  fprintf(stderr, "ipl addr: %ld\n", ipl_addr);
  fprintf(stderr, "ipl length: %ld\n", ipl_length);
  fprintf(stderr, "ipl entry point: %lx\n", ipl_entry);

  if((ipl_addr!=0) && (ipl_length>0)) n.addCapability("bootable", "Bootable disk");

  return true;
}

bool scan_partitions(hwNode & n)
{
  int i = 0;
  source s;
  int fd = open(n.getLogicalName().c_str(), O_RDONLY | O_NONBLOCK);
  hwNode * medium = NULL;

  if (fd < 0)
    return false;

  if(n.isCapable("removable"))
  {
    medium = n.addChild(hwNode("disc", hw::disk));

    medium->claim();
    medium->setSize(n.getSize());
    medium->setCapacity(n.getCapacity());
    medium->setLogicalName(n.getLogicalName());
  }
  else
    medium = &n;

  s.fd = fd;
  s.offset = 0;
  s.blocksize = BLOCKSIZE;
  s.size = medium->getSize();

  while(map_types[i].id)
  {
    if(map_types[i].detect && map_types[i].detect(s, *medium))
    {
      medium->addCapability(string("partitioned"), "Partitioned disk");
      medium->addCapability(string("partitioned:") + string(map_types[i].id), string(map_types[i].description));
      break;
    }
    i++;
  }

  close(fd);

  //if(medium != &n) free(medium);

  (void) &id;			// avoid warning "id defined but not used"

  return false;
}

