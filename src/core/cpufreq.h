#ifndef _CPUFREQ_H_
#define _CPUFREQ_H_

#include "hw.h"

class sysfsData
{
  unsigned long long max_freq;
  unsigned long long cur_freq;

public:
  unsigned long long get_max_freq () const;
  void set_max_freq (unsigned long long max_freq);

  unsigned long long get_cur_freq () const;
  void set_cur_freq (unsigned long long cur_freq);
};

bool scan_cpufreq(hwNode & n);
#endif
