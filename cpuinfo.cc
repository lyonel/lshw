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
  {
    cpu->claim(true);		// claim the cpu and all its children
    return cpu;
  }

  // "cpu:0" is equivalent to "cpu" on 1-CPU machines
  if ((n == 0) && (node.countChildren(hw::processor) <= 1))
    cpu = node.getChild(string("core/cpu"));
  if (cpu)
  {
    cpu->claim(true);
    return cpu;
  }

  hwNode *core = node.getChild("core");

  if (core)
    return core->addChild(hwNode("cpu", hw::processor));
  else
    return NULL;
}

#ifdef __powerpc__
static void cpuinfo_ppc(hwNode & node,
			string id,
			string value)
{
  static int currentcpu = 0;

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
  static int currentcpu = -1;

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
  unsigned long long frequency = 0;
  static int currentcpu = -1;

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

    if (id == "cpu" && cpu->getVersion() == "")
      cpu->setVersion(value);
    if (id == "cpu family" && cpu->getProduct() == "")
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

#ifdef __i386__
static void cpuinfo_x86(hwNode & node,
			string id,
			string value)
{
  static int currentcpu = -1;

  if (id == "processor")
    currentcpu++;

  hwNode *cpu = getcpu(node, currentcpu);

  if (cpu)
  {
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
      cpu->addCapability("fpu_exception");
    if (id == "flags")
      while (value.length() > 0)
      {
	size_t pos = value.find(' ');

	if (pos == string::npos)
	{
	  cpu->addCapability(value);
	  value = "";
	}
	else
	{
	  cpu->addCapability(value.substr(0, pos));
	  value = hw::strip(value.substr(pos));
	}
      }
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

#ifdef __i386__
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

  (void) &id;			// avoid warning "id defined but not used"

  return true;
}
