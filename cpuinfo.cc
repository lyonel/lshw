#include "cpuinfo.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>

static int splitlines(const string & s,
		      vector < string > &lines,
		      char separator = '\n')
{
  size_t i = 0, j = 0;
  int count;

  lines.clear();

  while ((j < s.length()) && ((i = s.find(separator, j)) != string::npos))
  {
    lines.push_back(s.substr(j, i - j));
    count++;
    i++;
    j = i;
  }
  if (j < s.length())
  {
    lines.push_back(s.substr(j));
    count++;
  }

  return count;
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
    if (id == "vendor_id")
      cpu->setVendor(value);
    if (id == "model name")
      cpu->setProduct(value);
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

bool scan_cpuinfo(hwNode & n)
{
  hwNode *core = n.getChild("core");
  int cpuinfo = open("/proc/cpuinfo", O_RDONLY);

  if (cpuinfo < 0)
    return false;

  if (!core)
  {
    n.addChild(hwNode("core", hw::system));
    core = n.getChild("core");
  }

  if (core)
  {
    char buffer[1024];
    size_t count;
    hwNode *cpu = core->getChild("cpu");
    string cpuinfo_str = "";

    while ((count = read(cpuinfo, buffer, sizeof(buffer))) > 0)
    {
      cpuinfo_str += string(buffer, count);
    }
    close(cpuinfo);

    vector < string > cpuinfo_lines;
    splitlines(cpuinfo_str, cpuinfo_lines);
    cpuinfo_str = "";		// free memory

    for (int i = 0; i < cpuinfo_lines.size(); i++)
    {
      string id = "";
      string value = "";
      size_t pos = 0;

      pos = cpuinfo_lines[i].find(':');

      if (pos != string::npos)
      {
	id = hw::strip(cpuinfo_lines[i].substr(0, pos));
	value = hw::strip(cpuinfo_lines[i].substr(pos + 1));

	cpuinfo_x86(n, id, value);
      }
    }
  }
  else
  {
    close(cpuinfo);
    return false;
  }
}

static char *id = "@(#) $Id: cpuinfo.cc,v 1.5 2003/01/19 20:55:31 ezix Exp $";
