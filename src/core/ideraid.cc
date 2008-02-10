/*
 *  ideraid.cc
 *
 */

#include "version.h"
#include "cpuinfo.h"
#include "osutils.h"
#include "cdrom.h"
#include "disk.h"
#include "heuristics.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <endian.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <vector>
#include <linux/hdreg.h>
#include <regex.h>

__ID("@(#) $Id$");

#define DEV_TWE "/dev/twe"

#define IORDY_SUP               0x0800            /* 1=support; 0=may be supported */
#define IORDY_OFF               0x0400            /* 1=may be disabled */
#define LBA_SUP                 0x0200            /* 1=Logical Block Address support */
#define DMA_SUP                 0x0100            /* 1=Direct Memory Access support */
#define DMA_IL_SUP              0x8000            /* 1=interleaved DMA support (ATAPI) */
#define CMD_Q_SUP               0x4000            /* 1=command queuing support (ATAPI) */
#define OVLP_SUP                0x2000            /* 1=overlap operation support (ATAPI) */

#define GEN_CONFIG              0                 /* general configuration */
#define CD_ROM                  5
#define NOT_ATA                 0x8000
#define NOT_ATAPI               0x4000            /* (check only if bit 15 == 1) */
#define EQPT_TYPE               0x1f00

static const char *description[] =
{
  "Direct-access device",                         /* word 0, bits 12-8 = 00 */
  "Sequential-access device",                     /* word 0, bits 12-8 = 01 */
  "Printer",                                      /* word 0, bits 12-8 = 02 */
  "Processor",                                    /* word 0, bits 12-8 = 03 */
  "Write-once device",                            /* word 0, bits 12-8 = 04 */
  "CD-ROM",                                       /* word 0, bits 12-8 = 05 */
  "Scanner",                                      /* word 0, bits 12-8 = 06 */
  "Optical memory",                               /* word 0, bits 12-8 = 07 */
  "Medium changer",                               /* word 0, bits 12-8 = 08 */
  "Communications device",                        /* word 0, bits 12-8 = 09 */
  "ACS-IT8 device",                               /* word 0, bits 12-8 = 0a */
  "ACS-IT8 device",                               /* word 0, bits 12-8 = 0b */
  "Array controller",                             /* word 0, bits 12-8 = 0c */
  "Enclosure services",                           /* word 0, bits 12-8 = 0d */
  "Reduced block command device",                 /* word 0, bits 12-8 = 0e */
  "Optical card reader/writer",                   /* word 0, bits 12-8 = 0f */
  "",                                             /* word 0, bits 12-8 = 10 */
  "",                                             /* word 0, bits 12-8 = 11 */
  "",                                             /* word 0, bits 12-8 = 12 */
  "",                                             /* word 0, bits 12-8 = 13 */
  "",                                             /* word 0, bits 12-8 = 14 */
  "",                                             /* word 0, bits 12-8 = 15 */
  "",                                             /* word 0, bits 12-8 = 16 */
  "",                                             /* word 0, bits 12-8 = 17 */
  "",                                             /* word 0, bits 12-8 = 18 */
  "",                                             /* word 0, bits 12-8 = 19 */
  "",                                             /* word 0, bits 12-8 = 1a */
  "",                                             /* word 0, bits 12-8 = 1b */
  "",                                             /* word 0, bits 12-8 = 1c */
  "",                                             /* word 0, bits 12-8 = 1d */
  "",                                             /* word 0, bits 12-8 = 1e */
  "Unknown",                                      /* word 0, bits 12-8 = 1f */
};

/* older kernels (2.2.x) have incomplete id structure for IDE devices */
#ifndef __NEW_HD_DRIVE_ID
#define command_set_1 command_sets
#define command_set_2 word83
#define hw_config word93
#endif


#define ATA_SMART_CMD                   0xb0
#define ATA_IDENTIFY_DEVICE             0xec

#define TW_OP_ATA_PASSTHRU 0x11

#define TW_IOCTL_FIRMWARE_PASS_THROUGH 0x108
#define TW_CMD_PACKET_WITH_DATA 0x1f

#define TW_MAX_SGL_LENGTH	62

typedef struct TAG_TW_Ioctl {
  int input_length;
  int output_length;
  unsigned char cdb[16];
  unsigned char opcode;
  // This one byte of padding is missing from the typedefs in the
  // kernel code, but it is indeed present.  We put it explicitly
  // here, so that the structure can be packed.  Adam agrees with
  // this.
  unsigned char packing;
  unsigned short table_id;
  unsigned char parameter_id;
  unsigned char parameter_size_bytes;
  unsigned char unit_index;
  // Size up to here is 30 bytes + 1 padding!
  unsigned char input_data[499];
  // Reserve lots of extra space for commands that set Sector Count
  // register to large values
  unsigned char output_data[512]; // starts 530 bytes in!
  // two more padding bytes here if structure NOT packed.
} TW_Ioctl;


/* 512 is the max payload size: increase if needed */
#define TW_IOCTL_BUFFER_SIZE sizeof(TW_Ioctl)

#pragma pack(1)
/* Scatter gather list entry */
typedef struct TAG_TW_SG_Entry {
  unsigned int address;
  unsigned int length;
} TW_SG_Entry;

/* Command header for ATA pass-thru.  Note that for different
   drivers/interfaces the length of sg_list (here TW_ATA_PASS_SGL_MAX)
   is different.  But it can be taken as same for all three cases
   because it's never used to define any other structures, and we
   never use anything in the sg_list or beyond! */

#define TW_ATA_PASS_SGL_MAX      60

typedef struct TAG_TW_Passthru {
  struct {
    unsigned char opcode:5;
    unsigned char sgloff:3;
  } byte0;
  unsigned char size;
  unsigned char request_id;
  struct {
    unsigned char aport:4;
    unsigned char host_id:4;
  } byte3;
  unsigned char status;  // On return, contains 3ware STATUS register
  unsigned char flags;
  unsigned short param;
  unsigned short features;  // On return, contains ATA ERROR register
  unsigned short sector_count;
  unsigned short sector_num;
  unsigned short cylinder_lo;
  unsigned short cylinder_hi;
  unsigned char drive_head;
  unsigned char command; // On return, contains ATA STATUS register
  TW_SG_Entry sg_list[TW_ATA_PASS_SGL_MAX];
  unsigned char padding[12];
} TW_Passthru;

/* Command Packet */
typedef struct TW_Command {
  /* First DWORD */
  struct {
    unsigned char opcode:5;
    unsigned char sgl_offset:3;
  } byte0;
  unsigned char size;
  unsigned char request_id;
  struct {
    unsigned char unit:4;
    unsigned char host_id:4;
  } byte3;
  /* Second DWORD */
  unsigned char status;
  unsigned char flags;
  union {
    unsigned short block_count;
    unsigned short parameter_count;
    unsigned short message_credits;
  } byte6;
  union {
    struct {
      uint32_t lba;
      TW_SG_Entry sgl[TW_MAX_SGL_LENGTH];
      uint32_t padding;      /* pad to 512 bytes */
    } io;
    struct {
      TW_SG_Entry sgl[TW_MAX_SGL_LENGTH];
      uint32_t padding[2];
    } param;
    struct {
      uint32_t response_queue_pointer;
      uint32_t padding[125];
    } init_connection;
    struct {
      char version[504];
    } ioctl_miniport_version;
  } byte8;
} TW_Command;

typedef struct TAG_TW_New_Ioctl {
  unsigned int  data_buffer_length;
  unsigned char padding [508];
  TW_Command    firmware_command;
  unsigned char          data_buffer[1];
  // three bytes of padding here
} TW_New_Ioctl;

static void fix_string(unsigned char * s, unsigned len)
{
  unsigned char c;

  while(len>1)
  {
    c = s[0];
    s[0] = s[1];
    s[1] = c;
    s += 2;
    len -= 2;
  }
}

static bool guess_manufacturer(hwNode & device);

static bool probe_port(unsigned controller, unsigned disknum, hwNode & parent)
{
  hwNode device("member:"+tostring(disknum), hw::disk);
  struct hd_driveid id;
  const u_int8_t *id_regs = (const u_int8_t *) &id;
  string devname = string(DEV_TWE) + tostring(controller);
  int fd = open(devname.c_str(), O_RDONLY | O_NONBLOCK);
  unsigned char ioctl_buffer[2*TW_IOCTL_BUFFER_SIZE];

   // only used for 6000/7000/8000 char device interface
  TW_New_Ioctl *tw_ioctl_char=NULL;
  TW_Passthru *passthru=NULL;

  memset(ioctl_buffer, 0, sizeof(ioctl_buffer));

  tw_ioctl_char = (TW_New_Ioctl *)ioctl_buffer;
  tw_ioctl_char->data_buffer_length = 512;
  passthru = (TW_Passthru *)&(tw_ioctl_char->firmware_command);

  passthru->byte0.opcode  = TW_OP_ATA_PASSTHRU;
  passthru->request_id    = 0xFF;
  passthru->byte3.aport   = disknum;
  passthru->byte3.host_id = 0;
  passthru->status        = 0;
  passthru->flags         = 0x1;
  passthru->drive_head    = 0x0;
  passthru->sector_num    = 0;
  passthru->byte0.sgloff = 0x5;
  passthru->size         = 0x7;
  passthru->param        = 0xD;
  passthru->sector_count = 0x1;
  passthru->command       = ATA_IDENTIFY_DEVICE;
  passthru->features    = 0;
  passthru->cylinder_lo = 0;
  passthru->cylinder_hi = 0;

  if (fd < 0)
    return false;

  memset(&id, 0, sizeof(id));
  if (ioctl(fd, TW_CMD_PACKET_WITH_DATA, tw_ioctl_char) != 0)
  {
    close(fd);
    return false;
  }

  close(fd);

  device.setPhysId(disknum);
  device.setBusInfo("raid@c" + tostring(controller) + "/p" + tostring(disknum));
  device.setLogicalName("c" + tostring(controller) + "/p" + tostring(disknum));
  device.claim();

  u_int16_t pidentity[256];
  for (int i = 0; i < 256; i++)
    pidentity[i] = tw_ioctl_char->data_buffer[2 * i] + (tw_ioctl_char->data_buffer[2 * i + 1] << 8);
  memcpy(&id, pidentity, sizeof(id));

  fix_string(id.model, sizeof(id.model));
  fix_string(id.fw_rev, sizeof(id.fw_rev));
  fix_string(id.serial_no, sizeof(id.serial_no));

  if (id.model[0])
    device.setProduct(hw::strip(string((char *) id.model, sizeof(id.model))));
  else
    return false;
  if (id.fw_rev[0])
    device.
      setVersion(hw::strip(string((char *) id.fw_rev, sizeof(id.fw_rev))));
  else
    return false;
  if (id.serial_no[0])
    device.
      setSerial(hw::
      strip(string((char *) id.serial_no, sizeof(id.serial_no))));
  else
    return false;

  if (!(pidentity[GEN_CONFIG] & NOT_ATA))
  {
    device.addCapability("ata", "ATA");
    device.setDescription("ATA Disk");
  }
  else if (!(pidentity[GEN_CONFIG] & NOT_ATAPI))
  {
    u_int8_t eqpt = (pidentity[GEN_CONFIG] & EQPT_TYPE) >> 8;
    device.addCapability("atapi", "ATAPI");

    if (eqpt == CD_ROM)
      device.addCapability("cdrom", "can read CD-ROMs");

    if (eqpt < 0x20)
      device.setDescription("IDE " + string(description[eqpt]));
  }
  if (id.config & (1 << 7))
    device.addCapability("removable", "support is removable");
  if (id.config & (1 << 15))
    device.addCapability("nonmagnetic", "support is non-magnetic (optical)");
  if (id.capability & 1)
    device.addCapability("dma", "Direct Memory Access");
  if (id.capability & 2)
    device.addCapability("lba", "Large Block Addressing");
  if (id.capability & 8)
    device.addCapability("iordy", "I/O ready reporting");
  if (id.command_set_1 & 1)
    device.addCapability("smart",
      "S.M.A.R.T. (Self-Monitoring And Reporting Technology)");
  if (id.command_set_1 & 2)
    device.addCapability("security", "ATA security extensions");
  if (id.command_set_1 & 4)
    device.addCapability("removable", "support is removable");
  if (id.command_set_1 & 8)
    device.addCapability("pm", "Power Management");
  if (id.command_set_2 & 8)
    device.addCapability("apm", "Advanced Power Management");

  if ((id.capability & 8) || (id.field_valid & 2))
  {
    if (id.field_valid & 4)
    {
      if (id.dma_ultra & 0x100)
        device.setConfig("mode", "udma0");
      if (id.dma_ultra & 0x200)
        device.setConfig("mode", "udma1");
      if (id.dma_ultra & 0x400)
        device.setConfig("mode", "udma2");
      if (id.hw_config & 0x2000)
      {
        if (id.dma_ultra & 0x800)
          device.setConfig("mode", "udma3");
        if (id.dma_ultra & 0x1000)
          device.setConfig("mode", "udma4");
        if (id.dma_ultra & 0x2000)
          device.setConfig("mode", "udma5");
        if (id.dma_ultra & 0x4000)
          device.setConfig("mode", "udma6");
        if (id.dma_ultra & 0x8000)
          device.setConfig("mode", "udma7");
      }
    }
  }

  if (id_regs[83] & 8)
    device.addCapability("apm", "Advanced Power Management");

//if (device.isCapable("iordy") && (id.capability & 4))
//device.setConfig("iordy", "yes");

  if (device.isCapable("smart"))
  {
    if (id.command_set_2 & (1 << 14))
      device.setConfig("smart", "on");
    else
      device.setConfig("smart", "off");
  }

  if (device.isCapable("apm"))
  {
    if (!(id_regs[86] & 8))
      device.setConfig("apm", "off");
    else
      device.setConfig("apm", "on");
  }

  if (device.isCapable("lba"))
    device.setCapacity((unsigned long long) id.lba_capacity * 512);
  if (device.isCapable("removable"))
    device.setCapacity(0);                        // we'll first have to make sure we have a disk

  if (device.isCapable("cdrom"))
    scan_cdrom(device);
  else
    scan_disk(device);

#if 0
  if (!(iddata[GEN_CONFIG] & NOT_ATA))
    device.addCapability("ata");
  else if (iddata[GEN_CONFIG] == CFA_SUPPORT_VAL)
  {
    device.addCapability("ata");
    device.addCapability("compactflash");
  }
  else if (!(iddata[GEN_CONFIG] & NOT_ATAPI))
  {
    device.addCapability("atapi");
    device.setDescription(description[(iddata[GEN_CONFIG] & EQPT_TYPE) >> 8]);
  }

  if (iddata[START_MODEL])
    device.setProduct(print_ascii((char *) &iddata[START_MODEL],
      LENGTH_MODEL));
  if (iddata[START_SERIAL])
    device.
      setSerial(print_ascii((char *) &iddata[START_SERIAL], LENGTH_SERIAL));
  if (iddata[START_FW_REV])
    device.
      setVersion(print_ascii((char *) &iddata[START_FW_REV], LENGTH_FW_REV));
  if (iddata[CAPAB_0] & LBA_SUP)
    device.addCapability("lba");
  if (iddata[CAPAB_0] & IORDY_SUP)
    device.addCapability("iordy");
  if (iddata[CAPAB_0] & IORDY_OFF)
  {
    device.addCapability("iordy");
    device.addCapability("iordyoff");
  }
  if (iddata[CAPAB_0] & DMA_SUP)
    device.addCapability("dma");
  if (iddata[CAPAB_0] & DMA_IL_SUP)
  {
    device.addCapability("interleaved_dma");
    device.addCapability("dma");
  }
  if (iddata[CAPAB_0] & CMD_Q_SUP)
    device.addCapability("command_queuing");
  if (iddata[CAPAB_0] & OVLP_SUP)
    device.addCapability("overlap_operation");
#endif

  guess_manufacturer(device);
  parent.addChild(device);
  return true;
}


static const char *manufacturers[] =
{
  "^ST.+", "Seagate",
  "^D...-.+", "IBM",
  "^IBM.+", "IBM",
  "^HITACHI.+", "Hitachi",
  "^IC.+", "Hitachi",
  "^HTS.+", "Hitachi",
  "^FUJITSU.+", "Fujitsu",
  "^MP.+", "Fujitsu",
  "^TOSHIBA.+", "Toshiba",
  "^MK.+", "Toshiba",
  "^MAXTOR.+", "Maxtor",
  "^Pioneer.+", "Pioneer",
  "^PHILIPS.+", "Philips",
  "^QUANTUM.+", "Quantum",
  "FIREBALL.+", "Quantum",
  "^WDC.+", "Western Digital",
  "WD.+", "Western Digital",
  NULL, NULL
};

static bool guess_manufacturer(hwNode & device)
{
  int i = 0;
  bool result = false;

  while (manufacturers[i])
  {
    if (matches(device.getProduct().c_str(), manufacturers[i], REG_ICASE))
    {
      device.setVendor(manufacturers[i + 1]);
      result = true;
    }
    i += 2;
  }


  return result;
}


bool scan_ideraid(hwNode & n)
{
  unsigned c = 0;
  unsigned u = 0;

  for(c=0; c<16; c++)
  {
    for(u=0; u<8; u++)
      if(!probe_port(c,u,n))
        break;
  }

  return true;
}
