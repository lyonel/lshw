#include "print.h"
#include <iostream>

static void tab(int level,
		bool connect = true)
{
  if (level <= 0)
    return;
  for (int i = 0; i < level - 1; i++)
    cout << "  |";
  cout << "  ";
  if (connect)
    cout << "+-";
  else
    cout << "  ";
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
  cout << node.getId() << endl;

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

    default:
      cout << node.getSize();
    }
    cout << endl;
  }

  for (int i = 0; i < node.countChildren(); i++)
  {
    print(*node.getChild(i), level + 1);
  }
}
