/*
 * heuristics.cc
 *
 *
 */

#include "version.h"
#include "sysfs.h"
#include "osutils.h"

__ID("@(#) $Id$");

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

  if(matches(info, "^[0-9]+-[0-9]+(\\.[0-9]+)*:[0-9]+\\.[0-9]+$")) // USB: host-port[.port]:config.interface
  {
    size_t colon = info.rfind(":");
    size_t dash = info.find("-");
    
    return "usb@" + info.substr(0, dash) + ":" + info.substr(dash+1, colon-dash-1);
  }

  if(matches(info, "^[[:xdigit:]]+-[0-9]+$")) // Firewire: guid-function
  {
    size_t dash = info.find("-");
    
    return "firewire@" + info.substr(0, dash);
  }

#if 1
//#ifdef __hppa__
  if(matches(info, "^[0-9]+(:[0-9]+)*$")) // PA-RISC: x:y:z:t corresponds to /x/y/z/t
  {
    string result = "parisc@";

    for(unsigned i=0; i<info.length(); i++)
      if(info[i] == ':')
        result += '/';
      else
        result += info[i];
    return result;
  }
#endif
  return "";
}

static string guessParentBusInfo(const hwNode & child)
{
  string sysfs_path = sysfs_finddevice(child.getLogicalName());
  vector < string > path;
  string result = "";

  if(sysfs_path == "") return "";

  splitlines(sysfs_path, path, '/');

  if(path.size()>1)
    path.pop_back();
  else
    return "";

  while((result=="") && (path.size()>1))
  {
    result = guessBusInfo(path[path.size()-1]);
    path.pop_back();
  }
  return result;
}

hwNode * guessParent(const hwNode & child, hwNode & base)
{
  return base.findChildByBusInfo(guessParentBusInfo(child));
}
