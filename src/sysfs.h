#ifndef _SYSFS_H_
#define _SYSFS_H_

#include "hw.h"

bool scan_sysfs(hwNode & n);

std::string sysfs_getbusinfo(const std::string & devclass, const std::string & devname);

std::string sysfs_driver(const std::string & devclass, const std::string & devname);

//hwNode * findParent(hwNode hwNode & system, 

#endif
