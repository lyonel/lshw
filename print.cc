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

void print(const hwNode & node,
	   int level)
{
  tab(level);
  cout << node.getId() << endl;
  if (node.getSize() > 0)
  {
    tab(level + 1, false);
    cout << "size: ";
    switch (node.getClass())
    {
    case hw::memory:
    case hw::storage:
      cout << (node.getSize() >> 20) << "MB";
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
