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
#include <string.h>
#include <unistd.h>

#include "osutils.h"

__ID("@(#) $Id: smp.cc 1897 2007-10-02 13:29:47Z lyonel $");

#if defined(__i386__)

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

/* MP Configuration Table Header */
typedef struct MPCTH {
    char        signature[ 4 ];
    u_short     base_table_length;
    uint8_t      spec_rev;
    uint8_t      checksum;
    char        oem_id[ 8 ];
    char        product_id[ 12 ];
    void*       oem_table_pointer;
    u_short     oem_table_size;
    u_short     entry_count;
    void*       apic_address;
    u_short     extended_table_length;
    uint8_t      extended_table_checksum;
    uint8_t      reserved;
} mpcth_t;

typedef struct PROCENTRY {
    uint8_t      type;
    uint8_t      apicID;
    uint8_t      apicVersion;
    uint8_t      cpuFlags;
    uint32_t      cpuSignature;
    uint32_t      featureFlags;
    uint32_t      reserved1;
    uint32_t      reserved2;
} ProcEntry;

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

static int readType( void )
{
    uint8_t      type;

    if ( read( pfd, &type, sizeof( type ) ) == sizeof( type ) )
      lseek( pfd, -sizeof( type ), SEEK_CUR );

    return (int)type;
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

static hwNode *addCPU(hwNode & n, int i, ProcEntry entry)
{
  hwNode *cpu = NULL;

  if(!(entry.cpuFlags & PROCENTRY_FLAG_EN))
    return NULL;

  cpu = n.findChildByBusInfo("cpu@"+tostring(i));
  if(!cpu)
  {
    hwNode *core = n.getChild("core");

    if (!core)
    {
      n.addChild(hwNode("core", hw::bus));
      core = n.getChild("core");
    }
    if(!core)
      cpu = n.addChild(hwNode("cpu", hw::processor));
    else
      cpu = core->addChild(hwNode("cpu", hw::processor));

    if(cpu)
      cpu->setBusInfo("cpu@"+tostring(i));
  }

  if(cpu)
  {
    cpu->setVersion(tostring((entry.cpuSignature >> 8) & 0x0f) + "." + tostring((entry.cpuSignature >> 4) & 0x0f) + "." + tostring(entry.cpuSignature & 0x0f));

    if(entry.cpuFlags & PROCENTRY_FLAG_EN)
      cpu->enable();
    if(entry.cpuFlags & PROCENTRY_FLAG_BP)
      cpu->addCapability("boot", "boot processor");
  }

  return cpu;
}

static void MPConfigDefault( hwNode & n, int featureByte )
{
    switch ( featureByte ) {
    case 1:
        n.addCapability("ISA");
        n.addCapability("APIC:82489DX");
        break;
    case 2:
    case 3:
        n.addCapability("EISA");
        n.addCapability("APIC:82489DX");
        break;
    case 4:
        n.addCapability("MCA");
        n.addCapability("APIC:82489DX");
        break;
    case 5:
        n.addCapability("ISA");
        n.addCapability("PCI");
        n.addCapability("APIC:integrated");
        break;
    case 6:
        n.addCapability("EISA");
        n.addCapability("PCI");
        n.addCapability("APIC:integrated");
        break;
    case 7:
        n.addCapability("MCA");
        n.addCapability("PCI");
        n.addCapability("APIC:integrated");
        break;
    }

    switch ( featureByte ) {
    case 1:
    case 2:
    case 3:
    case 4:
        n.setConfig("bus", 1);
        break;
    case 5:
    case 6:
    case 7:
        n.setConfig("bus", 2);
        break;
    }

    n.setConfig("cpus", 2);
}

static void MPConfigTableHeader( hwNode & n, uint32_t pap )
{
    mpcth_t	cth;
    int		c;
    int		entrytype;
    int ncpu = 0;
    int nbus = 0;
    int napic = 0;
    int nintr = 0;
    ProcEntry   entry;

    if ( !pap )
      return;

    /* read in cth structure */
    seekEntry( pap );
    readEntry( &cth, sizeof( cth ) );


  if(string(cth.signature, 4) != "PCMP")
    return;

  if(n.getVendor() == "")
    n.setVendor(hw::strip(string(cth.oem_id, 8)));
  if(n.getProduct() == "")
    n.setProduct(hw::strip(string(cth.product_id, 12)));

  n.addCapability("smp-1." + tostring(cth.spec_rev), "SMP specification v1."+tostring(cth.spec_rev));

    for (c = cth.entry_count; c; c--) {
	entrytype = readType();
	switch (entrytype) {
	case 0:
	    readEntry( &entry, sizeof( entry ) );
	    if(addCPU(n, ncpu, entry))
              ncpu++;
	    break;

	case 1:
	    //busEntry();
	    nbus++;
	    break;

	case 2:
	    //ioApicEntry();
	    napic++;
	    break;

	case 3:
	    //intEntry();
	    nintr++;
	    break;

	case 4:
	    //intEntry();
	    nintr++;
	    break;
	}
    }

    n.setConfig("cpus", ncpu);
}

bool issmp(hwNode & n)
{
  vm_offset_t paddr;
  mpfps_t mpfps;

  if((pfd = open("/dev/mem", O_RDONLY)) < 0)
    return false;

  if (apic_probe(&paddr) <= 0)
    return false;

  /* read in mpfps structure*/
  seekEntry(paddr);
  readEntry(&mpfps, sizeof(mpfps_t));

  if(string(mpfps.signature, 4) != "_MP_")
    return false;

  /* check whether a pre-defined MP config table exists */
  if (mpfps.mpfb1)
    MPConfigDefault(n, mpfps.mpfb1);
  else
  if(mpfps.pap)
    MPConfigTableHeader(n, mpfps.pap);

  close(pfd);
  return true;
}

#else

bool issmp(hwNode & n)
{
  return false;
}

#endif

bool scan_smp(hwNode & n)
{
  if(issmp(n))
  {
    n.addCapability("smp", "Symmetric Multi-Processing");
    return true;
  }
  else
    return false;
}
