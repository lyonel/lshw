/*
 * cpufreq.cc
 *
 * This module scans /sys for CPUfreq info
 *
 *
 *
 */

#include "version.h"
#include "hw.h"
#include "osutils.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

__ID("@(#) $Id$");

#define DEVICESCPUFREQ "/sys/devices/system/cpu/cpu%d/cpufreq/"

static long get_long(const string & path)
{
  long result = 0;
  FILE * in = fopen(path.c_str(), "r");

  if (in)
  {
    if(fscanf(in, "%ld", &result) != 1)
      result = 0;
    fclose(in);
  }

  return result;
}


static string cpubusinfo(int cpu)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "cpu@%d", cpu);

  return string(buffer);
}


bool scan_cpufreq(hwNode & node)
{
  char buffer[PATH_MAX];
  unsigned i =0;

  while(hwNode * cpu = node.findChildByBusInfo(cpubusinfo(i)))
  {
    snprintf(buffer, sizeof(buffer), DEVICESCPUFREQ, i);
    if(exists(buffer))
    {
      unsigned long long max, cur;
      pushd(buffer);

                                                  // in Hz
      max = 1000*(unsigned long long)get_long("cpuinfo_max_freq");
                                                  // in Hz
      cur = 1000*(unsigned long long)get_long("scaling_cur_freq");
      cpu->addCapability("cpufreq", "CPU Frequency scaling");
      if(cur) cpu->setSize(cur);
      if(max>cpu->getCapacity()) cpu->setCapacity(max);
      popd();
    }
    i++;
  }

  return true;
}
