#include "print.h"
#include "version.h"
#include "osutils.h"
#include <iostream>
#include <iomanip>

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
  const char *prefixes = "KMGTPH";
  int i = 0;

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
  const char *prefixes = "KMGTPH";
  int i = 0;

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
  if (html)
    config = node.getConfigValues("</td><td>=</td><td>");
  else
    config = node.getConfigValues("=");

  if (html && (level == 0))
  {
    cout <<
      "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" <<
      endl;
    cout << "<html>" << endl;
    cout << "<head>" << endl;
    cout << "<meta name=\"generator\" " <<
      " content=\"lshw " << getpackageversion() << "\">" << endl;
    cout << "<title>";
    cout << node.getId();
    cout << "</title>" << endl;
    cout << "</head>" << endl;
    cout << "<body>" << endl;
  }

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

  if (node.getLogicalName() != "")
  {
    tab(level + 1, false);
    if (html)
      cout << "<tr><td>";
    cout << "logical name: ";
    if (html)
      cout << "</td><td>";
    cout << node.getLogicalName();
    if (html)
      cout << "</td></tr>";
    cout << endl;
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
    for (int i = 0; i < config.size(); i++)
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

  if (html)
  {
    tab(level, false);
    cout << "</table>" << endl;
  }
  for (int i = 0; i < node.countChildren(); i++)
  {
    print(*node.getChild(i), html, level + 1);
  }
  if (html)
  {
    tab(level, false);
    cout << "</div>" << endl;
    if (level == 0)
    {
      cout << "</body>" << endl;
      cout << "</html>" << endl;
    }
  }
}

static string escape(const string & s)
{
  string result = "";

  for (int i = 0; i < s.length(); i++)
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

void printxml(hwNode & node,
	      int level)
{
  vector < string > config;

  config = node.getConfigValues("\" value=\"");

  if (level == 0)
    cout << "<?xml version=\"1.0\"?>" << endl;

  tab(level, false);
  cout << "<node id=\"" << node.getId() << "\"";
  if (node.disabled())
    cout << " disabled=\"true\"";
  if (node.claimed())
    cout << " claimed=\"true\"";

  cout << " class=\"" << node.getClassName() << "\"";
  cout << ">" << endl;

  if (node.getDescription() != "")
  {
    tab(level + 1, false);
    cout << "<description>";
    cout << escape(node.getDescription());
    cout << "</description>";
    cout << endl;
  }

  if (node.getProduct() != "")
  {
    tab(level + 1, false);
    cout << "<product>";
    cout << escape(node.getProduct());
    cout << "</product>";
    cout << endl;
  }

  if (node.getVendor() != "")
  {
    tab(level + 1, false);
    cout << "<vendor>";
    cout << escape(node.getVendor());
    cout << "</vendor>";
    cout << endl;
  }

  if (node.getLogicalName() != "")
  {
    tab(level + 1, false);
    cout << "<logicalname>";
    cout << escape(node.getLogicalName());
    cout << "</logicalname>";
    cout << endl;
  }

  if (node.getVersion() != "")
  {
    tab(level + 1, false);
    cout << "<version>";
    cout << escape(node.getVersion());
    cout << "</version>";
    cout << endl;
  }

  if (node.getSerial() != "")
  {
    tab(level + 1, false);
    cout << "<serial>";
    cout << escape(node.getSerial());
    cout << "</serial>";
    cout << endl;
  }

  if (node.getSlot() != "")
  {
    tab(level + 1, false);
    cout << "<slot>";
    cout << escape(node.getSlot());
    cout << "</slot>";
    cout << endl;
  }

  if (node.getSize() > 0)
  {
    tab(level + 1, false);
    cout << "<size";
    switch (node.getClass())
    {
    case hw::memory:
    case hw::address:
    case hw::storage:
    case hw::display:
      cout << " units=\"bytes\"";
      break;

    case hw::processor:
    case hw::bus:
    case hw::system:
      cout << " units=\"Hz\"";
      break;
    }
    cout << ">";
    cout << node.getSize();
    cout << "</size>";
    cout << endl;
  }

  if (node.getCapacity() > 0)
  {
    tab(level + 1, false);
    cout << "<capacity";
    switch (node.getClass())
    {
    case hw::memory:
    case hw::address:
    case hw::storage:
      cout << " units=\"bytes\"";
      break;

    case hw::processor:
    case hw::bus:
    case hw::system:
      cout << " units=\"Hz\"";
      break;
    }
    cout << ">";
    cout << node.getCapacity();
    cout << "</capacity>";
    cout << endl;
  }

  if (node.getClock() > 0)
  {
    tab(level + 1, false);
    cout << "<clock units=\"Hz\">";
    cout << node.getClock();
    cout << "</clock>";
    cout << endl;
  }

  if (config.size() > 0)
  {
    tab(level + 1, false);
    cout << "<configuration>" << endl;
    for (int j = 0; j < config.size(); j++)
    {
      tab(level + 2, false);
      cout << "<setting id=\"" << escape(config[j]) << "\" />";
      cout << endl;
    }
    tab(level + 1, false);
    cout << "</configuration>" << endl;
  }
  config.clear();

  splitlines(node.getCapabilities(), config, ' ');
  if (config.size() > 0)
  {
    tab(level + 1, false);
    cout << "<capabilities>" << endl;
    for (int j = 0; j < config.size(); j++)
    {
      tab(level + 2, false);
      cout << "<capability id=\"" << escape(config[j]) << "\" />";
      cout << endl;
    }
    tab(level + 1, false);
    cout << "</capabilities>" << endl;
  }
  config.clear();

  for (int i = 0; i < node.countChildren(); i++)
  {
    printxml(*node.getChild(i), level + 1);
  }

  tab(level, false);
  cout << "</node>" << endl;
}

static char *id = "@(#) $Id: print.cc,v 1.37 2003/03/11 08:45:11 ezix Exp $";
