#include "print.h"
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

void print(const hwNode & node,
	   bool html,
	   int level)
{
  vector < string > config;
  if (html)
    config = node.getConfigValues("</td><td>=</td><td>");
  else
    config = node.getConfigValues("=");

  tab(level, !html);

  if (html)
    cout << "<b>";
  cout << node.getId();
  if (node.disabled())
    cout << " DISABLED";
  if (html)
    cout << "</b><br>";
  cout << endl;

  if (html)
  {
    tab(level, false);
    cout << "<ul>" << endl;
    tab(level, false);
    cout << "<table bgcolor=\"#e8e0e0\">" << endl;
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
      cout << "</td><td><table>";
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
    cout << "</ul>" << endl;
  }
}

static char *id = "@(#) $Id: print.cc,v 1.26 2003/01/31 22:32:43 ezix Exp $";
