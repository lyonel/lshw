/*
 * print.cc
 *
 * This module prints out the hardware tree depending on options passed
 * Output can be plain text (good for human beings), HTML (good for web brow-
 * sers), XML (good for programs) or "hardware path" (ioscan-like).
 *
 */

#include "print.h"
#include "options.h"
#include "version.h"
#include "osutils.h"
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <stdio.h>

static char *id = "@(#) $Id$";

static void tab(int level,
		bool connect = true)
{
  if (level <= 0)
    return;
  for (int i = 0; i < level - 1; i++)
    cout << "   ";
  cout << "  ";
  if (connect)
    cout << "*-";
  else
    cout << "  ";
}

static void decimalkilos(unsigned long long value)
{
  const char *prefixes = "KMGTPEZY";
  unsigned int i = 0;

  while ((i <= strlen(prefixes)) && ((value > 10000) || (value % 1000 == 0)))
  {
    value = value / 1000;
    i++;
  }

  cout << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    cout << prefixes[i - 1];
}

static void kilobytes(unsigned long long value)
{
  const char *prefixes = "KMGTPEZY";
  unsigned int i = 0;

  while ((i <= strlen(prefixes)) && ((value > 10240) || (value % 1024 == 0)))
  {
    value = value >> 10;
    i++;
  }

  cout << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    cout << prefixes[i - 1];
  cout << "B";
}

void print(hwNode & node,
	   bool html,
	   int level)
{
  vector < string > config;
  vector < string > resources;
  if (html)
    config = node.getConfigValues("</td><td>=</td><td>");
  else
    config = node.getConfigValues("=");
  if (html)
    resources = node.getResources("</td><td>:</td><td>");
  else
    resources = node.getResources(":");

  if (html && (level == 0))
  {
    cout <<
      "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" <<
      endl;
    cout << "<html>" << endl;
    cout << "<head>" << endl;
    cout << "<meta name=\"generator\" " <<
      " content=\"lshw-" << getpackageversion() << "\">" << endl;
    cout << "<title>";
    cout << node.getId();
    cout << "</title>" << endl;
    cout << "</head>" << endl;
    cout << "<body>" << endl;
  }

  if (visible(node.getClassName()))
  {
    tab(level, !html);

    if (html)
      cout << "<b>";
    if (html && (!node.claimed() || node.disabled()))
      cout << "<font color=\"gray\">";
    cout << node.getId();
    if (node.disabled())
      cout << " DISABLED";
    if (!node.claimed())
      cout << " UNCLAIMED";
    if (html && (!node.claimed() || node.disabled()))
      cout << "</font>";
    if (html)
      cout << "</b><br>";
    cout << endl;

    if (html)
    {
      tab(level, false);
      cout << "<div style=\"margin-left: 2em\">" << endl;
      tab(level, false);
      cout << "<table bgcolor=\"#e8e0e0\"";
      cout << " summary=\"attributes of " << node.getId() << "\">" << endl;
    }
#if 0
    if (node.getHandle() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "handle: ";
      if (html)
	cout << "</td><td>";
      cout << node.getHandle();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }
#endif

    if (node.getDescription() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "description: ";
      if (html)
	cout << "</td><td>";
      cout << node.getDescription();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getProduct() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "product: ";
      if (html)
	cout << "</td><td>";
      cout << node.getProduct();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getVendor() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "vendor: ";
      if (html)
	cout << "</td><td>";
      cout << node.getVendor();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getPhysId() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "physical id: ";
      if (html)
	cout << "</td><td>";
      cout << node.getPhysId();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getBusInfo() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "bus info: ";
      if (html)
	cout << "</td><td>";
      cout << node.getBusInfo();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getLogicalName() != "")
    {
      vector<string> logicalnames = node.getLogicalNames();

      for(unsigned int i = 0; i<logicalnames.size(); i++)
      {
      	tab(level + 1, false);
      	if (html)
		cout << "<tr><td>";
      	cout << "logical name: ";
      	if (html)
		cout << "</td><td>";
      	cout << logicalnames[i];
      	if (html)
		cout << "</td></tr>";
      	cout << endl;
      }
    }

    if (node.getVersion() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "version: ";
      if (html)
	cout << "</td><td>";
      cout << node.getVersion();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getSerial() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "serial: ";
      if (html)
	cout << "</td><td>";
      cout << node.getSerial();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getSlot() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "slot: ";
      if (html)
	cout << "</td><td>";
      cout << node.getSlot();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getSize() > 0)
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "size: ";
      if (html)
	cout << "</td><td>";
      switch (node.getClass())
      {
      case hw::display:
      case hw::memory:
      case hw::address:
      case hw::storage:
      case hw::disk:
	kilobytes(node.getSize());
	if (html)
	  cout << "</td></tr>";
	break;

      case hw::processor:
      case hw::bus:
      case hw::system:
	decimalkilos(node.getSize());
	cout << "Hz";
	if (html)
	  cout << "</td></tr>";
	break;

      case hw::network:
	decimalkilos(node.getSize());
	cout << "B/s";
	if (html)
	  cout << "</td></tr>";
	break;

      case hw::power:
	cout << node.getSize();
	cout << "mWh";
	if (html)
	  cout << "</td></tr>";
	break;

      default:
	cout << node.getSize();
	if (html)
	  cout << "</td></tr>";
      }
      cout << endl;
    }

    if (node.getCapacity() > 0)
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "capacity: ";
      if (html)
	cout << "</td><td>";
      switch (node.getClass())
      {
      case hw::memory:
      case hw::address:
      case hw::storage:
      case hw::disk:
	kilobytes(node.getCapacity());
	if (html)
	  cout << "</td></tr>";
	break;

      case hw::processor:
      case hw::bus:
      case hw::system:
	decimalkilos(node.getCapacity());
	cout << "Hz";
	if (html)
	  cout << "</td></tr>";
	break;

      case hw::network:
	decimalkilos(node.getCapacity());
	cout << "B/s";
	if (html)
	  cout << "</td></tr>";
	break;

      case hw::power:
	cout << node.getCapacity();
	cout << "mWh";
	if (html)
	  cout << "</td></tr>";
	break;

      default:
	cout << node.getCapacity();
	if (html)
	  cout << "</td></tr>";
      }
      cout << endl;
    }

    if (node.getClass() == hw::address)
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      if (node.getSize() == 0)
	cout << "address: " << hex << setfill('0') << setw(8) << node.
	  getStart() << dec;
      else
	cout << "range: " << hex << setfill('0') << setw(8) << node.
	  getStart() << " - " << hex << setfill('0') << setw(8) << node.
	  getStart() + node.getSize() - 1 << dec;
      cout << endl;
    }

    if (node.getWidth() > 0)
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "width: ";
      if (html)
	cout << "</td><td>";
      cout << node.getWidth() << " bits";
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getClock() > 0)
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "clock: ";
      if (html)
	cout << "</td><td>";
      decimalkilos(node.getClock());
      cout << "Hz";
      if (node.getClass() == hw::memory)
	cout << " (" << 1.0e9 / node.getClock() << "ns)";
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (node.getCapabilities() != "")
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "capabilities: ";
      if (html)
	cout << "</td><td>";
      cout << node.getCapabilities();
      if (html)
	cout << "</td></tr>";
      cout << endl;
    }

    if (config.size() > 0)
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "configuration:";
      if (html)
	cout << "</td><td><table summary=\"configuration of " << node.
	  getId() << "\">";
      for (unsigned int i = 0; i < config.size(); i++)
      {
	if (html)
	  cout << "<tr><td>";
	cout << " " << config[i];
	if (html)
	  cout << "</td></tr>";
      }
      if (html)
	cout << "</table></td></tr>";
      cout << endl;
    }

    if (resources.size() > 0)
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "resources:";
      if (html)
	cout << "</td><td><table summary=\"resources of " << node.
	  getId() << "\">";
      for (unsigned int i = 0; i < resources.size(); i++)
      {
	if (html)
	  cout << "<tr><td>";
	cout << " " << resources[i];
	if (html)
	  cout << "</td></tr>";
      }
      if (html)
	cout << "</table></td></tr>";
      cout << endl;
    }

    if (html)
    {
      tab(level, false);
      cout << "</table>" << endl;
    }

  }				// if visible

  for (unsigned int i = 0; i < node.countChildren(); i++)
  {
    print(*node.getChild(i), html, visible(node.getClassName()) ? level + 1 : 1);
  }

  if (html)
  {
    if (visible(node.getClassName()))
    {
      tab(level, false);
      cout << "</div>" << endl;
    }
    if (level == 0)
    {
      cout << "</body>" << endl;
      cout << "</html>" << endl;
    }
  }

  (void) &id;			// avoid "id defined but not used" warning
}

struct hwpath
{
  string path;
  string description;
  string devname;
  string classname;
};

static void printhwnode(hwNode & node,
			vector < hwpath > &l,
			string prefix = "")
{
  hwpath entry;

  entry.path = "";
  if (node.getPhysId() != "")
    entry.path = prefix + "/" + node.getPhysId();
  entry.description = node.getProduct();
  if (entry.description == "")
    entry.description = node.getDescription();
  entry.devname = node.getLogicalName();
  entry.classname = node.getClassName();

  l.push_back(entry);
  for (unsigned int i = 0; i < node.countChildren(); i++)
  {
    printhwnode(*node.getChild(i), l, entry.path);
  }
}

static void printbusinfo(hwNode & node,
			vector < hwpath > &l)
{
  hwpath entry;

  entry.path = "";
  if (node.getBusInfo() != "")
    entry.path = node.getBusInfo();
  entry.description = node.getProduct();
  if (entry.description == "")
    entry.description = node.getDescription();
  entry.devname = node.getLogicalName();
  entry.classname = node.getClassName();

  l.push_back(entry);
  for (unsigned int i = 0; i < node.countChildren(); i++)
  {
    printbusinfo(*node.getChild(i), l);
  }
}

static void printincolumns( vector < hwpath > &l, const char *cols[])
{
  unsigned int l1 = strlen(cols[0]),
    l2 = strlen(cols[1]),
    l3 = strlen(cols[2]);
  unsigned int i = 0;

  for (i = 0; i < l.size(); i++)
  {
    if (l1 < l[i].path.length())
      l1 = l[i].path.length();
    if (l2 < l[i].devname.length())
      l2 = l[i].devname.length();
    if (l3 < l[i].classname.length())
      l3 = l[i].classname.length();
  }

  cout << cols[0];
  cout << spaces(2 + l1 - strlen(cols[0]));
  cout << cols[1];
  cout << spaces(2 + l2 - strlen(cols[1]));
  cout << cols[2];
  cout << spaces(2 + l3 - strlen(cols[2]));
  cout << cols[3];
  cout << endl;

  cout << spaces(l1 + l2 + l3 + strlen(cols[3]) + 6, "=");
  cout << endl;

  for (i = 0; i < l.size(); i++)
    if (visible(l[i].classname.c_str()))
    {
      cout << l[i].path;
      cout << spaces(2 + l1 - l[i].path.length());
      cout << l[i].devname;
      cout << spaces(2 + l2 - l[i].devname.length());
      cout << l[i].classname;
      cout << spaces(2 + l3 - l[i].classname.length());
      cout << l[i].description << endl;
    }
}

static const char *hwpathcols[] = { "H/W path",
  "Device",
  "Class",
  "Description"
};

void printhwpath(hwNode & node)
{
  vector < hwpath > l;
  printhwnode(node, l);
  printincolumns(l, hwpathcols);
}

static const char *businfocols[] = { "Bus info",
  "Device",
  "Class",
  "Description"
};

void printbusinfo(hwNode & node)
{
  vector < hwpath > l;
  printbusinfo(node, l);
  printincolumns(l, businfocols);
}
