/*
 * smp.cc
 *
 *
 */

#include "version.h"
#include "smp.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

__ID("@(#) $Id: mem.cc 1352 2006-05-27 23:54:13Z ezix $");

typedef unsigned long vm_offset_t;

/* MP Floating Pointer Structure */
typedef struct MPFPS {
        char    signature[4];
        uint32_t             pap;
        uint8_t      length;
        uint8_t      spec_rev;
        uint8_t      checksum;
        uint8_t      mpfb1;
        uint8_t      mpfb2;
        uint8_t      mpfb3;
        uint8_t      mpfb4;
        uint8_t      mpfb5;
} mpfps_t;


/* EBDA is @ 40:0e in real-mode terms */
#define EBDA_POINTER                    0x040e            /* location of EBDA pointer */

/* CMOS 'top of mem' is @ 40:13 in real-mode terms */
#define TOPOFMEM_POINTER                0x0413            /* BIOS: base memory size */

#define DEFAULT_TOPOFMEM                0xa0000

#define BIOS_BASE                          0xf0000
#define BIOS_BASE2                        0xe0000
#define BIOS_SIZE                          0x10000
#define ONE_KBYTE                          1024

#define GROPE_AREA1                      0x80000
#define GROPE_AREA2                      0x90000
#define GROPE_SIZE                        0x10000

#define PROCENTRY_FLAG_EN          0x01
#define PROCENTRY_FLAG_BP          0x02
#define IOAPICENTRY_FLAG_EN      0x01

#define MAXPNSTR                                132

static int pfd = -1; 

static bool seekEntry(vm_offset_t addr)
{
  return(lseek(pfd, (off_t)addr, SEEK_SET) >= 0);
}

static bool readEntry(void* entry, int size)
{
  return (read(pfd, entry, size) == size);
}

/*
 * set PHYSICAL address of MP floating pointer structure
 */
#define NEXT(X)         ((X) += 4)
static int apic_probe(vm_offset_t* paddr)
{
        unsigned int x;
        uint16_t segment;
        vm_offset_t target;
        unsigned int buffer[BIOS_SIZE];
        const char MP_SIG[]="_MP_";

        /* search Extended Bios Data Area, if present */
        seekEntry((vm_offset_t)EBDA_POINTER);
        readEntry(&segment, 2);
        if (segment) {                          /* search EBDA */
                target = (vm_offset_t)segment << 4;
                seekEntry(target);
                readEntry(buffer, ONE_KBYTE);

                for (x = 0; x < ONE_KBYTE / 4; NEXT(x)) {
                        if (!strncmp((char *)&buffer[x], MP_SIG, 4)) {
                                *paddr = (x*4) + target;
                                return 1;
                        }
                }
        }

        /* read CMOS for real top of mem */
        seekEntry((vm_offset_t)TOPOFMEM_POINTER);
        readEntry(&segment, 2);
        --segment;                                                /* less ONE_KBYTE */
        target = segment * 1024;
        seekEntry(target);
        readEntry(buffer, ONE_KBYTE);

        for (x = 0; x < ONE_KBYTE/4; NEXT(x)) {
                if (!strncmp((char *)&buffer[x], MP_SIG, 4)) {
                        *paddr = (x*4) + target;
                        return 2;
                }
        }
        /* we don't necessarily believe CMOS, check base of the last 1K of 640K */
        if (target != (DEFAULT_TOPOFMEM - 1024)) {
                target = (DEFAULT_TOPOFMEM - 1024);
                seekEntry(target);
                readEntry(buffer, ONE_KBYTE);

                for (x = 0; x < ONE_KBYTE/4; NEXT(x)) {
                        if (!strncmp((char *)&buffer[x], MP_SIG, 4)) {
                                *paddr = (x*4) + target;
                                return 3;
                        }
                }
        }

        /* search the BIOS */
        seekEntry(BIOS_BASE);
        readEntry(buffer, BIOS_SIZE);

        for (x = 0; x < BIOS_SIZE/4; NEXT(x)) {
                if (!strncmp((char *)&buffer[x], MP_SIG, 4)) {
                        *paddr = (x*4) + BIOS_BASE;
                        return 4;
                }
        }

        /* search the extended BIOS */
        seekEntry(BIOS_BASE2);
        readEntry(buffer, BIOS_SIZE);

        for (x = 0; x < BIOS_SIZE/4; NEXT(x)) {
                if (!strncmp((char *)&buffer[x], MP_SIG, 4)) {
                        *paddr = (x*4) + BIOS_BASE2;
                        return 4;
                }
        }

        /* search additional memory */
        target = GROPE_AREA1;
        seekEntry(target);
        readEntry(buffer, GROPE_SIZE);

        for (x = 0; x < GROPE_SIZE/4; NEXT(x)) {
                if (!strncmp((char *)&buffer[x], MP_SIG, 4)) {
                        *paddr = (x*4) + GROPE_AREA1;
                        return 5;
                }
        }

        target = GROPE_AREA2;
        seekEntry(target);
        readEntry(buffer, GROPE_SIZE);

        for (x = 0; x < GROPE_SIZE/4; NEXT(x)) {
                if (!strncmp((char *)&buffer[x], MP_SIG, 4)) {
                        *paddr = (x*4) + GROPE_AREA2;
                        return 6;
                }
        }

        *paddr = (vm_offset_t)0;
        return 0;
}

bool issmp()
{
  vm_offset_t paddr;
  mpfps_t mpfps;
  bool result = false;

  if((pfd = open("/dev/mem", O_RDONLY)) < 0)
    return false;

  if (apic_probe(&paddr) <= 0)
    return false;

  /* read in mpfps structure*/
  seekEntry(paddr);
  readEntry(&mpfps, sizeof(mpfps_t));

  /* check whether an MP config table exists */
  if (!mpfps.mpfb1)
    result = true;

  close(pfd);
  return result;
}

bool scan_smp(hwNode & n)
{
  hwNode *core = n.getChild("core");

  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  if (core)
  {
    if(issmp())
      core->addCapability("smp", "Symmetric Multi-Processing");
  }

  return false;
}
