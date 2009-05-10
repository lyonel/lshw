#ifndef _HEURISTICS_H_
#define _HEURISTICS_H_

#include "hw.h"

hwNode * guessParent(const hwNode & child, hwNode & base);

string guessBusInfo(const string &);
string guessParentBusInfo(const string &);
bool guessVendor(hwNode & device);
bool guessProduct(hwNode & device);

#endif
