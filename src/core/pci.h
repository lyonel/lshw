#ifndef _PCI_H_
#define _PCI_H_

#include "hw.h"

bool scan_pci(hwNode & n);
bool scan_pci_legacy(hwNode & n);
#endif
