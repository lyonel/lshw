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
  if(matches(info,"^....:..:..\\..$")) // 2.6-style PCI
  {
    return "pci@" + info.substr(info.length() - 7);
  }
  if(matches(info,"^..:..\\..$")) // 2.4-style PCI
  {
    return "pci@" + info;
  }

  if(matches(info, "^[0-9]+-[0-9]+:[0-9]+\\.[0-9]+$")) // USB: host-port:config.interface
  {
    unsigned host=0, port=0, config=0, interface=0;
    char buffer[20];

    sscanf(info.c_str(), "%u-%u:%u.%u", &host, &port, &config, &interface);
    if((host==0) || (port==0)) return "";

    port--;	// USB ports are numbered from 1, we want it from 0
    snprintf(buffer, sizeof(buffer), "usb@%u:%u", host, port);
    return string(buffer);
  }
  return "";
}

hwNode * guessParent(const hwNode & child, hwNode & base)
{
  (void)&id;	// to avoid warning "defined but not used"

  return NULL;
}
