#include "hw.h"
#include "print.h"

#include "version.h"
#include "mem.h"
#include "dmi.h"
#include "cpuinfo.h"
#include "cpuid.h"
#include "device-tree.h"
#include "pci.h"
#include "pcmcia.h"
#include "ide.h"
#include "scsi.h"
#include "spd.h"

#include <unistd.h>
#include <stdio.h>

void usage(const char *progname)
{
  fprintf(stderr, "Harware Lister (lshw) - %s\n", getpackageversion());
  fprintf(stderr, "usage: %s [-options ...]\n", progname);
  fprintf(stderr, "\t-version      print program version\n");
  fprintf(stderr, "\t-html         output hardware tree as HTML\n");
  fprintf(stderr, "\t-xml          output hardware tree as XML\n");
  fprintf(stderr, "\n");
}

int main(int argc,
	 char **argv)
{
  char hostname[80];
  bool htmloutput = false;
  bool xmloutput = false;

  if (argc > 2)
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

    if (!xmloutput && !htmloutput)
    {
      usage(argv[0]);
      exit(1);
    }
  }

  if (gethostname(hostname, sizeof(hostname)) == 0)
  {
    hwNode computer(hostname,
		    hw::system);

    scan_dmi(computer);
    scan_device_tree(computer);
    scan_spd(computer);
    scan_memory(computer);
    scan_cpuinfo(computer);
    scan_cpuid(computer);
    scan_pci(computer);
    scan_pcmcia(computer);
    scan_ide(computer);
    scan_scsi(computer);

    if (xmloutput)
      printxml(computer);
    else
      print(computer, htmloutput);
  }

  return 0;
}

static char *id = "@(#) $Id: main.cc,v 1.23 2003/04/16 16:24:09 ezix Exp $";
