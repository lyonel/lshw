#ifndef _SYSFS_H_
#define _SYSFS_H_

#include <string>
#include "hw.h"

using namespace std;

namespace sysfs
{

  class entry
  {
    public:

      static entry byBus(string devbus, string devname);
      static entry byClass(string devclass, string devname);

      entry & operator =(const entry &);
      entry(const entry &);
      ~entry();

      bool hassubdir(const string &);

      struct entry_i * This;

    private:
      entry();

  };

}                                                 // namespace sysfs


bool scan_sysfs(hwNode & n);

std::string sysfs_getbusinfo(const sysfs::entry &);
std::string sysfs_finddevice(const string &name);
#endif
