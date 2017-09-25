/*
 * pnp.cc
 *
 *
 *
 *
 *
 */
#include "version.h"
#include "pnp.h"
#include "sysfs.h"
#include "osutils.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <map>
#include <iostream>

__ID("@(#) $Id$");

using namespace std;

#define PNPVENDORS_PATH DATADIR"/pnp.ids:/usr/share/lshw/pnp.ids:/usr/share/hwdata/pnp.ids"
#define PNPID_PATH DATADIR"/pnpid.txt:/usr/share/lshw/pnpid.txt:/usr/share/hwdata/pnpid.txt"

static bool pnpdb_loaded = false;

static map < string, string > pnp_vendors;
static map < string, string > pnp_ids;

static void parse_pnp_vendors(const vector < string > & lines)
{
  for (vector < string >::const_iterator it = lines.begin();
      it != lines.end(); ++it)
  {
    const string & line = *it;
    if (line.length() < 5 || line.at(3) != '\t')
      continue;
    string id = line.substr(0, 3);
    id[0] = toupper(id[0]);
    id[1] = toupper(id[1]);
    id[2] = toupper(id[2]);
    string name = line.substr(4);
    // Microsoft is not really the manufacturer of all PNP devices
    if (id == "PNP")
      continue;
    pnp_vendors[id] = name;
  }
}

static void parse_pnp_ids(const vector < string > & lines)
{
  for (vector < string >::const_iterator it = lines.begin();
      it != lines.end(); ++it)
  {
    const string & line = *it;
    if (line.length() < 1 || line.at(0) == ';')
      continue;
    if (line.length() < 9 || line.at(7) != '\t')
      continue;
    string id = line.substr(0, 7);
    id[0] = toupper(id[0]);
    id[1] = toupper(id[1]);
    id[2] = toupper(id[2]);
    id[3] = tolower(id[3]);
    id[4] = tolower(id[4]);
    id[5] = tolower(id[5]);
    id[6] = tolower(id[6]);
    string desc = line.substr(8);
    pnp_ids[id] = desc;
  }
}

static void load_pnpdb()
{
  vector < string > lines;
  vector < string > filenames;

  splitlines(PNPVENDORS_PATH, filenames, ':');
  for (int i = filenames.size() - 1; i >= 0; i--)
  {
    lines.clear();
    if (loadfile(filenames[i], lines))
      parse_pnp_vendors(lines);
  }

  filenames.clear();
  splitlines(PNPID_PATH, filenames, ':');
  for (int i = filenames.size() - 1; i >= 0; i--)
  {
    lines.clear();
    if (loadfile(filenames[i], lines))
      parse_pnp_ids(lines);
  }

  pnpdb_loaded = true;
}

string pnp_vendorname(const string & id)
{
  if (!pnpdb_loaded)
    load_pnpdb();

  string vendorid = id.substr(0, 3);
  map < string, string >::const_iterator lookup = pnp_vendors.find(vendorid);
  if (lookup != pnp_vendors.end())
    return lookup->second;
  return "";
}

string pnp_description(const string & id)
{
  if (!pnpdb_loaded)
    load_pnpdb();

  map < string, string >::const_iterator lookup = pnp_ids.find(id);
  if (lookup != pnp_ids.end())
    return lookup->second;
  return "";
}

hw::hwClass pnp_class(const string & pnpid)
{
  long numericid = 0;

  if (pnpid.substr(0, 3) != "PNP")
    return hw::generic;

  numericid = strtol(pnpid.substr(3).c_str(), NULL, 16);

  if (numericid < 0x0300)
    return hw::system;
  if (numericid >= 0x300 && numericid < 0x0400)
    return hw::input;                             // keyboards
  if (numericid >= 0x400 && numericid < 0x0500)
    return hw::printer;                           // parallel
  if (numericid >= 0x500 && numericid < 0x0600)
    return hw::communication;                     // serial
  if (numericid >= 0x600 && numericid < 0x0800)
    return hw::storage;                           // ide
  if (numericid >= 0x900 && numericid < 0x0A00)
    return hw::display;                           // vga
  if (numericid == 0x802)
    return hw::multimedia;                        // Microsoft Sound System compatible device
  if (numericid >= 0xA00 && numericid < 0x0B00)
    return hw::bridge;                            // isa,pci...
  if (numericid >= 0xE00 && numericid < 0x0F00)
    return hw::bridge;                            // pcmcia
  if (numericid >= 0xF00 && numericid < 0x1000)
    return hw::input;                             // mice
  if (numericid < 0x8000)
    return hw::system;

  if (numericid >= 0x8000 && numericid < 0x9000)
    return hw::network;                           // network
  if (numericid >= 0xA000 && numericid < 0xB000)
    return hw::storage;                           // scsi&cd
  if (numericid >= 0xB000 && numericid < 0xC000)
    return hw::multimedia;                        // multimedia
  if (numericid >= 0xC000 && numericid < 0xD000)
    return hw::communication;                     // modems

  return hw::generic;
}

bool scan_pnp(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_bus("pnp");

  if (entries.empty())
    return false;

  hwNode *core = n.getChild("core");
  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    vector < string > pnpids = e.multiline_attr("id");
    if (pnpids.empty())
      continue;
    // devices can report multiple PnP IDs, just pick the first
    string pnpid = pnpids[0];

    hwNode device("pnp" + e.name(), pnp_class(pnpid));
    device.addCapability("pnp");
    string driver = e.driver();
    if (!driver.empty())
      device.setConfig("driver", driver);
    string vendor = pnp_vendorname(pnpid);
    if (!vendor.empty())
      device.setVendor(vendor);
    string name = e.string_attr("name");
    string description = pnp_description(pnpid);
    if (!name.empty())
      device.setProduct(name);
    else if (!description.empty())
      device.setProduct(description);
    else
      device.setProduct("PnP device " + pnpid);
    device.claim();

    core->addChild(device);
  }
  return true;
}
