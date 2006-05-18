/*
 * partitions.cc
 *
 *
 */

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS	64

#include "version.h"
#include "partitions.h"
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

#define LIFBLOCKSIZE 256

struct maptypes
{
	const char * id;
	const char * description;
	bool (*detect)(source & s, hwNode & n);
};

static bool detect_dosmap(source & s, hwNode & n);
static bool detect_macmap(source & s, hwNode & n);
static bool detect_lif(source & s, hwNode & n);
static bool detect_gpt(source & s, hwNode & n);

static struct maptypes map_types[] = {
	{"mac", "Apple Macintosh partition map", detect_macmap},
	{"bsd", "BSD disklabel", NULL},
	{"gpt", "GUID partition table", detect_gpt},
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

struct efi_guid_t
{
        uint32_t time_low;
        uint16_t time_mid;
        uint16_t time_hi_and_version;
        uint8_t  clock_seq_hi_and_reserved;
        uint8_t  clock_seq_low;
        uint8_t  node[6];
};

struct gpth
{
        uint64_t Signature;			/* offset: 0 */
        uint32_t Revision;			/* 8 */
        uint32_t HeaderSize;			/* 12 */
        uint32_t HeaderCRC32;			/* 16 */
        uint32_t Reserved1;			/* 20 */
        uint64_t MyLBA;				/* 24 */
        uint64_t AlternateLBA;			/* 32 */
        uint64_t FirstUsableLBA;		/* 40 */
        uint64_t LastUsableLBA;			/* 48 */
        efi_guid_t DiskGUID;			/* 56 */
        uint64_t PartitionEntryLBA;		/* 72 */
        uint32_t NumberOfPartitionEntries;	/* 80 */
        uint32_t SizeOfPartitionEntry;		/* 84 */
        uint32_t PartitionEntryArrayCRC32;	/* 88 */
};

struct efipartition
{
	efi_guid_t PartitionTypeGUID;
	efi_guid_t PartitionGUID;
	uint64_t StartingLBA;
	uint64_t EndingLBA;
	uint64_t Attributes;
	string PartitionName;
};

#define GPT_PMBR_LBA 0
#define GPT_PMBR_SECTORS 1
#define GPT_PRIMARY_HEADER_LBA 1
#define GPT_HEADER_SECTORS 1
#define GPT_PRIMARY_PART_TABLE_LBA 2

#define EFI_PMBR_OSTYPE_EFI 0xee
#define GPT_HEADER_SIGNATURE 0x5452415020494645LL	/* "EFI PART" */

struct dospartition
{
	unsigned long long start;
	unsigned long long size;
	unsigned char type;
	unsigned char flags;
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
	{0x8e, "Linux LVM Physical Volume", "lvm", "multi", "md"},
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
	{0xef, "EFI (FAT-12/16/32)", "", "boot", ""},/* Intel EFI System Partition */
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

static unsigned int lastlogicalpart = 5;

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

  if(s.diskname!="")
    dev = open_dev(buf.st_rdev + n, s.diskname+string(name));
  else
    dev = open_dev(buf.st_rdev + n, disk.getLogicalName()+string(name));

  if(dev>=0)
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
      if(s.diskname!="")
        partition.setLogicalName(s.diskname+string(name));
      else
        partition.setLogicalName(disk.getLogicalName()+string(name));
      return true;
    }
  }
  
  return false;
}

static bool is_extended(unsigned char type)
{
  return (type == 0x5) || (type == 0xf) || (type == 0x85);
}

static bool read_dospartition(source & s, unsigned short i, dospartition & p)
{
  static unsigned char buffer[BLOCKSIZE];
  unsigned char flags = 0;

  if(readlogicalblocks(s, buffer, 0, 1)!=1)	// read the first sector
    return false;

  if(le_short(buffer+510)!=0xaa55)		// wrong magic number
      return false;

  flags = buffer[446 + i*16];
  if(flags!=0 && flags!=0x80)
					// inconsistency: partition is either
    return false;			// bootable or non-bootable

  p.flags = flags;
  p.type = buffer[446 + i*16 + 4];
  p.start = s.blocksize*(unsigned long long)le_long(buffer + 446 + i*16 + 8);
  p.size = s.blocksize*(unsigned long long)le_long(buffer + 446 + i*16 + 12);

  return true;
}

static bool analyse_dospart(source & s,
		unsigned char flags,
		unsigned char type,
		hwNode & partition);

static bool analyse_dosextpart(source & s,
		unsigned char flags,
		unsigned char type,
		hwNode & extpart)
{
  source extendedpart = s;
  int i = 0;
  dospartition pte[2];			// we only read entries #0 and #1

  if(!is_extended(type))	// this is not an extended partition
    return false;

  extpart.setDescription("Extended partition");
  extpart.addCapability("extended", "Extended partition");
  extpart.addCapability("partitioned", "Partitioned disk");
  extpart.addCapability("partitioned:extended", "Extended partition");
  extpart.setSize(extpart.getCapacity());

  extpart.describeCapability("nofs", "No filesystem");

  do
  {
    for(i=0; i<2; i++)
      if(!read_dospartition(extendedpart, i, pte[i]))
        return false;

    if((pte[0].type == 0) || (pte[0].size == 0))
      return true;
    else
    {
      hwNode partition("logicalvolume", hw::disk);
      source spart = extendedpart;

      spart.offset = extendedpart.offset + pte[0].start;
      spart.size = pte[0].size;
 
      partition.setPhysId(lastlogicalpart);
      partition.setCapacity(spart.size);

      if(analyse_dospart(spart, pte[0].flags, pte[0].type, partition))
      {
        guess_logicalname(spart, extpart, lastlogicalpart, partition);
        extpart.addChild(partition);
        lastlogicalpart++;
      }
    }

    if((pte[1].type == 0) || (pte[1].size == 0))
      return true;
    else
    {
      extendedpart.offset = s.offset + pte[1].start;
      extendedpart.size = pte[1].size;
      if(!is_extended(pte[1].type))
        return false;
    }
  } while(true);

  return true;
}

static bool analyse_dospart(source & s,
		unsigned char flags,
		unsigned char type,
		hwNode & partition)
{
  int i = 0;

  if(is_extended(type))
    return analyse_dosextpart(s, flags, type, partition);

  if(flags!=0 && flags!=0x80)	// inconsistency: partition is either bootable or non-bootable
    return false;

  if(s.offset==0 || s.size==0) // unused entry
    return false;

  partition.setCapacity(s.size);

  if(flags == 0x80)
    partition.addCapability("bootable", "Bootable partition (active)");

  while(dos_sys_types[i].id)
  {
    if(dos_sys_types[i].type == type)
    {
      partition.setDescription(string(dos_sys_types[i].description)+" partition");
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

  scan_lvm(partition, s);

  return true;
}

#define UNUSED_ENTRY_GUID    \
    ((efi_guid_t) { 0x00000000, 0x0000, 0x0000, 0x00, 00, \
                    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }})
#define PARTITION_SYSTEM_GUID \
    ((efi_guid_t) {  (0xC12A7328),  (0xF81F), \
                     (0x11d2), 0xBA, 0x4B, \
                    { 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B }})
#define LEGACY_MBR_PARTITION_GUID \
    ((efi_guid_t) {  (0x024DEE41),  (0x33E7), \
                     (0x11d3), 0x9D, 0x69, \
                    { 0x00, 0x08, 0xC7, 0x81, 0xF3, 0x9F }})
#define PARTITION_MSFT_RESERVED_GUID \
    ((efi_guid_t) {  (0xe3c9e316),  (0x0b5c), \
                     (0x4db8), 0x81, 0x7d, \
                    { 0xf9, 0x2d, 0xf0, 0x02, 0x15, 0xae }})
#define PARTITION_LDM_DATA_GUID \
    ((efi_guid_t) {  (0xAF9B60A0),  (0x1431), \
                     (0x4f62), 0xbc, 0x68, \
                    { 0x33, 0x11, 0x71, 0x4a, 0x69, 0xad }})
#define PARTITION_LDM_METADATA_GUID \
    ((efi_guid_t) {  (0x5808C8AA),  (0x7E8F), \
                     (0x42E0), 0x85, 0xd2, \
                    { 0xe1, 0xe9, 0x04, 0x34, 0xcf, 0xb3 }})
#define PARTITION_BASIC_DATA_GUID \
    ((efi_guid_t) {  (0xEBD0A0A2),  (0xB9E5), \
                     (0x4433), 0x87, 0xC0, \
                    { 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 }})
#define PARTITION_RAID_GUID \
    ((efi_guid_t) {  (0xa19d880f),  (0x05fc), \
                     (0x4d3b), 0xa0, 0x06, \
                    { 0x74, 0x3f, 0x0f, 0x84, 0x91, 0x1e }})
#define PARTITION_SWAP_GUID \
    ((efi_guid_t) {  (0x0657fd6d),  (0xa4ab), \
                     (0x43c4), 0x84, 0xe5, \
                    { 0x09, 0x33, 0xc8, 0x4b, 0x4f, 0x4f }})
#define PARTITION_LVM_GUID \
    ((efi_guid_t) {  (0xe6d6d379),  (0xf507), \
                     (0x44c2), 0xa2, 0x3c, \
                    { 0x23, 0x8f, 0x2a, 0x3d, 0xf9, 0x28 }})
#define PARTITION_RESERVED_GUID \
    ((efi_guid_t) {  (0x8da63339),  (0x0007), \
                     (0x60c0), 0xc4, 0x36, \
                    { 0x08, 0x3a, 0xc8, 0x23, 0x09, 0x08 }})
#define PARTITION_HPUX_DATA_GUID \
    ((efi_guid_t) {  (0x75894c1e),  (0x3aeb), \
                     (0x11d3), 0xb7, 0xc1, \
                    { 0x7b, 0x03, 0xa0, 0x00, 0x00, 0x00 }})
#define PARTITION_HPSERVICE_GUID \
    ((efi_guid_t) {  (0xe2a1e728),  (0x32e3), \
                     (0x11d6), 0xa6, 0x82, \
                    { 0x7b, 0x03, 0xa0, 0x00, 0x00, 0x00 }})
#define PARTITION_APPLE_HFS_GUID \
    ((efi_guid_t) {  (0x48465300),  (0x0000), \
                     (0x11aa), 0xaa, 0x11, \
                    { 0x00, 0x30, 0x65, 0x43, 0xec, 0xac }})
#define PARTITION_FREEBSD_GUID \
    ((efi_guid_t) {  (0x516E7CB4),  (0x6ECF), \
                     (0x11d6), 0x8f, 0xf8, \
                    { 0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b }})

#define PARTITION_PRECIOUS 1
#define PARTITION_READONLY (1LL << 60)
#define PARTITION_HIDDEN   (1LL << 62)
#define PARTITION_NOMOUNT  (1LL << 63)


/* returns the EFI-style CRC32 value for buf
 * Dec 5, 2000 Matt Domsch <Matt_Domsch@dell.com>
 * - Copied crc32.c from the linux/drivers/net/cipe directory.
 * - Now pass seed as an arg
 * - changed unsigned long to uint32_t, added #include<stdint.h>
 * - changed len to be an unsigned long
 * - changed crc32val to be a register
 * - License remains unchanged!  It's still GPL-compatable!
 */

  /* ============================================================= */
  /*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or       */
  /*  code or tables extracted from it, as desired without restriction.     */
  /*                                                                        */
  /*  First, the polynomial itself and its table of feedback terms.  The    */
  /*  polynomial is                                                         */
  /*  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0   */
  /*                                                                        */
  /*  Note that we take it "backwards" and put the highest-order term in    */
  /*  the lowest-order bit.  The X^32 term is "implied"; the LSB is the     */
  /*  X^31 term, etc.  The X^0 term (usually shown as "+1") results in      */
  /*  the MSB being 1.                                                      */
  /*                                                                        */
  /*  Note that the usual hardware shift register implementation, which     */
  /*  is what we're using (we're merely optimizing it by doing eight-bit    */
  /*  chunks at a time) shifts bits into the lowest-order term.  In our     */
  /*  implementation, that means shifting towards the right.  Why do we     */
  /*  do it this way?  Because the calculated CRC must be transmitted in    */
  /*  order from highest-order term to lowest-order term.  UARTs transmit   */
  /*  characters in order from LSB to MSB.  By storing the CRC this way,    */
  /*  we hand it to the UART in the order low-byte to high-byte; the UART   */
  /*  sends each low-bit to hight-bit; and the result is transmission bit   */
  /*  by bit from highest- to lowest-order term without requiring any bit   */
  /*  shuffling on our part.  Reception works similarly.                    */
  /*                                                                        */
  /*  The feedback terms table consists of 256, 32-bit entries.  Notes:     */
  /*                                                                        */
  /*      The table can be generated at runtime if desired; code to do so   */
  /*      is shown later.  It might not be obvious, but the feedback        */
  /*      terms simply represent the results of eight shift/xor opera-      */
  /*      tions for all combinations of data and CRC register values.       */
  /*                                                                        */
  /*      The values must be right-shifted by eight bits by the "updcrc"    */
  /*      logic; the shift must be unsigned (bring in zeroes).  On some     */
  /*      hardware you could probably optimize the shift in assembler by    */
  /*      using byte-swap instructions.                                     */
  /*      polynomial $edb88320                                              */
  /*                                                                        */
  /*  --------------------------------------------------------------------  */

static uint32_t crc32_tab[] = {
      0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
      0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
      0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
      0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
      0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
      0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
      0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
      0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
      0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
      0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
      0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
      0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
      0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
      0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
      0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
      0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
      0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
      0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
      0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
      0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
      0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
      0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
      0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
      0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
      0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
      0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
      0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
      0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
      0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
      0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
      0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
      0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
      0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
      0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
      0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
      0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
      0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
      0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
      0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
      0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
      0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
      0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
      0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
      0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
      0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
      0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
      0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
      0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
      0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
      0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
      0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
      0x2d02ef8dL
   };

/* Return a 32-bit CRC of the contents of the buffer. */

uint32_t
__efi_crc32(const void *buf, unsigned long len, uint32_t seed)
{
  unsigned long i;
  register uint32_t crc32val;
  const unsigned char *s = (const unsigned char *)buf;

  crc32val = seed;
  for (i = 0;  i < len;  i ++)
    {
      crc32val =
	crc32_tab[(crc32val ^ s[i]) & 0xff] ^
	  (crc32val >> 8);
    }
  return crc32val;
}

/*
 *      This function uses the crc32 function by Gary S. Brown,
 * but seeds the function with ~0, and xor's with ~0 at the end.
 */
static inline uint32_t
efi_crc32(const void *buf, unsigned long len)
{
        return (__efi_crc32(buf, len, ~0L) ^ ~0L);
}

static efi_guid_t read_efi_guid(uint8_t *buffer)
{
  efi_guid_t result;

  memset(&result, 0, sizeof(result));

  result.time_low = le_long(buffer);
  result.time_mid = le_short(buffer+4);
  result.time_hi_and_version = le_short(buffer+4+2);
  result.clock_seq_hi_and_reserved = *(buffer+4+2+2);
  result.clock_seq_low = *(buffer+4+2+2+1);
  memcpy(result.node, buffer+4+2+2+1+1, sizeof(result.node));

  return result;
}

bool operator ==(const efi_guid_t & guid1, const efi_guid_t & guid2)
{
  return (guid1.time_low == guid2.time_low) &&
	(guid1.time_mid == guid2.time_mid) && 
	(guid1.time_hi_and_version == guid2.time_hi_and_version) && 
	(guid1.clock_seq_hi_and_reserved == guid2.clock_seq_hi_and_reserved) &&
	(guid1.clock_seq_low == guid2.clock_seq_low) &&
	(memcmp(guid1.node, guid2.node, sizeof(guid1.node))==0);
}

static string tostring(const efi_guid_t & guid)
{
  char buffer[50];

  snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", guid.time_low, guid.time_mid,guid.time_hi_and_version,guid.clock_seq_hi_and_reserved,guid.clock_seq_low,guid.node[0],guid.node[1],guid.node[2],guid.node[3],guid.node[4],guid.node[5]);
  return string(buffer);
}

static bool detect_gpt(source & s, hwNode & n)
{
  static uint8_t buffer[BLOCKSIZE];
  static gpth gpt_header;
  uint32_t i = 0;
  char gpt_version[8];
  uint8_t *partitions = NULL;
  uint8_t type;

  if(s.offset!=0)
    return false;	// partition tables must be at the beginning of the disk

  if(readlogicalblocks(s, buffer, GPT_PMBR_LBA, 1)!=1)	// read the first sector
    return false;

  if(le_short(buffer+510)!=0xaa55)		// wrong magic number
    return false;

  for(i=0; i<4; i++)
  {
    type = buffer[446 + i*16 + 4];

    if((type != 0) && (type != EFI_PMBR_OSTYPE_EFI))
      return false;	// the EFI pseudo-partition must be the only partition
  }

  if(readlogicalblocks(s, buffer, GPT_PRIMARY_HEADER_LBA, 1)!=1)	// read the second sector
    return false;				// (partition table header)

  gpt_header.Signature = le_longlong(buffer);
  gpt_header.Revision = be_long(buffer + 0x8); // big endian so that 1.0 -> 0x100
  gpt_header.HeaderSize = le_long(buffer + 0xc);
  gpt_header.HeaderCRC32 = le_long(buffer + 0x10);
  gpt_header.MyLBA = le_longlong(buffer + 0x18);
  gpt_header.AlternateLBA = le_longlong(buffer + 0x20);
  gpt_header.FirstUsableLBA = le_longlong(buffer + 0x28);
  gpt_header.LastUsableLBA = le_longlong(buffer + 0x30);
  gpt_header.DiskGUID = read_efi_guid(buffer + 0x38);
  gpt_header.PartitionEntryLBA = le_longlong(buffer + 0x48);
  gpt_header.NumberOfPartitionEntries = le_long(buffer + 0x50);
  gpt_header.SizeOfPartitionEntry = le_long(buffer + 0x54);
  gpt_header.PartitionEntryArrayCRC32 = le_long(buffer + 0x58);


  memset(buffer + 0x10, 0, sizeof(gpt_header.HeaderCRC32)); // zero-out the CRC32 before re-calculating it
  if(gpt_header.Signature != GPT_HEADER_SIGNATURE)
    return false;

  if(efi_crc32(buffer, 92) != gpt_header.HeaderCRC32)
    return false;		// check CRC32

  snprintf(gpt_version, sizeof(gpt_version), "%d.%02d", (gpt_header.Revision >> 8), (gpt_header.Revision & 0xff));
  
  n.addCapability("gpt-"+string(gpt_version), "GUID Partition Table version "+string(gpt_version));
  n.addHint("partitions", gpt_header.NumberOfPartitionEntries);
  n.setConfig("guid", tostring(gpt_header.DiskGUID));
  n.setHandle("GUID:" + tostring(gpt_header.DiskGUID));
  n.addHint("guid", tostring(gpt_header.DiskGUID));

  partitions = (uint8_t*)malloc(gpt_header.NumberOfPartitionEntries * gpt_header.SizeOfPartitionEntry + BLOCKSIZE);
  if(!partitions)
    return false;
  memset(partitions, 0, gpt_header.NumberOfPartitionEntries * gpt_header.SizeOfPartitionEntry + BLOCKSIZE);
  readlogicalblocks(s, partitions,
	gpt_header.PartitionEntryLBA,
	(gpt_header.NumberOfPartitionEntries * gpt_header.SizeOfPartitionEntry)/BLOCKSIZE + 1);

  for(i=0; i<gpt_header.NumberOfPartitionEntries; i++)
  {
    hwNode partition("volume", hw::disk);
    efipartition p;

    p.PartitionTypeGUID = read_efi_guid(partitions + gpt_header.SizeOfPartitionEntry * i);
    p.PartitionGUID = read_efi_guid(partitions + gpt_header.SizeOfPartitionEntry * i + 0x10);
    p.StartingLBA = le_longlong(partitions + gpt_header.SizeOfPartitionEntry * i + 0x20);
    p.EndingLBA = le_longlong(partitions + gpt_header.SizeOfPartitionEntry * i + 0x28);
    p.Attributes = le_longlong(partitions + gpt_header.SizeOfPartitionEntry * i + 0x30);
    for(int j=0; j<36; j++)
    {
      wchar_t c = le_short(partitions + gpt_header.SizeOfPartitionEntry * i + 0x38 + 2*j);
      if(!c)
        break;
      else
        p.PartitionName += utf8(c);
    }

    if(!(p.PartitionTypeGUID == UNUSED_ENTRY_GUID))
    {
      source spart = s;
      if(p.PartitionTypeGUID == LEGACY_MBR_PARTITION_GUID)
      {
        partition.setDescription("MBR partition scheme");
        partition.addCapability("nofs");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_BASIC_DATA_GUID)
        partition.setDescription("Data partition");
      else
      if(p.PartitionTypeGUID == PARTITION_SWAP_GUID)
      {
        partition.setDescription("Linux Swap partition");
        partition.addCapability("nofs");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_RAID_GUID)
      {
        partition.setDescription("Linux RAID partition");
        partition.addCapability("multi");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_LVM_GUID)
      {
        partition.setDescription("Linux LVM physical volume");
        partition.addCapability("multi");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_LDM_METADATA_GUID)
      {
        partition.setDescription("Windows LDM configuration");
        partition.addCapability("nofs");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_LDM_DATA_GUID)
      {
        partition.setDescription("Windows LDM data partition");
        partition.addCapability("multi");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_MSFT_RESERVED_GUID)
      {
        partition.setDescription("Windows reserved partition");
        partition.addCapability("nofs");
      }
      else
      if(p.PartitionTypeGUID == LEGACY_MBR_PARTITION_GUID)
      {
        partition.setDescription("MBR partition scheme");
        partition.addCapability("nofs");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_SYSTEM_GUID)
      {
        partition.setDescription("System partition");
        partition.addCapability("boot");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_HPUX_DATA_GUID)
      {
        partition.setDescription("HP-UX data partition");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_HPSERVICE_GUID)
      {
        partition.setDescription("HP-UX service partition");
        partition.addCapability("nofs");
        partition.addCapability("boot");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_RESERVED_GUID)
      {
        partition.setDescription("Reserved partition");
        partition.addCapability("nofs");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_APPLE_HFS_GUID)
      {
        partition.setDescription("Apple HFS partition");
      }
      else
      if(p.PartitionTypeGUID == PARTITION_FREEBSD_GUID)
      {
        partition.setDescription("FreeBSD partition");
      }
      else
          partition.setDescription("EFI partition");
      partition.setPhysId(i+1);
      partition.setCapacity(BLOCKSIZE * (p.EndingLBA - p.StartingLBA));
      partition.addHint("type", tostring(p.PartitionTypeGUID));
      partition.addHint("guid", tostring(p.PartitionGUID));
      partition.setSerial(tostring(p.PartitionGUID));
      partition.setHandle("GUID:" + tostring(p.PartitionGUID));
      partition.setConfig("name", p.PartitionName);
      if(p.Attributes && PARTITION_PRECIOUS)
        partition.addCapability("precious", "This partition is required for the platform to function");
      if(p.Attributes && PARTITION_READONLY)
        partition.addCapability("readonly", "Read-only partition");
      if(p.Attributes && PARTITION_HIDDEN)
        partition.addCapability("hidden");
      if(p.Attributes && PARTITION_NOMOUNT)
        partition.addCapability("nomount", "No automatic mount");

      partition.describeCapability("nofs", "No filesystem");
      partition.describeCapability("boot", "Contains boot code");
      partition.describeCapability("multi", "Multi-volumes");
      partition.describeCapability("hidden", "Hidden partition");

      spart.blocksize = s.blocksize;
      spart.offset = s.offset + p.StartingLBA*spart.blocksize;
      spart.size = (p.EndingLBA - p.StartingLBA)*spart.blocksize;
      guess_logicalname(spart, n, i+1, partition);
      scan_lvm(partition, spart);
      n.addChild(partition);
    }
  }
  
  free(partitions);

  return true;
}
static bool detect_dosmap(source & s, hwNode & n)
{
  static unsigned char buffer[BLOCKSIZE];
  int i = 0;
  unsigned char flags;
  unsigned char type;
  unsigned long long start, size;
  bool partitioned = false;

  if(s.offset!=0)
    return false;	// partition tables must be at the beginning of the disk

  if(readlogicalblocks(s, buffer, 0, 1)!=1)	// read the first sector
    return false;

  if(le_short(buffer+510)!=0xaa55)		// wrong magic number
    return false;

  lastlogicalpart = 5;

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
 
    partition.setDescription("Primary partition");
    partition.addCapability("primary", "Primary partition");
    partition.setPhysId(i+1);
    partition.setCapacity(spart.size);

    if(analyse_dospart(spart, flags, type, partition))
    {
      guess_logicalname(spart, n, i+1, partition);
      n.addChild(partition);
      partitioned = true;
    }
  }

  return partitioned;
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
    {
      partition.addHint("icon", string("md"));
      partition.addCapability("multi");
    }
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

      scan_lvm(partition, spart);
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

  s.diskname = n.getLogicalName();
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

  if(!medium->isCapable("partitioned"))
  {
    scan_lvm(*medium, s);
  }

  close(fd);

  //if(medium != &n) free(medium);

  return false;
}

