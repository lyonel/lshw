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
#include "network.h"
#include "isapnp.h"

#include <unistd.h>
#include <stdio.h>

static char *id = "@(#) $Id: main.cc,v 1.28 2003/07/15 07:06:34 ezix Exp $";

void usage(const char *progname)
{
  fprintf(stderr, "Harware Lister (lshw) - %s\n", getpackageversion());
  fprintf(stderr, "usage: %s [-options ...]\n", progname);
  fprintf(stderr, "\t-version      print program version\n");
  fprintf(stderr, "\t-html         output hardware tree as HTML\n");
  fprintf(stderr, "\t-xml          output hardware tree as XML\n");
  fprintf(stderr, "\t-short        output hardware paths\n");
  fprintf(stderr, "\n");
}

int main(int argc,
	 char **argv)
{
  char hostname[80];
  bool htmloutput = false;
  bool xmloutput = false;
  bool hwpath = false;

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

    if (strcmp(argv[1], "-short") == 0)
      hwpath = true;

    if (!xmloutput && !htmloutput && !hwpath)
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
    scan_isapnp(computer);
    scan_pcmcia(computer);
    scan_ide(computer);
    scan_scsi(computer);
    scan_network(computer);

    if (computer.getDescription() == "")
      computer.setDescription("Computer");
    computer.assignPhysIds();

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
