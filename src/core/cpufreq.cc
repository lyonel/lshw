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

static long get_ushort(const string & path)
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

static long get_ulong(const string & path)
{
  unsigned long result = 0;
  FILE * in = fopen(path.c_str(), "r");

  if (in)
  {
    if(fscanf(in, "%lu", &result) != 1)
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
    map <unsigned long, sysfsData> core_to_data;
    char buffer_online[PATH_MAX] = {0};

    snprintf(buffer, sizeof(buffer), DEVICESCPU, i);

    strcpy(buffer_online, buffer);
    strcat(buffer_online, "/online");

    while(exists(buffer))
    {
      if (get_ushort(buffer_online) == 1)
      {
        //read data and add to map
        sysfsData data;
        unsigned long long max, cur;
        unsigned long core_id;

        pushd(buffer);

        max = 1000*(unsigned long long)get_long("cpufreq/cpuinfo_max_freq");
        cur = 1000*(unsigned long long)get_long("cpufreq/scaling_cur_freq");
        data.set_max_freq(max);
        data.set_cur_freq(cur);

        string buffer_coreid = string(buffer) + string("/topology/core_id");
        if (exists(buffer_coreid))
        {
          core_id = get_ulong(buffer_coreid);
          core_to_data.insert(pair<unsigned long, sysfsData>(core_id, data));
        }

        popd();
      }
      snprintf(buffer, sizeof(buffer), DEVICESCPU, ++i);
    }

    i=0;
    while(hwNode *cpu = node.findChildByBusInfo(cpubusinfo(i)))
    {
      sysfsData data;
      unsigned long reg;
      try
      {
        unsigned long long max;

        reg = cpu->getReg();
        data = core_to_data.at(reg);
        core_to_data.erase(reg);//Erase for making next searches faster
        cpu->addCapability("cpufreq", "CPU Frequency scaling");
        cpu->setSize(data.get_cur_freq());
        max = data.get_max_freq();
        if(max>cpu->getCapacity())
          cpu->setCapacity(max);
      }
      catch(const out_of_range& oor)
      {
        cerr<<"key ("<<reg<<") not an element of the core-to-sysfs-data map: ";
        cerr<<oor.what()<<endl;
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
