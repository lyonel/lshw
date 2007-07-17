#ifndef _PRINT_H_
#define _PRINT_H_

#include "hw.h"

void print(hwNode & node, bool html=true, int level = 0);
void printhwpath(hwNode & node);
void printbusinfo(hwNode & node);

void status(const char *);
#endif
