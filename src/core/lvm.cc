/*
 * lvm.cc
 *
 * This scan reports LVM-related information by looking for LVM2 structures
 * So far, the following information is reported:
 * - presence of LVM2 on-disk structures
 * - size of LVM2 physical volume
 * - UUID of LVM2 physical volume
 *
 */

#include "version.h"
#include "config.h"
#include "lvm.h"
#include "osutils.h"
#include <string.h>

__ID("@(#) $Id$");

#define LABEL_ID "LABELONE"
#define LABEL_SIZE BLOCKSIZE                      /* Think very carefully before changing this */
#define LABEL_SCAN_SECTORS 4L
#define INITIAL_CRC 0xf597a6cf

/* On disk - 32 bytes */
struct label_header
{
  uint8_t id[8];                                  /* LABELONE */
  uint64_t sector_xl;                             /* Sector number of this label */
  uint32_t crc_xl;                                /* From next field to end of sector */
  uint32_t offset_xl;                             /* Offset from start of struct to contents */
  uint8_t type[8];                                /* LVM2 001 */
} __attribute__ ((packed));

/* On disk */
struct disk_locn
{
  uint64_t offset;                                /* Offset in bytes to start sector */
  uint64_t size;                                  /* Bytes */
} __attribute__ ((packed));

#define ID_LEN 32

/* Fields with the suffix _xl should be xlate'd wherever they appear */
/* On disk */
struct pv_header
{
  uint8_t pv_uuid[ID_LEN];

/* This size can be overridden if PV belongs to a VG */
  uint64_t device_size_xl;                        /* Bytes */

/* NULL-terminated list of data areas followed by */
/* NULL-terminated list of metadata area headers */
  struct disk_locn disk_areas_xl[0];              /* Two lists */
} __attribute__ ((packed));

/* In core */
struct label
{
  char type[8];
  uint64_t sector;
  struct labeller *labeller;
  void *info;
};

/* Calculate an endian-independent CRC of supplied buffer */
uint32_t calc_crc(uint32_t initial, void *buf, uint32_t size)
{
  static const uint32_t crctab[] =
  {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };
  uint32_t i, crc = initial;
  uint8_t *data = (uint8_t *) buf;

  for(i = 0; i < size; i++)
  {
    crc ^= *data++;
    crc = (crc >> 4) ^ crctab[crc & 0xf];
    crc = (crc >> 4) ^ crctab[crc & 0xf];
  }
  return crc;
}


static const char *id = "@(#) $Id$";

static string uuid(void * s)
{
  char * id = (char*)s;

  return string(id   ,6)+"-"+
    string(id+ 6,4)+"-"+
    string(id+10,4)+"-"+
    string(id+14,4)+"-"+
    string(id+18,4)+"-"+
    string(id+22,4)+"-"+
    string(id+26,6);
}


bool scan_lvm(hwNode & n, source & s)
{
  uint8_t sector[BLOCKSIZE];
  label_header lh;

  if(s.blocksize != BLOCKSIZE)
    return false;

  for(uint32_t i=0; i<4; i++)
  {
    if(readlogicalblocks(s, sector, i, 1) != 1)
      return false;
    memcpy(&lh, sector, sizeof(lh));
    lh.sector_xl = le_longlong(sector+8);
    lh.crc_xl = le_long(sector+0x10);
    lh.offset_xl = le_long(sector+0x14);

    if((strncmp((char*)lh.id, LABEL_ID, sizeof(lh.id))==0) &&
      (lh.sector_xl==i) &&
      (lh.offset_xl < BLOCKSIZE) &&
      (calc_crc(INITIAL_CRC, sector+0x14, LABEL_SIZE-0x14)==lh.crc_xl))
    {
      pv_header pvh;

      memcpy(&pvh, sector+lh.offset_xl, sizeof(pvh));
      if(n.getDescription()=="")
        n.setDescription(_("Linux LVM Physical Volume"));
      n.addCapability("lvm2");
      n.setSerial(uuid(pvh.pv_uuid));
      if(n.getCapacity()==0)
        n.setCapacity(n.getSize());
      n.setSize(le_longlong(&pvh.device_size_xl));
      return true;
    }
  }

  (void) &id;                                     // avoid warning "id defined but not used"

  return false;
}
