#include "cpuid.h"

#define cpuid(in,a,b,c,d)\
  asm("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (in));

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

  return node.addChild(hwNode("cache", hw::memory));
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
  unsigned long signature, eax, ebx, ecx, edx, unused;
  int stepping, model, family, type, reserved, brand, siblings;

  if (maxi >= 1)
  {
    cpuid(1, eax, ebx, ecx, edx);

    signature = eax;

    stepping = eax & 0xf;
    model = (eax >> 4) & 0xf;
    family = (eax >> 8) & 0xf;

    snprintf(buffer, sizeof(buffer), "%d.%d.%d", family, model, stepping);
    cpu->setVersion(buffer);
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
      cpuid(2, eax, ebx, ecx, edx);
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

    if (l1cache != 0 && l2cache != 0)
    {
      hwNode *l1 = cpu->getChild("cache:0");
      hwNode *l2 = cpu->getChild("cache:1");

      if (l1)
	l1->setSize(l1cache);
      else
      {
	hwNode newl1("cache",
		     hw::memory);

	newl1.setDescription("L1 cache");
	newl1.setSize(l1cache);

	cpu->addChild(newl1);
      }
      if (l2 && l2cache)
	l2->setSize(l1cache);
      else
      {
	hwNode newl2("cache",
		     hw::memory);

	newl2.setDescription("L2 cache");
	newl2.setSize(l2cache);

	cpu->addChild(newl2);
      }
    }
  }

  if (maxi >= 3)
  {
    cpuid(3, unused, unused, ecx, edx);

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
  return true;
}

static bool docyrix(unsigned long maxi,
		    hwNode * cpu,
		    int cpunumber = 0)
{
  return true;
}

static hwNode *getcpu(hwNode & node,
		      int n = 0)
{
  char cpuname[10];
  hwNode *cpu = NULL;

  if (n < 0)
    n = 0;

  snprintf(cpuname, sizeof(cpuname), "cpu:%d", n);
  cpu = node.getChild(string("core/") + string(cpuname));

  if (cpu)
    return cpu;

  if (n > 0)
    return NULL;

  // "cpu:0" is equivalent to "cpu" on 1-CPU machines
  if ((n == 0) && (node.countChildren(hw::processor) <= 1))
    cpu = node.getChild(string("core/cpu"));
  if (cpu)
    return cpu;

  hwNode *core = node.getChild("core");

  if (core)
    return core->addChild(hwNode("cpu", hw::processor));
  else
    return NULL;
}

bool scan_cpuid(hwNode & n)
{
  unsigned long maxi, unused, eax, ebx, ecx, edx;
  hwNode *cpu = NULL;
  int currentcpu = 0;

  while (cpu = getcpu(n, currentcpu))
  {
    cpuid(0, maxi, ebx, ecx, edx);
    maxi &= 0xffff;

    switch (ebx)
    {
    case 0x756e6547:		/* Intel */
      dointel(maxi, cpu, currentcpu);
      break;
    case 0x68747541:		/* AMD */
      doamd(maxi, cpu, currentcpu);
      break;
    case 0x69727943:		/* Cyrix */
      docyrix(maxi, cpu, currentcpu);
      break;
    default:
      return false;
    }

    currentcpu++;
  }

  return true;
}

static char *id = "@(#) $Id: cpuid.cc,v 1.1 2003/02/02 15:43:42 ezix Exp $";
