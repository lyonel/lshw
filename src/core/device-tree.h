#ifndef _DEVICETREE_H_
#define _DEVICETREE_H_

#include "hw.h"

bool scan_device_tree(hwNode & n);

void add_device_tree_info(hwNode & n, string sysfs_path);
#endif
