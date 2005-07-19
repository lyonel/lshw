#include "cpuinfo.h"
#include "osutils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>

static char *id =
  "@(#) $Id$";

static int currentcpu = 0;

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
  {
    cpu->addHint("icon", string("cpu"));
    cpu->claim(true);		// claim the cpu and all its children
    cpu->enable();		// enable it
    return cpu;
  }

  hwNode *core = node.getChild("core");

  if (core)
  {
    hwNode newcpu("cpu", hw::processor);

    newcpu.setBusInfo(cpubusinfo);
    newcpu.claim();
    return core->addChild(newcpu);
  }
  else
    return NULL;
}

#ifdef __powerpc__
static void cpuinfo_ppc(hwNode & node,
			string id,
			string value)
{
  if (id == "processor")
    currentcpu++;

  hwNode *cpu = getcpu(node, currentcpu);

  if (cpu)
  {
    cpu->claim(true);
    if (id == "revision")
      cpu->setVersion(value);
    if (id == "cpu")
      cpu->setProduct(value);
  }
}
#endif

#ifdef __ia64__
static void cpuinfo_ia64(hwNode & node,
			 string id,
			 string value)
{
  unsigned long long frequency = 0;
  int i;

  if (id == "processor")
    currentcpu++;

  hwNode *cpu = getcpu(node, currentcpu);

  if (cpu)
  {
    cpu->claim(true);

    if (id == "cpu number")
    {
      int physicalcpu = 0;

      physicalcpu = atoi(value.c_str());

      if (physicalcpu != currentcpu)
      {
	cpu->addCapability("emulated");
	cpu->addCapability("hyperthreading");
      }
    }

    if (id == "vendor")
    {
      if (value == "GenuineIntel")
	value = "Intel Corp.";
      cpu->setVendor(value);
    }

    if (id == "revision")
      cpu->setVersion(value);

    if (id == "family")
      cpu->setProduct(value);

    if (id == "cpu MHz" && cpu->getSize() == 0)
    {
      double frequency = 0.0;

      frequency = atof(value.c_str());
      cpu->setSize((unsigned long long) (frequency * 1E6));
    }
  }
}
#endif

#ifdef __hppa__
static void cpuinfo_hppa(hwNode & node,
			 string id,
			 string value)
{
  if (id == "processor")
    currentcpu++;

  hwNode *cpu = getcpu(node, currentcpu);

  if (id == "model" && node.getProduct() == "")
    node.setProduct(value);
  if (id == "model name" && node.getDescription() == "")
    node.setDescription(value);
  if (id == "software id" && node.getSerial() == "")
    node.setSerial(value);

  if (cpu)
  {
    cpu->claim(true);

    if (id == "cpu family" && cpu->getVersion() == "")
      cpu->setVersion(value);
    if (id == "cpu" && cpu->getProduct() == "")
      cpu->setProduct(value);
    if (id == "cpu MHz" && cpu->getSize() == 0)
    {
      double frequency = 0.0;

      frequency = atof(value.c_str());
      cpu->setSize((unsigned long long) (frequency * 1E6));
    }
  }
}
#endif

#ifdef __alpha__
static void cpuinfo_alpha(hwNode & node,
			  string id,
			  string value)
{
  static int cpusdetected = 0;
  static int cpusactive = 0;
  unsigned long long frequency = 0;
  int i;

  hwNode *cpu = getcpu(node, 0);

  if (id == "platform string" && node.getProduct() == "")
    node.setProduct(value);
  if (id == "system serial number" && node.getSerial() == "")
    node.setSerial(value);
  if (id == "system type")
    node.setVersion(node.getVersion() + " " + value);
  if (id == "system variation")
    node.setVersion(node.getVersion() + " " + value);
  if (id == "system revision")
    node.setVersion(node.getVersion() + " " + value);

  if (id == "cpus detected")
    cpusdetected = atoi(value.c_str());
  if (id == "cpus active")
    cpusactive = atoi(value.c_str());
  if (id == "cycle frequency [Hz]")
    frequency = atoll(value.c_str());

  if (cpu)
  {
    cpu->claim(true);

    if (frequency)
      cpu->setSize(frequency);
  }

  for (i = 1; i < cpusdetected; i++)
  {
    hwNode *mycpu = getcpu(node, i);

    if (mycpu)
    {
      mycpu->disable();

      if (cpu)
	mycpu->setSize(cpu->getSize());
    }
  }
  for (i = 1; i < cpusactive; i++)
  {
    hwNode *mycpu = getcpu(node, i);

    if (mycpu)
      mycpu->enable();
  }
}
#endif

#if defined(__i386__) || defined(__x86_64__)
static void cpuinfo_x86(hwNode & node,
			string id,
			string value)
{
  static int siblings = -1;

  if(currentcpu < 0) siblings = -1;

  if ((siblings<0) && (id == "siblings"))
  {
    siblings = atoi(value.c_str());
    siblings--;
  }

  if (id == "processor")
  {
    siblings--;

    if(siblings >= 0)
      return;
    else
      currentcpu++;
  }

  hwNode *cpu = getcpu(node, currentcpu);

  if (cpu)
  {

    // x86 CPUs are assumed to be 32 bits per default
    if(cpu->getWidth()==0) cpu->setWidth(32);

    cpu->claim(true);
    if (id == "vendor_id")
    {
      if (value == "AuthenticAMD")
	value = "Advanced Micro Devices [AMD]";
      if (value == "GenuineIntel")
	value = "Intel Corp.";
      cpu->setVendor(value);
    }
    if (id == "model name")
      cpu->setProduct(value);
    //if ((id == "cpu MHz") && (cpu->getSize() == 0))
    //{
    //cpu->setSize((long long) (1000000L * atof(value.c_str())));
    //}
    if (id == "Physical processor ID")
      cpu->setSerial(value);
    if ((id == "fdiv_bug") && (value == "yes"))
      cpu->addCapability("fdiv_bug");
    if ((id == "hlt_bug") && (value == "yes"))
      cpu->addCapability("hlt_bug");
    if ((id == "f00f_bug") && (value == "yes"))
      cpu->addCapability("f00f_bug");
    if ((id == "coma_bug") && (value == "yes"))
      cpu->addCapability("coma_bug");
    if ((id == "fpu") && (value == "yes"))
      cpu->addCapability("fpu");
    if ((id == "wp") && (value == "yes"))
      cpu->addCapability("wp");
    if ((id == "fpu_exception") && (value == "yes"))
      cpu->addCapability("fpu_exception", "FPU exceptions reporting");
    if (id == "flags")
      while (value.length() > 0)
      {
	size_t pos = value.find(' ');
        string capability = (pos==string::npos)?value:value.substr(0, pos);

        if(capability == "lm") capability = "x86-64";

	cpu->addCapability(capability);

	if (pos == string::npos)
	  value = "";
	else
	  value = hw::strip(value.substr(pos));
      }

    cpu->describeCapability("fpu", "mathematical co-processor");
    cpu->describeCapability("vme", "virtual mode extensions");
    cpu->describeCapability("de", "debugging extensions");
    cpu->describeCapability("pse", "page size extensions");
    cpu->describeCapability("tsc", "time stamp counter");
    cpu->describeCapability("msr", "model-specific registers");
    cpu->describeCapability("mce", "machine check exceptions");
    cpu->describeCapability("cx8", "compare and exchange 8-byte");
    cpu->describeCapability("apic", "on-chip advanced programmable interrupt controller (APIC)");
    cpu->describeCapability("sep", "fast system calls");
    cpu->describeCapability("mtrr", "memory type range registers");
    cpu->describeCapability("pge", "page global enable");
    cpu->describeCapability("mca", "machine check architecture");
    cpu->describeCapability("cmov", "conditional move instruction");
    cpu->describeCapability("pat", "page attribute table");
    cpu->describeCapability("pse36", "36-bit page size extensions");
    cpu->describeCapability("pn", "processor serial number");
    cpu->describeCapability("psn", "processor serial number");
    //cpu->describeCapability("clflush", "");
    cpu->describeCapability("dts", "debug trace and EMON store MSRs");
    cpu->describeCapability("acpi", "thermal control (ACPI)");
    cpu->describeCapability("fxsr", "fast floating point save/restore");
    cpu->describeCapability("sse", "streaming SIMD extensions (SSE)");
    cpu->describeCapability("sse2", "streaming SIMD extensions (SSE2)");
    cpu->describeCapability("ss", "self-snoop");
    cpu->describeCapability("tm", "thermal interrupt and status");
    cpu->describeCapability("ia64", "IA-64 (64-bit Intel CPU)");
    cpu->describeCapability("pbe", "pending break event");
    cpu->describeCapability("syscall", "fast system calls");
    cpu->describeCapability("mp", "multi-processor capable");
    cpu->describeCapability("nx", "no-execute bit (NX)");
    cpu->describeCapability("mmxext", "multimedia extensions (MMXExt)");
    cpu->describeCapability("3dnowext", "multimedia extensions (3DNow!Ext)");
    cpu->describeCapability("3dnow", "multimedia extensions (3DNow!)");
    //cpu->describeCapability("recovery", "");
    cpu->describeCapability("longrun", "LongRun Dynamic Power/Thermal Management");
    cpu->describeCapability("lrti", "LongRun Table Interface");
    cpu->describeCapability("cxmmx", "multimedia extensions (Cyrix MMX)");
    cpu->describeCapability("k6_mtrr", "AMD K6 MTRRs");
    //cpu->describeCapability("cyrix_arr", "");
    //cpu->describeCapability("centaur_mcr", "");
    //cpu->describeCapability("pni", "");
    //cpu->describeCapability("monitor", "");
    //cpu->describeCapability("ds_cpl", "");
    //cpu->describeCapability("est", "");
    //cpu->describeCapability("tm2", "");
    //cpu->describeCapability("cid", "");
    //cpu->describeCapability("xtpr", "");
    cpu->describeCapability("rng", "random number generator");
    cpu->describeCapability("rng_en", "random number generator (enhanced)");
    cpu->describeCapability("ace", "advanced cryptography engine");
    cpu->describeCapability("ace_en", "advanced cryptography engine (enhanced)");
    cpu->describeCapability("ht", "HyperThreading");
    cpu->describeCapability("lm", "64bits extensions (x86-64)");
    cpu->describeCapability("x86-64", "64bits extensions (x86-64)");
    cpu->describeCapability("mmx", "multimedia extensions (MMX)");
    cpu->describeCapability("pae", "4GB+ memory addressing (Physical Address Extension)");

    if(cpu->isCapable("ia64") || cpu->isCapable("lm") || cpu->isCapable("x86-64"))
      cpu->setWidth(64);

    if(node.getWidth()==0) node.setWidth(cpu->getWidth());
  }
}
#endif

bool scan_cpuinfo(hwNode & n)
{
  hwNode *core = n.getChild("core");
  int cpuinfo = open("/proc/cpuinfo", O_RDONLY);

  if (cpuinfo < 0)
    return false;

  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  if (core)
  {
    char buffer[1024];
    size_t count;
    string cpuinfo_str = "";

    while ((count = read(cpuinfo, buffer, sizeof(buffer))) > 0)
    {
      cpuinfo_str += string(buffer, count);
    }
    close(cpuinfo);

    vector < string > cpuinfo_lines;
    splitlines(cpuinfo_str, cpuinfo_lines);
    cpuinfo_str = "";		// free memory
    currentcpu = -1;

    for (unsigned int i = 0; i < cpuinfo_lines.size(); i++)
    {
      string id = "";
      string value = "";
      size_t pos = 0;

      pos = cpuinfo_lines[i].find(':');

      if (pos != string::npos)
      {
	id = hw::strip(cpuinfo_lines[i].substr(0, pos));
	value = hw::strip(cpuinfo_lines[i].substr(pos + 1));

#if defined(__i386__) || defined(__x86_64__)
	cpuinfo_x86(n, id, value);
#endif
#ifdef __powerpc__
	cpuinfo_ppc(n, id, value);
#endif
#ifdef __hppa__
	cpuinfo_hppa(n, id, value);
#endif
#ifdef __alpha__
	cpuinfo_alpha(n, id, value);
#endif
#ifdef __ia64__
	cpuinfo_ia64(n, id, value);
#endif
      }
    }
  }
  else
  {
    close(cpuinfo);
    return false;
  }

  hwNode *cpu = getcpu(n, 0);
  if(cpu && (n.getWidth()==0))
    n.setWidth(cpu->getWidth());

  (void) &id;			// avoid warning "id defined but not used"

  return true;
}
