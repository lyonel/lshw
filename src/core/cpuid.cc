#include "version.h"
#include "config.h"
#include "cpuid.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

__ID("@(#) $Id$");

#if defined(__i386__) || defined(__alpha__)

static hwNode *getcache(hwNode & node,
int n = 0)
{
  char cachename[10];
  hwNode *cache = NULL;

  if (n < 0)
    n = 0;

  snprintf(cachename, sizeof(cachename), "cache:%d", n);
  cache = node.getChild(string(cachename));

  if (cache)
    return cache;

// "cache:0" is equivalent to "cache" if we only have L1 cache
  if ((n == 0) && (node.countChildren(hw::memory) <= 1))
    cache = node.getChild(string("cache"));
  if (cache)
    return cache;
  else
    return NULL;
}


static hwNode *getcpu(hwNode & node,
int n = 0)
{
  char cpubusinfo[10];
  hwNode *cpu = NULL;

  if (n < 0)
    n = 0;

  snprintf(cpubusinfo, sizeof(cpubusinfo), "cpu@%d", n);
  cpu = node.findChildByBusInfo(cpubusinfo);

  if (cpu)
    return cpu;

  if (n > 0)
    return NULL;

  hwNode *core = node.getChild("core");

  if (core)
  {
    hwNode cpu("cpu", hw::processor);

    cpu.setBusInfo(cpubusinfo);
    cpu.addHint("icon", string("cpu"));
    cpu.claim();

    return core->addChild(cpu);
  }
  else
    return NULL;
}
#endif                                            // __i386__ || __alpha__

#ifdef __i386__

/* %ebx may be the PIC register.  */
#define cpuid_up(in,a,b,c,d)\
  __asm__ ("xchgl\t%%ebx, %1\n\t"			\
	   "cpuid\n\t"					\
	   "xchgl\t%%ebx, %1\n\t"			\
	   : "=a" (a), "=r" (b), "=c" (c), "=d" (d)	\
	   : "0" (in))

static void cpuid(int cpunumber,
unsigned long idx,
unsigned long &eax,
unsigned long &ebx,
unsigned long &ecx,
unsigned long &edx)
{
  char cpuname[50];
  int fd = -1;
  unsigned char buffer[16];

  snprintf(cpuname, sizeof(cpuname), "/dev/cpu/%d/cpuid", cpunumber);
  fd = open(cpuname, O_RDONLY);
  if (fd >= 0)
  {
    lseek(fd, idx, SEEK_CUR);
    memset(buffer, 0, sizeof(buffer));
    if(read(fd, buffer, sizeof(buffer)) == sizeof(buffer))
    {
      eax = (*(unsigned long *) buffer);
      ebx = (*(unsigned long *) (buffer + 4));
      ecx = (*(unsigned long *) (buffer + 8));
      edx = (*(unsigned long *) (buffer + 12));
    }
    close(fd);
  }
  else
    cpuid_up(idx, eax, ebx, ecx, edx);
}


/* Decode Intel TLB and cache info descriptors */
static void decode_intel_tlb(int x,
long long &l1cache,
long long &l2cache)
{
  x &= 0xff;
  switch (x)
  {
    case 0:
      break;
    case 0x1:
// Instruction TLB: 4KB pages, 4-way set assoc, 32 entries
      break;
    case 0x2:
// Instruction TLB: 4MB pages, 4-way set assoc, 2 entries
      break;
    case 0x3:
// Data TLB: 4KB pages, 4-way set assoc, 64 entries
      break;
    case 0x4:
// Data TLB: 4MB pages, 4-way set assoc, 8 entries
      break;
    case 0x6:
// 1st-level instruction cache: 8KB, 4-way set assoc, 32 byte line size
      l1cache += 8 * 1024;
      break;
    case 0x8:
// 1st-level instruction cache: 16KB, 4-way set assoc, 32 byte line size
      l1cache += 16 * 1024;
      break;
    case 0xa:
// 1st-level data cache: 8KB, 2-way set assoc, 32 byte line size
      l1cache += 8 * 1024;
      break;
    case 0xc:
// 1st-level data cache: 16KB, 4-way set assoc, 32 byte line size
      l1cache += 16 * 1024;
      break;
    case 0x40:
// No 2nd-level cache, or if 2nd-level cache exists, no 3rd-level cache
      break;
    case 0x41:
// 2nd-level cache: 128KB, 4-way set assoc, 32 byte line size
      l2cache = 128 * 1024;
      break;
    case 0x42:
// 2nd-level cache: 256KB, 4-way set assoc, 32 byte line size
      l2cache = 256 * 1024;
      break;
    case 0x43:
// 2nd-level cache: 512KB, 4-way set assoc, 32 byte line size
      l2cache = 512 * 1024;
      break;
    case 0x44:
// 2nd-level cache: 1MB, 4-way set assoc, 32 byte line size
      l2cache = 1024 * 1024;
      break;
    case 0x45:
// 2nd-level cache: 2MB, 4-way set assoc, 32 byte line size
      l2cache = 2 * 1024 * 1024;
      break;
    case 0x50:
// Instruction TLB: 4KB and 2MB or 4MB pages, 64 entries
      break;
    case 0x51:
// Instruction TLB: 4KB and 2MB or 4MB pages, 128 entries
      break;
    case 0x52:
// Instruction TLB: 4KB and 2MB or 4MB pages, 256 entries
      break;
    case 0x5b:
// Data TLB: 4KB and 4MB pages, 64 entries
      break;
    case 0x5c:
// Data TLB: 4KB and 4MB pages, 128 entries
      break;
    case 0x5d:
// Data TLB: 4KB and 4MB pages, 256 entries
      break;
    case 0x66:
// 1st-level data cache: 8KB, 4-way set assoc, 64 byte line size
      l1cache += 8 * 1024;
      break;
    case 0x67:
// 1st-level data cache: 16KB, 4-way set assoc, 64 byte line size
      l1cache += 16 * 1024;
      break;
    case 0x68:
// 1st-level data cache: 32KB, 4-way set assoc, 64 byte line size
      l1cache += 32 * 1024;
      break;
    case 0x70:
// Trace cache: 12K-micro-op, 4-way set assoc
      break;
    case 0x71:
// Trace cache: 16K-micro-op, 4-way set assoc
      break;
    case 0x72:
// Trace cache: 32K-micro-op, 4-way set assoc
      break;
    case 0x79:
// 2nd-level cache: 128KB, 8-way set assoc, sectored, 64 byte line size
      l2cache += 128 * 1024;
      break;
    case 0x7a:
// 2nd-level cache: 256KB, 8-way set assoc, sectored, 64 byte line size
      l2cache += 256 * 1024;
      break;
    case 0x7b:
// 2nd-level cache: 512KB, 8-way set assoc, sectored, 64 byte line size
      l2cache += 512 * 1024;
      break;
    case 0x7c:
// 2nd-level cache: 1MB, 8-way set assoc, sectored, 64 byte line size
      l2cache += 1024 * 1024;
      break;
    case 0x82:
// 2nd-level cache: 256KB, 8-way set assoc, 32 byte line size
      l2cache += 256 * 1024;
      break;
    case 0x83:
// 2nd-level cache: 512KB, 8-way set assoc 32 byte line size
      l2cache += 512 * 1024;
      break;
    case 0x84:
// 2nd-level cache: 1MB, 8-way set assoc, 32 byte line size
      l2cache += 1024 * 1024;
      break;
    case 0x85:
// 2nd-level cache: 2MB, 8-way set assoc, 32 byte line size
      l2cache += 2 * 1024 * 1024;
      break;
    default:
// unknown TLB/cache descriptor
      break;
  }
}


static bool dointel(unsigned long maxi,
hwNode * cpu,
int cpunumber = 0)
{
  char buffer[1024];
  unsigned long signature = 0, flags = 0, bflags = 0, eax = 0, ebx = 0, ecx = 0, edx = 0, unused = 0;
  int stepping, model, family;

  if (!cpu)
    return false;

  cpu->addHint("logo", string("intel"));

  if (maxi >= 1)
  {
    cpuid(cpunumber, 1, eax, ebx, ecx, edx);

    signature = eax;

    stepping = eax & 0xf;
    model = (eax >> 4) & 0xf;
    family = (eax >> 8) & 0xf;
    flags = edx;
    bflags = ebx;

    snprintf(buffer, sizeof(buffer), "%d.%d.%d", family, model, stepping);
    cpu->setVersion(buffer);

    if(ecx & (1 << 5))
      cpu->addCapability("vmx", _("CPU virtualization (Vanderpool)"));

/* Hyper-Threading Technology */
    if (flags & (1 << 28))
    {
      char buff[20];
      unsigned int nr_ht = (bflags >> 16) & 0xFF;
      unsigned int phys_id = (bflags >> 24) & 0xFF;

      snprintf(buff, sizeof(buff), "%d", phys_id);
      cpu->setConfig("id", buff);

      hwNode logicalcpu("logicalcpu", hw::processor);
      logicalcpu.setDescription(_("Logical CPU"));
      logicalcpu.addCapability("logical", _("Logical CPU"));
      logicalcpu.setWidth(cpu->getWidth());
      logicalcpu.claim();
      cpu->addCapability("ht", _("HyperThreading"));

      if(nr_ht>1)
        for(unsigned int i=0; i< nr_ht; i++)
      {
        snprintf(buff, sizeof(buff), "CPU:%d.%d", phys_id, i);
        logicalcpu.setHandle(buff);
        logicalcpu.setPhysId(phys_id, i+1);
        cpu->addChild(logicalcpu);
        cpu->claim();
      }
    }

  }

  if (maxi >= 2)
  {
/*
 * Decode TLB and cache info
 */
    int ntlb, i;
    long long l1cache = 0, l2cache = 0;

    ntlb = 255;
    for (i = 0; i < ntlb; i++)
    {
      cpuid(cpunumber, 2, eax, ebx, ecx, edx);
      ntlb = eax & 0xff;
      decode_intel_tlb(eax >> 8, l1cache, l2cache);
      decode_intel_tlb(eax >> 16, l1cache, l2cache);
      decode_intel_tlb(eax >> 24, l1cache, l2cache);

      if ((ebx & 0x80000000) == 0)
      {
        decode_intel_tlb(ebx, l1cache, l2cache);
        decode_intel_tlb(ebx >> 8, l1cache, l2cache);
        decode_intel_tlb(ebx >> 16, l1cache, l2cache);
        decode_intel_tlb(ebx >> 24, l1cache, l2cache);
      }
      if ((ecx & 0x80000000) == 0)
      {
        decode_intel_tlb(ecx, l1cache, l2cache);
        decode_intel_tlb(ecx >> 8, l1cache, l2cache);
        decode_intel_tlb(ecx >> 16, l1cache, l2cache);
        decode_intel_tlb(ecx >> 24, l1cache, l2cache);
      }
      if ((edx & 0x80000000) == 0)
      {
        decode_intel_tlb(edx, l1cache, l2cache);
        decode_intel_tlb(edx >> 8, l1cache, l2cache);
        decode_intel_tlb(edx >> 16, l1cache, l2cache);
        decode_intel_tlb(edx >> 24, l1cache, l2cache);
      }
    }

    if (l1cache != 0)
    {
      hwNode *l1 = getcache(*cpu, 0);
      hwNode *l2 = getcache(*cpu, 1);

      if (l1)
      {
        l1->setSize(l1cache);
        if (l1->getDescription() == "")
          l1->setDescription(_("L1 cache"));
      }
      else
      {
        hwNode cache("cache",
          hw::memory);
        cache.setSize(l1cache);
        cache.setDescription(_("L1 cache"));

        cpu->addChild(cache);
      }

      if (l2cache != 0)
      {
        if (l2 && (l2cache != 0))
        {
          l2->setSize(l2cache);
          if (l2->getDescription() == "")
            l2->setDescription(_("L2 cache"));
        }
        else
        {
          hwNode cache("cache",
            hw::memory);
          cache.setSize(l2cache);
          cache.setDescription(_("L2 cache"));

          cpu->addChild(cache);
        }
      }
    }
  }

  if (maxi >= 3)
  {
    cpuid(cpunumber, 3, unused, unused, ecx, edx);

    snprintf(buffer, sizeof(buffer),
      "%04lX-%04lX-%04lX-%04lX-%04lX-%04lX",
      signature >> 16,
      signature & 0xffff,
      edx >> 16, edx & 0xffff, ecx >> 16, ecx & 0xffff);

    cpu->setSerial(buffer);
  }
  else
    cpu->setSerial("");

  return true;
}


static bool doamd(unsigned long maxi,
hwNode * cpu,
int cpunumber = 0)
{
  unsigned long maxei = 0, eax, ebx, ecx, edx;
  long long l1cache = 0, l2cache = 0;
  unsigned int family = 0, model = 0, stepping = 0;
  char buffer[1024];

  if (maxi < 1)
    return false;

  cpu->addHint("logo", string("amd"));

  cpuid(cpunumber, 1, eax, ebx, ecx, edx);
  stepping = eax & 0xf;
  model = (eax >> 4) & 0xf;
  family = (eax >> 8) & 0xf;
  snprintf(buffer, sizeof(buffer), "%d.%d.%d", family, model, stepping);
  cpu->setVersion(buffer);

  cpuid(cpunumber, 0x80000000, maxei, ebx, ecx, edx);

  if (maxei >= 0x80000005)
  {
    cpuid(cpunumber, 0x80000005, eax, ebx, ecx, edx);

    l1cache = (ecx >> 24) * 1024;                 // data cache
    l1cache += (edx >> 24) * 1024;                // instruction cache
  }
  if (maxei >= 0x80000006)
  {
    cpuid(cpunumber, 0x80000006, eax, ebx, ecx, edx);

    l2cache = (ecx >> 16) * 1024;
  }

  if (l1cache != 0)
  {
    hwNode *l1 = cpu->getChild("cache:0");
    hwNode *l2 = cpu->getChild("cache:1");

    if (l1)
      l1->setSize(l1cache);
    else
    {
      hwNode newl1("cache",
        hw::memory);

      newl1.setDescription(_("L1 cache"));
      newl1.setSize(l1cache);

      cpu->addChild(newl1);
    }
    if (l2 && l2cache)
      l2->setSize(l2cache);
    else
    {
      hwNode newl2("cache",
        hw::memory);

      newl2.setDescription(_("L2 cache"));
      newl2.setSize(l2cache);

      if (l2cache)
        cpu->addChild(newl2);
    }
  }

  return true;
}


static bool docyrix(unsigned long maxi,
hwNode * cpu,
int cpunumber = 0)
{
  unsigned long eax, ebx, ecx, edx;
  unsigned int family = 0, model = 0, stepping = 0;
  char buffer[1024];

  if (maxi < 1)
    return false;

  cpuid(cpunumber, 1, eax, ebx, ecx, edx);
  stepping = eax & 0xf;
  model = (eax >> 4) & 0xf;
  family = (eax >> 8) & 0xf;
  snprintf(buffer, sizeof(buffer), "%d.%d.%d", family, model, stepping);
  cpu->setVersion(buffer);

  return true;
}


static __inline__ bool flag_is_changeable_p(unsigned int flag)
{
  unsigned int f1, f2;
  __asm__ volatile ("pushfl\n\t"
    "pushfl\n\t"
    "popl %0\n\t"
    "movl %0,%1\n\t"
    "xorl %2,%0\n\t"
    "pushl %0\n\t"
    "popfl\n\t"
    "pushfl\n\t" "popl %0\n\t" "popfl\n\t":"=&r" (f1),
    "=&r"(f2):"ir"(flag));
  return ((f1 ^ f2) & flag) != 0;
}


static bool haveCPUID()
{
  return flag_is_changeable_p(0x200000);
}


/*
 * Estimate CPU MHz routine by Andrea Arcangeli <andrea@suse.de>
 * Small changes by David Sterba <sterd9am@ss1000.ms.mff.cuni.cz>
 *
 */

static __inline__ unsigned long long int rdtsc()
{
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31":"=A" (x));
  return x;
}


static float estimate_MHz(int cpunum,
long sleeptime = 250000)
{
  struct timezone tz;
  struct timeval tvstart, tvstop;
  unsigned long long int cycles[2];               /* gotta be 64 bit */
  float microseconds;                             /* total time taken */
  unsigned long eax, ebx, ecx, edx;
  double freq = 1.0f;

/*
 * Make sure we have a TSC (and hence RDTSC)
 */
  cpuid(cpunum, 1, eax, ebx, ecx, edx);
  if ((edx & (1 << 4)) == 0)
  {
    return 0;                                     // can't estimate frequency
  }

  memset(&tz, 0, sizeof(tz));

/*
 * get this function in cached memory
 */
  gettimeofday(&tvstart, &tz);
  cycles[0] = rdtsc();
  gettimeofday(&tvstart, &tz);

/*
 * we don't trust that this is any specific length of time
 */
  usleep(sleeptime);

  gettimeofday(&tvstop, &tz);
  cycles[1] = rdtsc();
  gettimeofday(&tvstop, &tz);

  microseconds = (tvstop.tv_sec - tvstart.tv_sec) * 1000000 +
    (tvstop.tv_usec - tvstart.tv_usec);

  return (float) (cycles[1] - cycles[0]) / (microseconds / freq);
}


static float average_MHz(int cpunum,
int tries = 2)
{
  float frequency = 0;

  for (int i = 1; i <= tries; i++)
    frequency += estimate_MHz(cpunum, i * 150000);

  if (tries > 0)
    return frequency / (float) tries;
  else
    return 0;
}


static long round_MHz(float fMHz)
{
  long MHz = (long)fMHz;

  if ((MHz % 50) > 15)
    return ((MHz / 50) * 50) + 50;
  else
    return ((MHz / 50) * 50);
}


bool scan_cpuid(hwNode & n)
{
  unsigned long maxi, ebx, ecx, edx;
  hwNode *cpu = NULL;
  int currentcpu = 0;

  if (!haveCPUID())
    return false;

  while ((cpu = getcpu(n, currentcpu)))
  {
    cpu->claim(true);                             // claim the cpu and all its children
    cpuid(currentcpu, 0, maxi, ebx, ecx, edx);
    maxi &= 0xffff;

    switch (ebx)
    {
      case 0x756e6547:                            /* Intel */
        dointel(maxi, cpu, currentcpu);
        break;
      case 0x68747541:                            /* AMD */
        doamd(maxi, cpu, currentcpu);
        break;
      case 0x69727943:                            /* Cyrix */
        docyrix(maxi, cpu, currentcpu);
        break;
      default:
        return false;
    }

    cpu->claim(true);                             // claim the cpu and all its children
    if (cpu->getSize() == 0)
      cpu->setSize((unsigned long long) (1000000uL * round_MHz(average_MHz(currentcpu))));

    currentcpu++;
  }

  return true;
}


#else

#ifdef __alpha__

#define BWX (1 << 0)
#define FIX (1 << 1)
#define CIX (1 << 2)
#define MVI (1 << 8)
#define PAT (1 << 9)
#define PMI (1 << 12)

bool scan_cpuid(hwNode & n)
{
  hwNode *cpu = NULL;
  int currentcpu = 0;
  unsigned long ver = 0, mask = 0;

  while (cpu = getcpu(n, currentcpu))
  {
    asm("implver %0":"=r"(ver));
    asm("amask %1, %0": "=r"(mask):"r"(-1));

    cpu->setVendor("Digital Equipment Corporation");
    cpu->setProduct("Alpha");
    cpu->setWidth(64);

    if ((~mask) & BWX)
      cpu->addCapability("BWX");
    if ((~mask) & FIX)
      cpu->addCapability("FIX");
    if ((~mask) & CIX)
      cpu->addCapability("CIX");
    if ((~mask) & MVI)
      cpu->addCapability("MVI");
    if ((~mask) & PAT)
      cpu->addCapability("PAT");
    if ((~mask) & PMI)
      cpu->addCapability("PMI");

    switch (ver)
    {
      case 0:
        cpu->setVersion("EV4");
        break;
      case 1:
        switch (~mask)
        {
          case 0:
            cpu->setVersion("EV5");
            break;
          case BWX:
            cpu->setVersion("EV56");
            break;
          case BWX | MVI:
            cpu->setVersion("PCA56");
            break;
          default:
            cpu->setVersion("EV5 unknown");
        }
        break;
      case 2:
        switch (~mask)
        {
          case BWX | FIX | MVI | PAT:
            cpu->setVersion("EV6");
            break;
          case BWX | FIX | MVI | PAT | CIX:
            cpu->setVersion("EV67");
            break;
          case BWX | FIX | MVI | PAT | CIX | PMI:
            cpu->setVersion("EV68");
            break;
          default:
            cpu->setVersion("EV6 unknown");
        }
        break;
      case 3:
        switch (~mask)
        {
          case BWX | FIX | MVI | PAT | CIX | PMI:
            cpu->setVersion("EV7x");
            break;
          default:
            cpu->setVersion("EV7 unknown");
        }
        break;
    }

    currentcpu++;
  }

  return true;
}


#else

bool scan_cpuid(hwNode & n)
{
  return true;
}
#endif                                            /* __alpha__ */
#endif                                            /* __i386__ */
