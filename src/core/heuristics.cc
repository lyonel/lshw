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
  if(matches(info,"^[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]:[[:xdigit:]][[:xdigit:]]:[[:xdigit:]][[:xdigit:]]\\.[[:xdigit:]]$")) // 2.6-style PCI
  {
    return "pci@" + info.substr(5);
  }
  if(matches(info,"^[[:xdigit:]][[:xdigit:]]:[[:xdigit:]][[:xdigit:]]\\.[[:xdigit:]]$")) // 2.4-style PCI
  {
    return "pci@" + info;
  }

  //	1-2.4:1.0	->	usb@1:2.4
  if(matches(info, "^[0-9]+-[0-9]+(\\.[0-9]+)*:[0-9]+\\.[0-9]+$")) // USB: host-port[.port]:config.interface
  {
    size_t colon = info.rfind(":");
    size_t dash = info.find("-");
    
    return "usb@" + info.substr(0, dash) + ":" + info.substr(dash+1, colon-dash-1);
  }

  if(matches(info, "^parisc[0-9]+(:parisc[0-9]+)*$")) // PA-RISC: pariscx:y:z:t
  {
    return "parisc@" + info.substr(5);
  }
  return "";
}

hwNode * guessParent(const hwNode & child, hwNode & base)
{
  (void)&id;	// to avoid warning "defined but not used"

  return NULL;
}
