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
static bool detect_ext3(hwNode & n, source & s);

static struct fstypes fs_types[] =
{
  {"blank", "Blank", "", NULL},
  {"fat", "MS-DOS FAT derivatives (FAT12, FAT16, FAT32)", "", NULL},
  {"ntfs", "Windows NTFS", "secure", NULL},
  {"hpfs", "OS/2 HPFS", "secure", NULL},
  {"ext2", "EXT2/EXT3", "secure", detect_ext2},
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
#define EXT4_FEATURE_INCOMPAT_64BIT             0x0080
#define EXT4_FEATURE_INCOMPAT_MMP               0x0100

static string uuid(const uint8_t s_uuid[16])
{
  char buffer[50];

  snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", s_uuid[0], s_uuid[1], s_uuid[2], s_uuid[3], s_uuid[4], s_uuid[5], s_uuid[6], s_uuid[7], s_uuid[8], s_uuid[9], s_uuid[10], s_uuid[11], s_uuid[12], s_uuid[13], s_uuid[14], s_uuid[15]);
  return string(buffer);
}

static bool detect_ext2(hwNode & n, source & s)
{
  static char buffer[EXT2_DEFAULT_BLOCK_SIZE];
  source ext2volume;
  ext2_super_block *sb = (ext2_super_block*)buffer;
  uint32_t ext2_version = 0;
  time_t mtime, wtime, mkfstime;
  tm ltime;
  char datetime[80];
  unsigned long blocksize = EXT2_DEFAULT_BLOCK_SIZE;

  ext2volume = s;
  ext2volume.blocksize = EXT2_DEFAULT_BLOCK_SIZE;

  if(readlogicalblocks(ext2volume, buffer, 1, 1)!=1) // read the second block
    return false;

  if(le_short(&sb->s_magic) != EXT2_SUPER_MAGIC)	// wrong magic number
    return false;

  blocksize = (1 << le_long(&sb->s_log_block_size));
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

  mtime = (time_t)le_long(&sb->s_mtime);
  if(mtime && localtime_r(&mtime, &ltime))
  {
    strftime(datetime, sizeof(datetime), "%F %T", &ltime);
    n.setConfig("mounted", datetime);
  }
  wtime = (time_t)le_long(&sb->s_wtime);
  if(wtime && localtime_r(&wtime, &ltime))
  {
    strftime(datetime, sizeof(datetime), "%F %T", &ltime);
    n.setConfig("modified", datetime);
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

    if(le_long(&sb->s_feature_compat) && EXT3_FEATURE_COMPAT_HAS_JOURNAL)
    {
      n.addCapability("journaled");
      
      mkfstime = (time_t)le_long(&sb->s_mkfs_time);
      if(mkfstime && localtime_r(&mkfstime, &ltime))
      {
        strftime(datetime, sizeof(datetime), "%F %T", &ltime);
        n.setConfig("created", datetime);
      }
    }
    if(le_long(&sb->s_feature_compat) && EXT2_FEATURE_COMPAT_EXT_ATTR)
      n.addCapability("extended_attributes", "Extended Attributes");
    if(le_long(&sb->s_feature_ro_compat) && EXT2_FEATURE_RO_COMPAT_LARGE_FILE)
      n.addCapability("large_files", "4GB+ files");
    if(le_long(&sb->s_feature_ro_compat) && EXT4_FEATURE_RO_COMPAT_HUGE_FILE)
      n.addCapability("huge_files", "16TB+ files");
    if(le_long(&sb->s_feature_incompat) && EXT3_FEATURE_INCOMPAT_RECOVER)
      n.addCapability("recover", "needs recovery");
  }

  if(n.isCapable("journaled"))
  {
    n.addCapability("ext3");
    n.setDescription("EXT3 volume");
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
      n.addCapability(fs_types[i].id, fs_types[i].description);
      n.addCapability(string("initialized"), "initialized volume");
      if(n.getDescription()=="")
        n.setDescription(string(fs_types[i].description) + " volume");
      break;
    }
    i++;
  }

  return scan_lvm(n,s);
}
