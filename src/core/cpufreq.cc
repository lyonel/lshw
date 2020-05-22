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
#include "cpufreq.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <map>
#include <iostream>
#include <stdexcept>
#include <sstream>

__ID("@(#) $Id$");

#define DEVICESCPU "/sys/devices/system/cpu/cpu%d"
#define DEVICESCPUFREQ DEVICESCPU "/cpufreq/"

using namespace std;

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

static unsigned short get_ushort(const string & path)
{
  unsigned short result = 0;
  FILE * in = fopen(path.c_str(), "r");

  if (in)
  {
    if(fscanf(in, "%hu", &result) != 1)
      result = 0;
    fclose(in);
  }

  return result;
}

static unsigned int get_uint(const string & path)
{
  unsigned int result = 0;
  FILE * in = fopen(path.c_str(), "r");

  if (in)
  {
    if(fscanf(in, "%u", &result) != 1)
      result = 0;
    fclose(in);
  }

  return result;
}

unsigned long long sysfsData::get_max_freq () const
{
  return max_freq;
}

void sysfsData::set_max_freq (unsigned long long max_freq)
{
  this->max_freq = max_freq;
}

unsigned long long sysfsData::get_cur_freq () const
{
  return cur_freq;
}

void sysfsData::set_cur_freq (unsigned long long cur_freq)
{
  this->cur_freq = cur_freq;
}

bool scan_cpufreq(hwNode & node)
{

  char buffer[PATH_MAX];
  unsigned i =0;
  string desc = node.getDescription();

  if (desc=="PowerNV" || desc=="pSeries Guest" || desc=="pSeries LPAR")
  {
    map <uint32_t, sysfsData> core_to_data;
    char buffer_online[PATH_MAX] = {0};

    snprintf(buffer, sizeof(buffer), DEVICESCPU, i);

    strncpy(buffer_online, buffer, sizeof(buffer_online) - 1);
    buffer_online[sizeof(buffer_online) - 1] = '\0';

    strncat(buffer_online, "/online",
      sizeof(buffer_online) - strlen(buffer_online) - 1);

    while(exists(buffer))
    {
      if (get_ushort(buffer_online) == 1)
      {
        //read data and add to map
        sysfsData data;
        unsigned long long max, cur;
        uint32_t core_id;
        string buffer_coreid = "";

        pushd(buffer);

        max = 1000*(unsigned long long)get_long("cpufreq/cpuinfo_max_freq");
        cur = 1000*(unsigned long long)get_long("cpufreq/scaling_cur_freq");

        if (max==0 && cur==0)
        {
          popd();
          snprintf(buffer, sizeof(buffer), DEVICESCPU, ++i);
          continue;
        }

        data.set_max_freq(max);
        data.set_cur_freq(cur);

        buffer_coreid = string(buffer) + string("/topology/core_id");
        if (exists(buffer_coreid))
        {
          core_id = get_uint(buffer_coreid);
          core_to_data.insert(pair<uint32_t, sysfsData>(core_id, data));
        }

        popd();
      }
      snprintf(buffer, sizeof(buffer), DEVICESCPU, ++i);
    }

    i=0;
    hwNode *cpu;
    while((cpu = node.findChildByBusInfo(cpubusinfo(i))) &&
          !core_to_data.empty())
    {
      sysfsData data;
      string physId;
      uint32_t reg;
      stringstream ss;
      try
      {
        unsigned long long cur, max;

        physId = cpu->getPhysId();
        ss<<physId;
        ss>>reg;
        data = core_to_data.at(reg);

        /* We come here only when some sysfsData value corresponding to reg
         * is found in the core_to_data map.
	 */
        core_to_data.erase(reg);//Erase for making next searches faster

        cur = data.get_cur_freq();
        max = data.get_max_freq();
        cpu->addCapability("cpufreq", "CPU Frequency scaling");
        cpu->setSize(cur);
        if(max > cpu->getCapacity())
          cpu->setCapacity(max);
      }
      catch(const out_of_range& oor)
      {
        /* Do nothing. Coming here indicates either:
         * 1. kernel does not have cpufreq module loaded and hence no map entry
         *    matching reg key is found. Catch is encountered for each cpu-core
         *    and we skip adding cpufreq capability for all cores.
         * 2. All SMT threads are offline for this cpu-core. We skip adding
         *    cpufreq capability for this cpu-core.
         */
      }
      i++;
    }
  }
  else
  {
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
  }

  return true;
}
