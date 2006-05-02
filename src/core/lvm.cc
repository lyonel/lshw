/*
 * lvm.cc
 *
 *
 */

#include "lvm.h"

#define LABEL_ID "LABELONE"
#define LABEL_SIZE BLOCKSIZE  /* Think very carefully before changing this */
#define LABEL_SCAN_SECTORS 4L

/* On disk - 32 bytes */
struct label_header {
        uint8_t id[8];          /* LABELONE */
        uint64_t sector_xl;     /* Sector number of this label */
        uint32_t crc_xl;        /* From next field to end of sector */
        uint32_t offset_xl;     /* Offset from start of struct to contents */
        uint8_t type[8];        /* LVM2 001 */
} __attribute__ ((packed));

/* In core */
struct label {
        char type[8];
        uint64_t sector;
        struct labeller *labeller;
        void *info;
};


static char *id = "@(#) $Id: $";

bool scan_lvm(hwNode & n, source & s)
{
  (void) &id;			// avoid warning "id defined but not used"

  return false;
}
