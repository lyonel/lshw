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

struct source
{
	int fd;
	ssize_t blocksize;
	off_t offset;
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
static bool detect_nomap(source & s, hwNode & n);

static struct maptypes map_types[] = {
	{"mac", "Apple Macintosh partition map", detect_macmap},
	{"bsd", "BSD disklabel", NULL},
	{"hp-ux", "HP-UX non-LVM", NULL},
	{"solaris-x86", "Solaris disklabel", NULL},
	{"solaris-sparc", "Solaris disklabel", NULL},
	{"raid", "Linux RAID", NULL},
	{"lvm", "Linux LVM", NULL},
	{"atari", "Atari ST", NULL},
	{"amiga", "Amiga", NULL},
	{"dos", "MS-DOS partition table", detect_dosmap},
	{"raw", "not partitioned", detect_nomap},
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
	const char * filesystems;
};


static struct systypes dos_sys_types[] = {
	{0x00, "Empty", "empty", "nofs", ""},
	{0x01, "FAT12", "fat12", "", "fat"},
	{0x02, "XENIX root", "xenix", "", ""},
	{0x03, "XENIX usr", "xenix", "", ""},
	{0x04, "FAT16 <32M", "fat16", "", "fat"},
	{0x05, "Extended", "extended", "", ""},		/* DOS 3.3+ extended partition */
	{0x06, "FAT16", "fat16", "", "fat"},		/* DOS 16-bit >=32M */
	{0x07, "HPFS/NTFS", "ntfs", "", "ntfs,hpfs"},	/* OS/2 IFS, eg, HPFS or NTFS or QNX */
	{0x08, "AIX", "", "", ""},		/* AIX boot (AIX -- PS/2 port) or SplitDrive */
	{0x09, "AIX bootable", "", "", ""},	/* AIX data or Coherent */
	{0x0a, "OS/2 Boot Manager", "", "nofs", ""},/* OS/2 Boot Manager */
	{0x0b, "W95 FAT32", "fat32", "", "fat"},
	{0x0c, "W95 FAT32 (LBA)", "fat32", "", "fat"},/* LBA really is `Extended Int 13h' */
	{0x0e, "W95 FAT16 (LBA)", "fat32", "", "fat"},
	{0x0f, "W95 Ext'd (LBA)", "fat32", "", "fat"},
	{0x10, "OPUS", "", "", ""},
	{0x11, "Hidden FAT12", "fat12", "hidden", "fat"},
	{0x12, "Compaq diagnostics", "", "", "fat"},
	{0x14, "Hidden FAT16 <32M", "", "hidden", "fat"},
	{0x16, "Hidden FAT16", "", "hidden", "fat"},
	{0x17, "Hidden HPFS/NTFS", "ntfs", "hidden", "ntfs"},
	{0x18, "AST SmartSleep", "", "nofs", ""},
	{0x1b, "Hidden W95 FAT32", "", "hidden", "fat"},
	{0x1c, "Hidden W95 FAT32 (LBA)", "", "hidden", "fat"},
	{0x1e, "Hidden W95 FAT16 (LBA)", "", "hidden", "fat"},
	{0x24, "NEC DOS", "", "", "fat"},
	{0x39, "Plan 9", "plan9", "", ""},
	{0x3c, "PartitionMagic recovery", "", "nofs", ""},
	{0x40, "Venix 80286", "", "", ""},
	{0x41, "PPC PReP Boot", "", "", ""},
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
	{0x70, "DiskSecure Multi-Boot", "", "", ""},
	{0x75, "PC/IX", "", "", ""},
	{0x80, "Old Minix", "minix", "", ""},	/* Minix 1.4a and earlier */
	{0x81, "Minix / old Linux", "minix", "", ""},/* Minix 1.4b and later */
	{0x82, "Linux swap / Solaris", "swap", "nofs", ""},
	{0x83, "Linux", "linux", "", ""},
	{0x84, "OS/2 hidden C: drive", "", "hidden", ""},
	{0x85, "Linux extended", "", "nofs", ""},
	{0x86, "NTFS volume set", "", "", ""},
	{0x87, "NTFS volume set", "", "", ""},
	{0x88, "Linux plaintext", "", "", ""},
	{0x8e, "Linux LVM", "lvm", "nofs", ""},
	{0x93, "Amoeba", "", "", ""},
	{0x94, "Amoeba BBT", "", "", ""},	/* (bad block table) */
	{0x9f, "BSD/OS", "bsdos", "", ""},		/* BSDI */
	{0xa0, "IBM Thinkpad hibernation", "", "nofs", ""},
	{0xa5, "FreeBSD", "freebsd", "", ""},		/* various BSD flavours */
	{0xa6, "OpenBSD", "openbsd", "", ""},
	{0xa7, "NeXTSTEP", "nextstep", "", ""},
	{0xa8, "Darwin UFS", "darwin", "", ""},
	{0xa9, "NetBSD", "netbsd", "", ""},
	{0xab, "Darwin boot", "", "nofs", ""},
	{0xb7, "BSDI fs", "", "", ""},
	{0xb8, "BSDI swap", "", "nofs", ""},
	{0xbb, "Boot Wizard hidden", "", "nofs", ""},
	{0xbe, "Solaris boot", "", "nofs", ""},
	{0xbf, "Solaris", "", "", ""},
	{0xc1, "DRDOS/sec (FAT-12)", "", "", "fat"},
	{0xc4, "DRDOS/sec (FAT-16 < 32M)", "", "", "fat"},
	{0xc6, "DRDOS/sec (FAT-16)", "", "", "fat"},
	{0xc7, "Syrinx", "", "", ""},
	{0xda, "Non-FS data", "", "nofs", ""},
	{0xdb, "CP/M / CTOS / ...", "", "", ""},/* CP/M or Concurrent CP/M or
					   Concurrent DOS or CTOS */
	{0xde, "Dell Utility", "", "", ""},	/* Dell PowerEdge Server utilities */
	{0xdf, "BootIt", "", "nofs", ""},		/* BootIt EMBRM */
	{0xe1, "DOS access", "", "", "fat"},	/* DOS access or SpeedStor 12-bit FAT
					   extended partition */
	{0xe3, "DOS R/O", "", "", ""},		/* DOS R/O or SpeedStor */
	{0xe4, "SpeedStor", "", "", "fat"},	/* SpeedStor 16-bit FAT extended
					   partition < 1024 cyl. */
	{0xeb, "BeOS fs", "", "", "befs"},
	{0xee, "EFI GPT", "", "nofs", ""},		/* Intel EFI GUID Partition Table */
	{0xef, "EFI (FAT-12/16/32)", "", "", "fat"},/* Intel EFI System Partition */
	{0xf0, "Linux/PA-RISC boot", "", "", ""},/* Linux/PA-RISC boot loader */
	{0xf1, "SpeedStor", "", "", ""},
	{0xf4, "SpeedStor", "", "", ""},	/* SpeedStor large partition */
	{0xf2, "DOS secondary", "", "", ""},	/* DOS 3.3+ secondary */
	{0xfd, "Linux raid autodetect", "", "", ""},/* New (2.2.x) raid partition with
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

static bool detect_dosmap(source & s, hwNode & n)
{
  static unsigned char buffer[BLOCKSIZE];

  if(s.offset!=0)
    return false;	// partition tables must be at the beginning of the disk

  if(readlogicalblocks(s, buffer, 0, 1)!=1)	// read the first sector
    return false;

  if(le_short(buffer+510)!=0xaa55)			// wrong magic number
    return false;

  return true;
}

static bool detect_macmap(source & s, hwNode & n)
{
  static unsigned char buffer[BLOCKSIZE];

  if(s.offset!=0)
    return false;	// partition maps must be at the beginning of the disk

  if(readlogicalblocks(s, buffer, 1, 1)!=1)	// read the second sector
    return false;

  if(be_short(buffer)!=0x504d)			// wrong magic number
    return false;

  return true;
}

static bool detect_nomap(source & s, hwNode & n)
{
  return true;
}

bool scan_partitions(hwNode & n)
{
  int i = 0;
  source s;
  int fd = open(n.getLogicalName().c_str(), O_RDONLY | O_NONBLOCK);

  if (fd < 0)
    return false;

  s.fd = fd;
  s.offset = 0;
  s.blocksize = BLOCKSIZE;
  s.size = n.getSize();

  while(map_types[i].id)
  {
    if(map_types[i].detect && map_types[i].detect(s, n))
    {
      n.addCapability(string("partitioned"), "Partitioned disk");
      n.addCapability(string("partitioned:") + string(map_types[i].id), string(map_types[i].description));
      break;
    }
    i++;
  }

  close(fd);

  (void) &id;			// avoid warning "id defined but not used"

  return false;
}

