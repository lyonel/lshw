#include "print.h"
#include <iostream>

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

void decimalkilos(unsigned long long value)
{
  const char *prefixes = "KMGTPH";
  int i = 0;

  while ((i <= strlen(prefixes)) && (value > 10000))
  {
    value = value / 1000;
    i++;
  }

  cout << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    cout << prefixes[i - 1];
}

void kilobytes(unsigned long long value)
{
  const char *prefixes = "KMGTPH";
  int i = 0;

  while ((i <= strlen(prefixes)) && (value > 10240))
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
	   int level)
{
  tab(level);
  cout << node.getId();
  if (node.disabled())
    cout << " DISABLED";
  cout << endl;

  if (node.getDescription() != "")
  {
    tab(level + 1, false);
    cout << "description: " << node.getDescription() << endl;
  }

  if (node.getProduct() != "")
  {
    tab(level + 1, false);
    cout << "product: " << node.getProduct() << endl;
  }

  if (node.getVendor() != "")
  {
    tab(level + 1, false);
    cout << "vendor: " << node.getVendor() << endl;
  }

  if (node.getVersion() != "")
  {
    tab(level + 1, false);
    cout << "version: " << node.getVersion() << endl;
  }

  if (node.getSerial() != "")
  {
    tab(level + 1, false);
    cout << "serial: " << node.getSerial() << endl;
  }

  if (node.getSlot() != "")
  {
    tab(level + 1, false);
    cout << "slot: " << node.getSlot() << endl;
  }

  if (node.getSize() > 0)
  {
    tab(level + 1, false);
    cout << "size: ";
    switch (node.getClass())
    {
    case hw::memory:
    case hw::storage:
      kilobytes(node.getSize());
      break;

    case hw::processor:
    case hw::bus:
    case hw::system:
      decimalkilos(node.getSize());
      cout << "Hz";
      break;

    default:
      cout << node.getSize();
    }
    cout << endl;
  }

  if (node.getCapacity() > 0)
  {
    tab(level + 1, false);
    cout << "capacity: ";
    switch (node.getClass())
    {
    case hw::memory:
    case hw::storage:
      kilobytes(node.getCapacity());
      break;

    case hw::processor:
    case hw::bus:
    case hw::system:
      decimalkilos(node.getCapacity());
      cout << "Hz";
      break;

    default:
      cout << node.getCapacity();
    }
    cout << endl;
  }

  if (node.getClock() > 0)
  {
    tab(level + 1, false);
    cout << "clock: ";
    decimalkilos(node.getClock());
    cout << "Hz";
    if (node.getClass() == hw::memory)
      cout << " (" << 1.0e9 / node.getClock() << "ns)";
    cout << endl;
  }

  for (int i = 0; i < node.countChildren(); i++)
  {
    print(*node.getChild(i), level + 1);
  }
}
