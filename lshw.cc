#include "hw.h"
#include "print.h"
#include "main.h"
#include "version.h"
#include "options.h"

#include <unistd.h>
#include <stdio.h>

static char *id = "@(#) $Id: lshw.cc,v 1.2 2003/11/07 16:51:06 ezix Exp $";

void usage(const char *progname)
{
  fprintf(stderr, "Hardware Lister (lshw) - %s\n", getpackageversion());
  fprintf(stderr, "usage: %s [-options ...]\n", progname);
  fprintf(stderr, "\t-version        print program version\n");
  fprintf(stderr, "\t-html           output hardware tree as HTML\n");
  fprintf(stderr, "\t-xml            output hardware tree as XML\n");
  fprintf(stderr, "\t-short          output hardware paths\n");
  fprintf(stderr,
	  "\t-disable test   disable a test (like pci, isapnp, cpuid, etc. )\n");
  fprintf(stderr, "\n");
}

int main(int argc,
	 char **argv)
{
  bool htmloutput = false;
  bool xmloutput = false;
  bool hwpath = false;

  if (!parse_options(argc, argv))
  {
    usage(argv[0]);
    exit(1);
  }

  if (argc == 2)
  {
    if (strcmp(argv[1], "-version") == 0)
    {
      printf("%s\n", getpackageversion());
      exit(0);
    }
    if (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0)
    {
      usage(argv[0]);
      exit(0);
    }
    if (strcmp(argv[1], "-xml") == 0)
      xmloutput = true;

    if (strcmp(argv[1], "-html") == 0)
      htmloutput = true;

    if (strcmp(argv[1], "-short") == 0)
      hwpath = true;

    if (!xmloutput && !htmloutput && !hwpath)
    {
      usage(argv[0]);
      exit(1);
    }
  }

  if (geteuid() != 0)
  {
    fprintf(stderr, "WARNING: you should run this program as super-user.\n");
  }

  disable("isapnp");

  {
    hwNode computer("computer",
		    hw::system);

    scan_system(computer);

    if (hwpath)
      printhwpath(computer);
    else
    {
      if (xmloutput)
	printxml(computer);
      else
	print(computer, htmloutput);
    }
  }

  (void) &id;			// avoid warning "id defined but not used"

  return 0;
}
