/*
 * volumes.cc
 *
 */

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include "version.h"
#include "config.h"
#include "options.h"
#include "volumes.h"
#include "blockio.h"
#include "lvm.h"
#include "fat.h"
#include "osutils.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

__ID("@(#) $Id$");

struct fstypes
{
  const char * id;
  const char * description;
  const char * capabilities;
  bool (*detect)(hwNode & n, source & s);
};

static bool detect_luks(hwNode & n, source & s);
static bool detect_ext2(hwNode & n, source & s);
static bool detect_reiserfs(hwNode & n, source & s);
static bool detect_fat(hwNode & n, source & s);
static bool detect_hfsx(hwNode & n, source & s);
static bool detect_hfs(hwNode & n, source & s);
static bool detect_apfs(hwNode & n, source & s);
static bool detect_ntfs(hwNode & n, source & s);
static bool detect_swap(hwNode & n, source & s);

static struct fstypes fs_types[] =
{
  {"blank", "Blank", "", NULL},
  {"fat", "Windows FAT", "", detect_fat},
  {"ntfs", "Windows NTFS", "secure", detect_ntfs},
  {"hpfs", "OS/2 HPFS", "secure", NULL},
  {"ext2", "EXT2/EXT3", "secure", detect_ext2},
  {"reiserfs", "Linux ReiserFS", "secure,journaled", detect_reiserfs},
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
  {"hfsplus", "MacOS HFS+", "secure,journaled", detect_hfsx},
  {"hfs", "MacOS HFS", "", detect_hfs},
  {"apfs", "MacOS APFS", "", detect_apfs},
  {"luks", "Linux Unified Key Setup", "encrypted", detect_luks},
  {"swap", "Linux swap", "", detect_swap},
  { NULL, NULL, NULL, NULL }
};

struct ext2_super_block {
        uint32_t   s_inodes_count;         /* Inodes count */
        uint32_t   s_blocks_count;         /* Blocks count */
        uint32_t   s_r_blocks_count;       /* Reserved blocks count */
        uint32_t   s_free_blocks_count;    /* Free blocks count */
        uint32_t   s_free_inodes_count;    /* Free inodes count */
        uint32_t   s_first_data_block;     /* First Data Block */
        uint32_t   s_log_block_size;       /* Block size */
        int32_t   s_log_frag_size;        /* Fragment size */
        uint32_t   s_blocks_per_group;     /* # Blocks per group */
        uint32_t   s_frags_per_group;      /* # Fragments per group */
        uint32_t   s_inodes_per_group;     /* # Inodes per group */
        uint32_t   s_mtime;                /* Mount time */
        uint32_t   s_wtime;                /* Write time */
        uint16_t   s_mnt_count;            /* Mount count */
        int16_t   s_max_mnt_count;        /* Maximal mount count */
        uint16_t   s_magic;                /* Magic signature */
        uint16_t   s_state;                /* File system state */
        uint16_t   s_errors;               /* Behaviour when detecting errors */
        uint16_t   s_minor_rev_level;      /* minor revision level */
        uint32_t   s_lastcheck;            /* time of last check */
        uint32_t   s_checkinterval;        /* max. time between checks */
        uint32_t   s_creator_os;           /* OS */
        uint32_t   s_rev_level;            /* Revision level */
        uint16_t   s_def_resuid;           /* Default uid for reserved blocks */
        uint16_t   s_def_resgid;           /* Default gid for reserved blocks */
        /*
         * These fields are for EXT2_DYNAMIC_REV superblocks only.
         *
         * Note: the difference between the compatible feature set and
         * the incompatible feature set is that if there is a bit set
         * in the incompatible feature set that the kernel doesn't
         * know about, it should refuse to mount the filesystem.
         *
         * e2fsck's requirements are more strict; if it doesn't know
         * about a feature in either the compatible or incompatible
         * feature set, it must abort and not try to meddle with
         * things it doesn't understand...
         */
        uint32_t   s_first_ino;            /* First non-reserved inode */
        uint16_t   s_inode_size;           /* size of inode structure */
        uint16_t   s_block_group_nr;       /* block group # of this superblock */
        uint32_t   s_feature_compat;       /* compatible feature set */
        uint32_t   s_feature_incompat;     /* incompatible feature set */
        uint32_t   s_feature_ro_compat;    /* readonly-compatible feature set */
        uint8_t    s_uuid[16];             /* 128-bit uuid for volume */
        char    s_volume_name[16];      /* volume name */
        char    s_last_mounted[64];     /* directory where last mounted */
        uint32_t   s_algorithm_usage_bitmap; /* For compression */
        /*
         * Performance hints.  Directory preallocation should only
         * happen if the EXT2_FEATURE_COMPAT_DIR_PREALLOC flag is on.
         */
        uint8_t    s_prealloc_blocks;      /* Nr of blocks to try to preallocate*/
        uint8_t    s_prealloc_dir_blocks;  /* Nr to preallocate for dirs */
        uint16_t   s_reserved_gdt_blocks;  /* Per group table for online growth */
        /*
         * Journaling support valid if EXT2_FEATURE_COMPAT_HAS_JOURNAL set.
         */
        uint8_t    s_journal_uuid[16];     /* uuid of journal superblock */
        uint32_t   s_journal_inum;         /* inode number of journal file */
        uint32_t   s_journal_dev;          /* device number of journal file */
        uint32_t   s_last_orphan;          /* start of list of inodes to delete */
        uint32_t   s_hash_seed[4];         /* HTREE hash seed */
        uint8_t    s_def_hash_version;     /* Default hash version to use */
        uint8_t    s_jnl_backup_type;      /* Default type of journal backup */
        uint16_t   s_desc_size;            /* Group desc. size: INCOMPAT_64BIT */
        uint32_t   s_default_mount_opts;
        uint32_t   s_first_meta_bg;        /* First metablock group */
        uint32_t   s_mkfs_time;            /* When the filesystem was created */
        uint32_t   s_jnl_blocks[17];       /* Backup of the journal inode */
        uint32_t   s_blocks_count_hi;      /* Blocks count high 32bits */
        uint32_t   s_r_blocks_count_hi;    /* Reserved blocks count high 32 bits*/
        uint32_t   s_free_blocks_hi;       /* Free blocks count */
        uint16_t   s_min_extra_isize;      /* All inodes have at least # bytes */
        uint16_t   s_want_extra_isize;     /* New inodes should reserve # bytes */
        uint32_t   s_flags;                /* Miscellaneous flags */
        uint16_t   s_raid_stride;          /* RAID stride */
        uint16_t   s_mmp_interval;         /* # seconds to wait in MMP checking */
        uint64_t   s_mmp_block;            /* Block for multi-mount protection */
        uint32_t   s_raid_stripe_width;    /* blocks on all data disks (N*stride)*/
        uint32_t   s_reserved[163];        /* Padding to the end of the block */
};

/*
 * The second extended file system magic number
 */
#define EXT2_SUPER_MAGIC	0xEF53

#define EXT2_DEFAULT_BLOCK_SIZE	1024

/*
 * File system states
 */
#define EXT2_VALID_FS                   0x0001  /* Unmounted cleanly */
#define EXT2_ERROR_FS                   0x0002  /* Errors detected */

/*
 * Codes for operating systems
 */
#define EXT2_OS_LINUX           0
#define EXT2_OS_HURD            1
#define EXT2_OS_MASIX           2
#define EXT2_OS_FREEBSD         3
#define EXT2_OS_LITES           4

#define EXT2_FEATURE_COMPAT_DIR_PREALLOC        0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES       0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL         0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR            0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INODE        0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX           0x0020
#define EXT2_FEATURE_COMPAT_LAZY_BG             0x0040

#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER     0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE       0x0002
/* #define EXT2_FEATURE_RO_COMPAT_BTREE_DIR     0x0004 not used */
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE        0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM         0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK        0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE      0x0040

#define EXT2_FEATURE_INCOMPAT_COMPRESSION       0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE          0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER           0x0004 /* Needs recovery */
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV       0x0008 /* Journal device */
#define EXT2_FEATURE_INCOMPAT_META_BG           0x0010
#define EXT3_FEATURE_INCOMPAT_EXTENTS           0x0040
#define EXT4_FEATURE_INCOMPAT_EXTENTS           0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT             0x0080
#define EXT4_FEATURE_INCOMPAT_MMP               0x0100

static string uuid(const uint8_t s_uuid[16])
{
  char buffer[50];

  snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", s_uuid[0], s_uuid[1], s_uuid[2], s_uuid[3], s_uuid[4], s_uuid[5], s_uuid[6], s_uuid[7], s_uuid[8], s_uuid[9], s_uuid[10], s_uuid[11], s_uuid[12], s_uuid[13], s_uuid[14], s_uuid[15]);
  return string(buffer);
}

static string datetime(time_t timestamp, bool utc = true)
{
  char buffer[50];
  tm ltime;

  if(timestamp)
  {
    if(utc)
      localtime_r(&timestamp, &ltime);
    else
      gmtime_r(&timestamp, &ltime);
    strftime(buffer, sizeof(buffer), "%F %T", &ltime);
    return string(buffer);
  }
  else
    return "";
}

static bool detect_ext2(hwNode & n, source & s)
{
  static char buffer[EXT2_DEFAULT_BLOCK_SIZE];
  source ext2volume;
  ext2_super_block *sb = (ext2_super_block*)buffer;
  uint32_t ext2_version = 0;
  time_t mtime, wtime, mkfstime;
  unsigned long long blocksize = EXT2_DEFAULT_BLOCK_SIZE;

  ext2volume = s;
  ext2volume.blocksize = EXT2_DEFAULT_BLOCK_SIZE;

  if(readlogicalblocks(ext2volume, buffer, 1, 1)!=1) // read the second block
    return false;

  if(le_short(&sb->s_magic) != EXT2_SUPER_MAGIC)	// wrong magic number
    return false;

  blocksize = 1024LL*(1 << le_long(&sb->s_log_block_size));
  if(blocksize < EXT2_DEFAULT_BLOCK_SIZE)
    blocksize = EXT2_DEFAULT_BLOCK_SIZE;
  n.setSize(blocksize * le_long(&sb->s_blocks_count));

  switch(le_long(&sb->s_creator_os))
  {
    case EXT2_OS_LINUX:
	n.setVendor("Linux");
	break;
    case EXT2_OS_HURD:
	n.setVendor("GNU Hurd");
	break;
    case EXT2_OS_MASIX:
	n.setVendor("MASIX");
	break;
    case EXT2_OS_FREEBSD:
	n.setVendor("FreeBSD");
	break;
    case EXT2_OS_LITES:
	n.setVendor("LITES");
	break;
  }

  ext2_version = le_short(&sb->s_rev_level);
  n.setVersion(tostring(ext2_version)+"."+tostring(le_short(&sb->s_minor_rev_level)));

  if (enabled("output:time"))
  {
    mtime = (time_t)le_long(&sb->s_mtime);
    n.setConfig("mounted", datetime(mtime));
    wtime = (time_t)le_long(&sb->s_wtime);
    n.setConfig("modified", datetime(wtime));
  }

  if(ext2_version >= 1)
  {
    switch(le_short(&sb->s_state))
    {
      case EXT2_VALID_FS:
	n.setConfig("state", "clean");
	break;
      case EXT2_ERROR_FS:
	n.setConfig("state", "unclean");
	break;
      default:
	n.setConfig("state", "unknown");
    }
    n.setConfig("label", hw::strip(string(sb->s_volume_name, sizeof(sb->s_volume_name))));
    n.setConfig("lastmountpoint", hw::strip(string(sb->s_last_mounted, sizeof(sb->s_last_mounted))));
    n.setSerial(hw::strip(uuid(sb->s_uuid)));

    if(le_long(&sb->s_feature_compat) & EXT3_FEATURE_COMPAT_HAS_JOURNAL)
    {
      n.addCapability("journaled");
      
      mkfstime = (time_t)le_long(&sb->s_mkfs_time);
      n.setConfig("created", datetime(mkfstime));
    }
    if(le_long(&sb->s_feature_compat) & EXT2_FEATURE_COMPAT_EXT_ATTR)
      n.addCapability("extended_attributes", _("Extended Attributes"));
    if(le_long(&sb->s_feature_ro_compat) & EXT2_FEATURE_RO_COMPAT_LARGE_FILE)
      n.addCapability("large_files", _("4GB+ files"));
    if(le_long(&sb->s_feature_ro_compat) & EXT4_FEATURE_RO_COMPAT_HUGE_FILE)
      n.addCapability("huge_files", _("16TB+ files"));
    if(le_long(&sb->s_feature_ro_compat) & EXT4_FEATURE_RO_COMPAT_DIR_NLINK)
      n.addCapability("dir_nlink", _("directories with 65000+ subdirs"));
    if(le_long(&sb->s_feature_incompat) & EXT3_FEATURE_INCOMPAT_RECOVER)
      n.addCapability("recover", _("needs recovery"));
    if(le_long(&sb->s_feature_incompat) & EXT4_FEATURE_INCOMPAT_64BIT)
      n.addCapability("64bit", _("64bit filesystem"));
    if(le_long(&sb->s_feature_incompat) & EXT4_FEATURE_INCOMPAT_EXTENTS)
      n.addCapability("extents", _("extent-based allocation"));
  }

  if(n.isCapable("journaled"))
  {
    if(n.isCapable("huge_files") || n.isCapable("64bit") || n.isCapable("extents") || n.isCapable("dir_nlink"))
    {
      n.addCapability("ext4");
      n.setDescription(_("EXT4 volume"));
      n.setConfig("filesystem", "ext4");
    }
    else
    {
      n.addCapability("ext3");
      n.setDescription(_("EXT3 volume"));
      n.setConfig("filesystem", "ext3");
    }
  }

  return true;
}

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

  n.addCapability("encrypted", _("Encrypted volume"));
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

#define REISERFSBLOCKSIZE 0x1000

static bool detect_reiserfs(hwNode & n, source & s)
{
  static char buffer[REISERFSBLOCKSIZE];
  source reiserfsvolume;
  string magic;
  long long blocksize = 0;

  reiserfsvolume = s;
  reiserfsvolume.blocksize = REISERFSBLOCKSIZE;
                                                  // read the 16th block
  if(readlogicalblocks(reiserfsvolume, buffer, 0x10, 1)!=1)
    return false;

  magic = hw::strip(string(buffer+52, 10));
  if(magic != "ReIsEr2Fs" && magic != "ReIsErFs" && magic != "ReIsEr3Fs")                    // wrong magic
    return false;

  n.setConfig("label", hw::strip(string(buffer + 0x64, 16)));

  blocksize = le_short(buffer+44);
  n.setSize(le_long(buffer)*blocksize);

  if(le_long(buffer+20) != 0)
    n.addCapability("journaled");

  n.setSerial(hw::strip(uuid((uint8_t*)buffer+0x54)));

  switch(le_long(buffer+64))
  {
    case 1:
	n.setConfig("hash", "tea");
	break;
    case 2:
	n.setConfig("hash", "yura");
	break;
    case 3:
	n.setConfig("hash", "r5");
  }

  switch(le_short(buffer+50))
  {
    case 1:
	n.setConfig("state", "clean");
	break;
    case 2:
	n.setConfig("state", "unclean");
	break;
    default:
	n.setConfig("state", "unknown");
  }

  if(magic == "ReIsErFs")
    n.setVersion("3.5");
  if(magic == "ReIsEr2Fs")
    n.setVersion("3.6");
  if(magic == "ReIsEr3Fs")
    n.setVersion("nonstandard " + tostring(le_short(buffer+72)));

  n.setCapacity(reiserfsvolume.size);
  return true;
}

static string dos_serial(unsigned long serial)
{
  char buffer[16];

  snprintf(buffer, sizeof(buffer), "%04lx-%04lx", serial >> 16, serial & 0xffff);

  return string(buffer);
}

static bool detect_fat(hwNode & n, source & s)
{
  static char buffer[BLOCKSIZE];
  source fatvolume;
  string magic, label;
  unsigned long long bytes_per_sector = 512;
  unsigned long long reserved_sectors = 0;
  unsigned long long hidden_sectors = 0;
  unsigned long long size = 0;
  unsigned long serial = 0;

  fatvolume = s;
  fatvolume.blocksize = BLOCKSIZE;

                                                  // read the first block
  if(readlogicalblocks(fatvolume, buffer, 0, 1)!=1)
    return false;

  if(be_short(buffer+0x1fe) != 0x55aa)		// no "boot" signature
    return false;

  magic = hw::strip(string(buffer+0x52, 8));
  if(magic != "FAT32")
    magic = hw::strip(string(buffer+0x36, 8));
  if(magic != "FAT12" && magic != "FAT16" && magic != "FAT32")                    // wrong magic
    return false;

  n.setVendor(hw::strip(string(buffer+0x3, 8)));
  n.setVersion(magic);

  bytes_per_sector = le_short(buffer+0xb);
  reserved_sectors = le_short(buffer+0xe);
  hidden_sectors = le_short(buffer+0x1c);
  size = le_long(buffer+0x20);
  size -= reserved_sectors + hidden_sectors;
  size *= bytes_per_sector;
  n.setSize(size);
  n.setCapacity(fatvolume.size);

  n.setConfig("FATs", buffer[0x10]);

  if(magic == "FAT32")
  {
    label = hw::strip(std::string(buffer+0x47, 11));
    serial = le_long(buffer+0x43);
  }
  else
  {
    label = hw::strip(std::string(buffer+0x2b, 11));
    serial = le_long(buffer+0x27);
  }

  if(label != "NO NAME" && label != "")
    n.setConfig("label", label);

  n.setSerial(dos_serial(serial));
  n.setDescription("");

  scan_fat(n, fatvolume);
  return true;
}

typedef uint32_t HFSCatalogNodeID;

struct HFSPlusExtentDescriptor {
    uint32_t                  startBlock;
    uint32_t                  blockCount;
};
typedef HFSPlusExtentDescriptor HFSPlusExtentRecord[8];

struct HFSPlusForkData {
    uint64_t                  logicalSize;
    uint32_t                  clumpSize;
    uint32_t                  totalBlocks;
    HFSPlusExtentRecord     extents;
};

struct HFSPlusVolumeHeader {
    uint16_t              signature;
    uint16_t              version;
    uint32_t              attributes;
    uint32_t              lastMountedVersion;
    uint32_t              journalInfoBlock;
 
    uint32_t              createDate;
    uint32_t              modifyDate;
    uint32_t              backupDate;
    uint32_t              checkedDate;
 
    uint32_t              fileCount;
    uint32_t              folderCount;
 
    uint32_t              blockSize;
    uint32_t              totalBlocks;
    uint32_t              freeBlocks;
 
    uint32_t              nextAllocation;
    uint32_t              rsrcClumpSize;
    uint32_t              dataClumpSize;
    HFSCatalogNodeID    nextCatalogID;
 
    uint32_t              writeCount;
    uint64_t              encodingsBitmap;
 
    uint32_t              finderInfo[8];
 
    HFSPlusForkData     allocationFile;
    HFSPlusForkData     extentsFile;
    HFSPlusForkData     catalogFile;
    HFSPlusForkData     attributesFile;
    HFSPlusForkData     startupFile;
};

enum {
    /* Bits 0-6 are reserved */
    kHFSVolumeHardwareLockBit       =  7,
    kHFSVolumeUnmountedBit          =  8,
    kHFSVolumeSparedBlocksBit       =  9,
    kHFSVolumeNoCacheRequiredBit    = 10,
    kHFSBootVolumeInconsistentBit   = 11,
    kHFSCatalogNodeIDsReusedBit     = 12,
    kHFSVolumeJournaledBit          = 13,
    /* Bit 14 is reserved */
    kHFSVolumeSoftwareLockBit       = 15
    /* Bits 16-31 are reserved */
};

#define HFSBLOCKSIZE 1024

#define HFSTIMEOFFSET 2082844800UL // time difference between 1904-01-01 00:00:00 and 1970-01-01 00:00:00

static bool detect_hfsx(hwNode & n, source & s)
{
  static char buffer[HFSBLOCKSIZE];
  source hfsvolume;
  string magic;
  HFSPlusVolumeHeader *vol = (HFSPlusVolumeHeader*)buffer;
  uint16_t version = 0;
  uint32_t attributes = 0;
  time_t mkfstime, fscktime, wtime;

  hfsvolume = s;
  hfsvolume.blocksize = HFSBLOCKSIZE;

                                                  // read the second block
  if(readlogicalblocks(hfsvolume, buffer, 1, 1)!=1)
    return false;

  magic = hw::strip(string(buffer, 2));
  if((magic != "H+") && (magic != "HX"))		// wrong signature
    return false;

  version = be_short(&vol->version);
  if(version >= 5)
    n.addCapability("hfsx");

  magic = hw::strip(string(buffer+8, 4));
  if(magic == "10.0")
    n.setVendor("Mac OS X");
  if(magic == "8.10")
    n.setVendor("Mac OS");
  if(magic == "HFSJ")
    n.setVendor("Mac OS X (journaled)");
  if(magic == "fsck")
    n.setVendor("Mac OS X (fsck)");
  n.setConfig("lastmountedby", hw::strip(magic));

  n.setSize((unsigned long long)be_long(&vol->blockSize) * (unsigned long long)be_long(&vol->totalBlocks));
  n.setVersion(tostring(version));
 
  attributes = be_long(&vol->attributes);
  if(attributes & (1 << kHFSVolumeJournaledBit))
    n.addCapability("journaled");
  if(attributes & (1 << kHFSVolumeSoftwareLockBit))
    n.addCapability("ro");
  if(attributes & (1 << kHFSBootVolumeInconsistentBit))
    n.addCapability("recover");
  if(attributes & (1 << kHFSVolumeUnmountedBit))
    n.setConfig("state", "clean");
  else
    n.setConfig("state", "unclean");
  
  n.setSerial(uuid((uint8_t*)&vol->finderInfo[6]));	// finderInfo[6] and finderInfo[7] contain uuid

  if(vol->finderInfo[0])
    n.addCapability("bootable");
  if(vol->finderInfo[3])
    n.addCapability("macos", _("Contains a bootable Mac OS installation"));
  if(vol->finderInfo[5])
    n.addCapability("osx", _("Contains a bootable Mac OS X installation"));
  if(vol->finderInfo[0] && (vol->finderInfo[0] == vol->finderInfo[3]))
    n.setConfig("boot", "macos");
  if(vol->finderInfo[0] && (vol->finderInfo[0] == vol->finderInfo[5]))
    n.setConfig("boot", "osx");

  mkfstime = (time_t)(be_long(&vol->createDate) - HFSTIMEOFFSET);
  fscktime = (time_t)(be_long(&vol->checkedDate) - HFSTIMEOFFSET);
  wtime = (time_t)(be_long(&vol->modifyDate) - HFSTIMEOFFSET);
  n.setConfig("created", datetime(mkfstime, false));	// creation time uses local time
  if (enabled("output:time"))
  {
    n.setConfig("checked", datetime(fscktime));
    n.setConfig("modified", datetime(wtime));
  }

  return true;
}

/* Master Directory Block (HFS only) */

#pragma pack(2)
struct HFSMasterDirectoryBlock
{
  uint16_t drSigWord;           /* volume signature */

  uint32_t drCrDate;              /* date and time of volume creation */
  uint32_t drLsMod;               /* date and time of last modification */
  uint16_t drAtrb;                /* volume attributes */
  int16_t drNmFls;               /* number of files in root folder */
  uint16_t drVBMSt;               /* first block of volume bitmap */
  uint16_t drAllocPtr;            /* start of next allocation search */
  uint16_t drNmAlBlks;            /* number of allocation blocks in volume */
  int32_t drAlBlkSiz;            /* size (in bytes) of allocation blocks */
  int32_t drClpSiz;              /* default clump size */
  int16_t drAlBlSt;              /* first allocation block in volume */
  uint32_t drNxtCNID;             /* next unused catalog node ID */
  int16_t drFreeBks;             /* number of unused allocation blocks */
  char drVN[28];                /* volume name */
  uint32_t drVolBkUp;            /* date and time of last backup */
  uint16_t drVSeqNum;      /* backup sequence number */
  uint32_t drWrCnt;        /* fs write count */
  uint32_t drXTClpSiz;     /* clumpsize for the extents B-tree */
  uint32_t drCTClpSiz;     /* clumpsize for the catalog B-tree */
  uint16_t drNmRtDirs;     /* number of directories in the root directory */
  uint32_t drFilCnt;       /* number of files in the fs */
  uint32_t drDirCnt;       /* number of directories in the fs */
  uint32_t drFndrInfo[8]; /* data used by the Finder */
  uint16_t drEmbedSigWord; /* embedded volume signature */
  uint32_t drEmbedExtent;  /* starting block number (xdrStABN) 
                                   and number of allocation blocks 
                                   (xdrNumABlks) occupied by embedded
                                   volume */
  uint32_t drXTFlSize;     /* bytes in the extents B-tree */
  uint8_t  drXTExtRec[12]; /* extents B-tree's first 3 extents */
  uint32_t drCTFlSize;     /* bytes in the catalog B-tree */
  uint8_t  drCTExtRec[12]; /* catalog B-tree's first 3 extents */
};

#define HFS_ATTR_HW_LOCKED		(1 << 7) // Set if the volume is locked by hardware
#define HFS_ATTR_CLEAN			(1 << 8) // Set if the volume was successfully unmounted
#define HFS_ATTR_BAD_BLOCKS_SPARED	(1 << 9) // Set if the volume has had its bad blocks spared
#define HFS_ATTR_LOCKED			(1 << 15) // Set if the volume is locked by software

static bool detect_hfs(hwNode & n, source & s)
{
  static char buffer[HFSBLOCKSIZE];
  source hfsvolume;
  string magic;
  HFSMasterDirectoryBlock *vol = (HFSMasterDirectoryBlock*)buffer;
  uint16_t attributes = 0;
  time_t mkfstime, dumptime, wtime;

  hfsvolume = s;
  hfsvolume.blocksize = HFSBLOCKSIZE;

                                                  // read the second block
  if(readlogicalblocks(hfsvolume, buffer, 1, 1)!=1)
    return false;

  magic = hw::strip(string(buffer, 2));
  if((magic != "BD"))		// wrong signature
    return false;

  n.setSize((unsigned long long)be_short(&vol->drNmAlBlks) * (unsigned long long)be_long(&vol->drAlBlkSiz));
  n.setConfig("label", hw::strip(string(vol->drVN, sizeof(vol->drVN))));

  attributes = be_short(&vol->drAtrb);
  if(attributes & (HFS_ATTR_LOCKED | HFS_ATTR_HW_LOCKED))
    n.addCapability("ro");
  if(attributes & HFS_ATTR_CLEAN)
    n.setConfig("state", "clean");
  else
    n.setConfig("state", "unclean");
  if(vol->drFndrInfo[0])
    n.addCapability("bootable");

  mkfstime = (time_t)be_long(&vol->drCrDate);
  dumptime = (time_t)be_long(&vol->drVolBkUp);
  wtime = (time_t)be_long(&vol->drLsMod);
  if(mkfstime)
    n.setConfig("created", datetime(mkfstime - HFSTIMEOFFSET, false));	// all dates use local time
  if(dumptime)
    n.setConfig("backup", datetime(dumptime - HFSTIMEOFFSET, false));
  if(wtime && enabled("output:time"))
    n.setConfig("modified", datetime(wtime - HFSTIMEOFFSET, false));

  return true;
}

#define APFS_CONTAINER_SUPERBLOCK_TYPE 1
#define APFS_CONTAINER_SUPERBLOCK_SUBTYPE 0
#define APFS_STANDARD_BLOCK_SIZE 4096

/*
 * This struct is much longer than this, but this seems
 * to contain the useful bits (for now).
 *
 * All values are little-endian.
 */
struct apfs_super_block {
  // Generic part to all APFS objects
  uint64_t checksum;
  uint64_t oid;
  uint64_t xid;
  uint16_t type;
  uint16_t flags;
  uint16_t subtype;
  uint16_t pad;

  // Specific to container header
  uint32_t magic; // 'NXSB'
  uint32_t block_size;
  uint64_t block_count;
  uint64_t features;
  uint64_t read_only_features;
  uint64_t incompatible_features;
  uint8_t uuid[16];
};

static bool detect_apfs(hwNode & n, source & s)
{
  static char buffer[sizeof(apfs_super_block)];
  source apfsvolume;
  apfs_super_block *sb = (apfs_super_block*)buffer;
  unsigned long block_size;

  apfsvolume = s;

  if(readlogicalblocks(apfsvolume, buffer, 0, 1)!=1)
    return false;

  if(le_long(&sb->magic) != 0x4253584eu) // 'NXSB'
    return false;

  if(le_short(&sb->type) != APFS_CONTAINER_SUPERBLOCK_TYPE)
    return false;

  if(le_short(&sb->subtype) != APFS_CONTAINER_SUPERBLOCK_SUBTYPE)
    return false;

  if(le_short(&sb->pad) != 0)
    return false;

  /*
    * This check is pretty draconian, but should avoid false
    * positives. Can be improved as more APFS documentation
    * is published.
  */
  block_size = le_long(&sb->block_size);
  if(block_size != APFS_STANDARD_BLOCK_SIZE)
    return false;
  
  apfsvolume.blocksize = block_size;

  n.setSize((unsigned long long)block_size * le_longlong(&sb->block_count));

  // TODO: APFS contains many volumes and scanning these would be a further job.
  return true;
}

struct mft_entry
{
  char magic[4];
  uint16_t usa_ofs;
  uint16_t usa_count;

  uint64_t logseqnumber;
  uint16_t seq;
  uint16_t links;
  uint16_t attr_ofs;
  uint16_t flags;
  uint32_t bytes_used;
  uint32_t bytes_allocated;
  
  /* some garbage */
};

// for flags
#define MFT_RECORD_IN_USE 1
#define MFT_RECORD_IS_DIRECTORY 2

struct attr_entry
{
/*Ofs*/ 
/*  0*/ uint32_t type;        /* The (32-bit) type of the attribute. */
/*  4*/ uint32_t length;             /* Byte size of the resident part of the
                                   attribute (aligned to 8-byte boundary).
                                   Used to get to the next attribute. */
/*  8*/ uint8_t non_resident;        /* If 0, attribute is resident.
                                   If 1, attribute is non-resident. */
/*  9*/ uint8_t name_length;         /* Unicode character size of name of attribute.
                                   0 if unnamed. */
/* 10*/ uint16_t name_offset;        /* If name_length != 0, the byte offset to the
                                   beginning of the name from the attribute
                                   record. Note that the name is stored as a
                                   Unicode string. When creating, place offset
                                   just at the end of the record header. Then,
                                   follow with attribute value or mapping pairs
                                   array, resident and non-resident attributes
                                   respectively, aligning to an 8-byte
                                   boundary. */
/* 12*/ uint16_t flags;       /* Flags describing the attribute. */
/* 14*/ uint16_t instance;           /* The instance of this attribute record. This
                                   number is unique within this mft record (see
                                   MFT_RECORD/next_attribute_instance notes
                                   above for more details). */
                /* CAUTION: we only use resident attributes. */
                struct {
/* 16 */                uint32_t value_length; /* Byte size of attribute value. */
/* 20 */                uint16_t value_offset; /* Byte offset of the attribute
                                               value from the start of the
                                               attribute record. When creating,
                                               align to 8-byte boundary if we
                                               have a name present as this might
                                               not have a length of a multiple
                                               of 8-bytes. */
/* 22 */                uint8_t resident_flags; /* See above. */
/* 23 */                int8_t reservedR;       /* Reserved/alignment to 8-byte
                                               boundary. */
                };
};

typedef enum {
        AT_UNUSED                       = 0,
        AT_STANDARD_INFORMATION         = 0x10,
        AT_ATTRIBUTE_LIST               = 0x20,
        AT_FILE_NAME                    = 0x30,
        AT_OBJECT_ID                    = 0x40,
        AT_VOLUME_NAME                  = 0x60,
        AT_VOLUME_INFORMATION           = 0x70,
        AT_END                          = 0xffffffff,
	// other values are defined but we don't use them
} ATTR_TYPES;

typedef enum { 
        VOLUME_IS_DIRTY                 = 0x0001,
        VOLUME_RESIZE_LOG_FILE          = 0x0002,
        VOLUME_UPGRADE_ON_MOUNT         = 0x0004,
        VOLUME_MOUNTED_ON_NT4           = 0x0008,
        VOLUME_DELETE_USN_UNDERWAY      = 0x0010,
        VOLUME_REPAIR_OBJECT_ID         = 0x0020,
        VOLUME_MODIFIED_BY_CHKDSK       = 0x8000,
        VOLUME_FLAGS_MASK               = 0x803f,
} VOLUME_FLAGS;

struct volinfo {
        uint64_t reserved;           /* Not used (yet?). */
        uint8_t major_ver;           /* Major version of the ntfs format. */
        uint8_t minor_ver;           /* Minor version of the ntfs format. */
        uint16_t flags;     /* Bit array of VOLUME_* flags. */
};

struct stdinfo
{
/*Ofs*/
/*  0*/ int64_t creation_time;              /* Time file was created. Updated when
                                           a filename is changed(?). */
/*  8*/ int64_t last_data_change_time;      /* Time the data attribute was last
                                           modified. */
/* 16*/ int64_t last_mft_change_time;       /* Time this mft record was last
                                           modified. */
/* 24*/ int64_t last_access_time;           /* Approximate time when the file was
                                           last accessed (obviously this is not
                                           updated on read-only volumes). In
                                           Windows this is only updated when
                                           accessed if some time delta has
                                           passed since the last update. Also,
                                           last access times updates can be
                                           disabled altogether for speed. */
/* 32*/ uint32_t file_attributes; /* Flags describing the file. */
	/* CAUTION: this structure is incomplete */
};

#define MFT_RECORD_SIZE 1024
#define MFT_MFT		0	// inode for $MFT
#define MFT_MFTMirr	1	// inode for $MFTMirr
#define MFT_LogFile	2	// inode for $LogFile
#define MFT_VOLUME	3	// inode for $Volume

#define NTFSTIMEOFFSET ((int64_t)(369 * 365 + 89) * 24 * 3600 * 10000000)

static time_t ntfs2utc(int64_t ntfs_time)
{
  return (ntfs_time - (NTFSTIMEOFFSET)) / 10000000;
}

static bool detect_ntfs(hwNode & n, source & s)
{
  static char buffer[BLOCKSIZE];
  source ntfsvolume;
  string magic, serial, name, version;
  unsigned long long bytes_per_sector = 512;
  unsigned long long sectors_per_cluster = 8;
  signed char clusters_per_mft_record = 1;
  unsigned long long reserved_sectors = 0;
  unsigned long long hidden_sectors = 0;
  unsigned long long size = 0;
  unsigned long long mft = 0;
  mft_entry *entry = NULL;
  attr_entry *attr = NULL;
  unsigned long long mft_record_size = MFT_RECORD_SIZE;
  volinfo *vi = NULL;
  stdinfo *info = NULL;
  string guid = "";

  ntfsvolume = s;
  ntfsvolume.blocksize = BLOCKSIZE;

                                                  // read the first block
  if(readlogicalblocks(ntfsvolume, buffer, 0, 1)!=1)
    return false;

  if(be_short(buffer+0x1fe) != 0x55aa)		// no "boot" signature
    return false;

  magic = hw::strip(string(buffer+3, 8));
  if(magic != "NTFS")				// wrong magic
    return false;

  bytes_per_sector = le_short(buffer+0xb);
  sectors_per_cluster = le_short(buffer+0xd);
  reserved_sectors = le_short(buffer+0xe);
  hidden_sectors = le_short(buffer+0x1c);
  size = le_long(buffer+0x28);
  size -= reserved_sectors + hidden_sectors;
  size *= bytes_per_sector;

  serial = dos_serial(le_long(buffer+0x48));

  mft = le_longlong(buffer+0x30);
  clusters_per_mft_record = (char)buffer[0x40];
  if(clusters_per_mft_record < 0)
    mft_record_size = 1 << -clusters_per_mft_record;
  else
  {
    mft_record_size = clusters_per_mft_record;
    mft_record_size *= bytes_per_sector * sectors_per_cluster;
  }

  ntfsvolume = s;
  ntfsvolume.offset += mft * bytes_per_sector * sectors_per_cluster; // point to $MFT
  ntfsvolume.blocksize = mft_record_size;
  // FIXME mft_record_size<=sizeof(buffer)
  if(readlogicalblocks(ntfsvolume, buffer, MFT_VOLUME, 1)!=1)	// read $Volume
    return false;

  entry = (mft_entry*)buffer;
  if(strncmp(entry->magic, "FILE", 4) != 0)
    return false;

  if(le_short(&entry->flags) & MFT_RECORD_IN_USE)	// $Volume is valid
  {
    unsigned offset = le_short(&entry->attr_ofs);

    while(offset < mft_record_size)
    {
       attr = (attr_entry*)(buffer+offset);

       if(attr->type == AT_END)
         break;

       if(!attr->non_resident)		// ignore non-resident attributes
         switch(le_long(&attr->type))
         {
           case AT_STANDARD_INFORMATION:
             info = (stdinfo*)(buffer+offset+le_short(&attr->value_offset));
             break;
           case AT_VOLUME_INFORMATION:
             vi = (volinfo*)(buffer+offset+le_short(&attr->value_offset));
             vi->flags = le_short(&vi->flags);
             version = tostring(vi->major_ver) + "." + tostring(vi->minor_ver);
             break;
           case AT_OBJECT_ID:
             guid = uuid((uint8_t*)buffer+offset+le_short(&attr->value_offset));
             break;
           case AT_VOLUME_NAME:
             name = utf8((uint16_t*)(buffer+offset+le_short(&attr->value_offset)), le_short(&attr->value_length)/2, true);
             break;
           }
       offset += le_short(&attr->length);
    }
  }

  n.setSize(size);
  n.setCapacity(ntfsvolume.size);
  n.setConfig("clustersize", bytes_per_sector * sectors_per_cluster);
  n.setConfig("label", name);
  n.setSerial(serial);
  if(guid != "")
    n.setSerial(guid);
  n.setVersion(version);
  n.setDescription("");
  if(vi)
  {
    if(vi->flags & VOLUME_IS_DIRTY)
      n.setConfig("state", "dirty");
    else
      n.setConfig("state", "clean");

    if(vi->flags & VOLUME_MODIFIED_BY_CHKDSK)
      n.setConfig("modified_by_chkdsk", "true");
    if(vi->flags & VOLUME_MOUNTED_ON_NT4)
      n.setConfig("mounted_on_nt4", "true");
    if(vi->flags & VOLUME_UPGRADE_ON_MOUNT)
      n.setConfig("upgrade_on_mount", "true");
    if(vi->flags & VOLUME_RESIZE_LOG_FILE)
      n.setConfig("resize_log_file", "true");
  }
  if(info)
  {
    n.setConfig("created", datetime(ntfs2utc(le_longlong(&info->creation_time))));
    //n.setConfig("mounted", datetime(ntfs2utc(le_longlong(&info->last_access_time))));
    //n.setConfig("modified", datetime(ntfs2utc(le_longlong(&info->last_mft_change_time))));
  }
  return true;
}

#define SWAPBLOCKSIZE 1024

static bool detect_swap(hwNode & n, source & s)
{
  static char buffer[SWAPBLOCKSIZE];
  source swapvolume;
  unsigned long version = 0;
  unsigned long pages = 0;
  unsigned long badpages = 0;
  unsigned long long pagesize = 4096;
  bool bigendian = false;
  string signature = "";

  swapvolume = s;
  swapvolume.blocksize = SWAPBLOCKSIZE;

  if(readlogicalblocks(swapvolume, buffer, 3, 1)!=1) // look for a signature
    return false;
  signature = string(buffer+SWAPBLOCKSIZE-10, 10);
  if(signature != "SWAPSPACE2" && signature != "SWAP-SPACE")
    return false;

  swapvolume = s;
  swapvolume.blocksize = SWAPBLOCKSIZE;
  if(readlogicalblocks(swapvolume, buffer, 1, 1)!=1) // skip the first block
    return false;

  version = le_long(buffer);
  if(version == 0x01000000)
  {
    bigendian = true;
    version = 1;
  }

  if((version < 0) || (version > 1))	// unknown version
    return false;

  if(bigendian)
  {
    pages = be_long(buffer + 4) + 1;
    badpages = be_long(buffer + 8);
  }
  else
  {
    pages = le_long(buffer + 4) + 1;
    badpages = le_long(buffer + 8);
  }
  pagesize = swapvolume.size / (unsigned long long)pages;

  n.setSerial(uuid((uint8_t*)buffer+0xc));
  n.setConfig("label", hw::strip(std::string(buffer+0x1c, 16)));
  n.setConfig("pagesize", pagesize);
  n.setSize((unsigned long long)(pages - badpages) * pagesize);
  n.setCapacity(swapvolume.size);
  n.setVersion(tostring(version));
  n.setDescription("");

  return true;
}

bool scan_volume(hwNode & n, source & s)
{
  int i = 0;

  while(fs_types[i].id)
  {
    if(fs_types[i].detect && fs_types[i].detect(n, s))
    {
      n.addCapability(fs_types[i].id, fs_types[i].description);
      if(n.getConfig("filesystem") == "")
        n.setConfig("filesystem", fs_types[i].id);
      n.addCapability("initialized", _("initialized volume"));
      if(n.getDescription()=="")
        n.setDescription(string(fs_types[i].description) + " "+string(_("volume")));
      return true;
    }
    i++;
  }

  return scan_lvm(n,s);
}
