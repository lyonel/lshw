/*
 * heuristics.cc
 *
 *
 */

#include "sysfs.h"
#include "osutils.h"

static char *id = "@(#) $Id$";

string guessBusInfo(const string & info)
{
  if((info.length()==12) && (info[4] == ':') && (info[7] == ':') && (info[10] == '.')) // 2.6-style PCI
  {
    return "pci@" + info.substr(info.length() - 7);
  }
  if((info.length()==7) && (info[2] == ':') && (info[5] == '.')) // 2.4-style PCI
  {
    return "pci@" + info;
  }
  return "";
}

hwNode * guessParent(const hwNode & child, hwNode & base)
{
  (void)&id;	// to avoid warning "defined but not used"

  return NULL;
}
