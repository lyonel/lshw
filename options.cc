/*
 * options.cc
 *
 * This module handles global options passed on the command-line.
 *
 */

#include "options.h"
#include "osutils.h"

#include <set>
#include <vector>
#include <string>

using namespace std;

static char *id = "@(#) $Id$";

static set < string > disabled_tests;
static set < string > visible_classes;

static void remove_option_argument(int i,
				   int &argc,
				   char *argv[])
{
  for (int j = i; j + 2 < argc; j++)
    argv[j] = argv[j + 2];

  argc -= 2;
}

bool parse_options(int &argc,
		   char *argv[])
{
  int i = 1;

  while (i < argc)
  {
    if (strcmp(argv[i], "-disable") == 0)
    {
      if (i + 1 >= argc)
	return false;		// -disable requires an argument

      disable(argv[i + 1]);

      remove_option_argument(i, argc, argv);
    }
    else if (strcmp(argv[i], "-enable") == 0)
    {
      if (i + 1 >= argc)
	return false;		// -enable requires an argument

      enable(argv[i + 1]);

      remove_option_argument(i, argc, argv);
    }
    else if ((strcmp(argv[i], "-class") == 0) || (strcmp(argv[i], "-C") == 0))
    {
      vector < string > classes;

      if (i + 1 >= argc)
	return false;		// -class requires an argument

      splitlines(argv[i + 1], classes, ',');

      for (unsigned int j = 0; j < classes.size(); j++)
	visible_classes.insert(classes[j]);

      remove_option_argument(i, argc, argv);
    }
    else
      i++;
  }

  return true;
}

bool enabled(const char *option)
{
  return !(disabled(option));
}

bool disabled(const char *option)
{
  return disabled_tests.find(string(option)) != disabled_tests.end();
}

void enable(const char *option)
{
  if (!disabled(option))
    return;

  disabled_tests.erase(string(option));
}

void disable(const char *option)
{
  disabled_tests.insert(string(option));

  (void) &id;			// avoid warning "id defined but not used"
}

bool visible(const char *c)
{
  if (visible_classes.size() == 0)
    return true;
  return visible_classes.find(string(c)) != visible_classes.end();
}
