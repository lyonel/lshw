/*
 * print.cc
 *
 *
 */

#include "print.h"
#include "options.h"
#include "version.h"
#include "osutils.h"
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <stdio.h>

static char *id = "@(#) $Id: print.cc 764 2004-11-26 01:28:23Z ezix $";

static void decimalkilos(ostringstream & out, unsigned long long value)
{
  const char *prefixes = "KMGTPEZY";
  unsigned int i = 0;

  while ((i <= strlen(prefixes)) && ((value > 10000) || (value % 1000 == 0)))
  {
    value = value / 1000;
    i++;
  }

  out << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    out << prefixes[i - 1];
}

static void kilobytes(ostringstream & out, unsigned long long value)
{
  const char *prefixes = "KMGTPEZY";
  unsigned int i = 0;

  while ((i <= strlen(prefixes)) && ((value > 10240) || (value % 1024 == 0)))
  {
    value = value >> 10;
    i++;
  }

  out << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    out << prefixes[i - 1];
  out << "B";
}

static string printattr(const string & name, const string & value)
{
  if(value == "") return "";

  return "<b>" + name + ":</b> " + value + "\n";
}

string printmarkup(const hwNode & node)
{
  vector < string > config;
  vector < string > resources;
  ostringstream out;

  config = node.getConfigValues("=");
  resources = node.getResources(":");

  out << "<b><i>" + node.getId();
  if(!node.claimed() || node.disabled())
    out << "<span color=\"gray\">";
  if(node.disabled())
    out << " DISABLED";
  if(!node.claimed())
    out << " UNCLAIMED";
  if(!node.claimed() || node.disabled())
    out << "</span>";
  out << "</i></b>" << endl << endl;

  out << printattr("description", node.getDescription());
  out << printattr("product", node.getProduct());
  out << printattr("vendor", node.getVendor());
  out << printattr("bus info", node.getBusInfo());
  if (node.getLogicalName() != "")
  {
    vector<string> logicalnames = node.getLogicalNames();

    for(unsigned int i = 0; i<logicalnames.size(); i++)
    	out << printattr("logical name", logicalnames[i]);
  }

  out << printattr("version", node.getVersion());
  out << printattr("serial", node.getSerial());
  out << printattr("slot", node.getSlot());

  if (node.getSize() > 0)
    {
      out << "<b>size:</b> ";
      switch (node.getClass())
      {
      case hw::display:
      case hw::memory:
      case hw::address:
      case hw::storage:
      case hw::disk:
	kilobytes(out, node.getSize());
	break;

      case hw::processor:
      case hw::bus:
      case hw::system:
	decimalkilos(out, node.getSize());
	out << "Hz";
	break;

      case hw::network:
	decimalkilos(out, node.getSize());
	out << "bps";
	break;

      default:
	out << node.getSize();
      }
      out << endl;
    }

    if (node.getCapacity() > 0)
    {
      out << "<b>capacity:</b> ";
      switch (node.getClass())
      {
      case hw::memory:
      case hw::address:
      case hw::storage:
      case hw::disk:
	kilobytes(out, node.getCapacity());
	break;

      case hw::processor:
      case hw::bus:
      case hw::system:
	decimalkilos(out, node.getCapacity());
	out << "Hz";
	break;

      default:
	out << node.getCapacity();
      }
      out << endl;
    }

    if (node.getClass() == hw::address)
    {
      if (node.getSize() == 0)
	out << "<b>address:</b> " << hex << setfill('0') << setw(8) << node.
	  getStart() << dec;
      else
	out << "<b>range:</b> " << hex << setfill('0') << setw(8) << node.
	  getStart() << " - " << hex << setfill('0') << setw(8) << node.
	  getStart() + node.getSize() - 1 << dec;
      out << endl;
    }

    if (node.getClock() > 0)
    {
      out << "<b>clock:</b> ";
      decimalkilos(out, node.getClock());
      out << "Hz";
      if (node.getClass() == hw::memory)
	out << " (" << 1.0e9 / node.getClock() << "ns)";
      out << endl;
    }

  out << printattr("capabilities", node.getCapabilities());

#if 0
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

  }				// if visible

#endif
  (void) &id;			// avoid "id defined but not used" warning

  return out.str();
}

static string escape(const string & s)
{
  string result = "";

  for (unsigned int i = 0; i < s.length(); i++)
    switch (s[i])
    {
    case '<':
      result += "&lt;";
      break;
    case '>':
      result += "&gt;";
      break;
    case '&':
      result += "&ampersand;";
      break;
    default:
      result += s[i];
    }

  return result;
}
