#include "cpuinfo.h"
#include <stdio.h>

bool scan_cpuinfo(hwNode & n)
{
  hwNode *core = n.getChild("core");
  FILE *cpuinfo = fopen("/proc/cpuinfo", "r");

  if (!cpuinfo)
    return false;

  if (!core)
  {
    n.addChild(hwNode("core", hw::system));
    core = n.getChild("core");
  }

  if (core)
  {
    hwNode *cpu = core->getChild("cpu");

    fclose(cpuinfo);
  }
  else
  {
    fclose(cpuinfo);
    return false;
  }
}
