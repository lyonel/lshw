#include "hw.h"
#include "print.h"

#include "version.h"
#include "options.h"
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

static char *id = "@(#) $Id: main.cc,v 1.32 2003/09/29 17:07:52 ezix Exp $";

void usage(const char *progname)
{
  fprintf(stderr, "Harware Lister (lshw) - %s\n", getpackageversion());
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
  char hostname[80];
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

  if (gethostname(hostname, sizeof(hostname)) == 0)
  {
    hwNode computer(hostname,
		    hw::system);

    status("DMI");
    if (enabled("dmi"))
      scan_dmi(computer);
    status("device-tree");
    if (enabled("device-tree"))
      scan_device_tree(computer);
    status("SPD");
    if (enabled("spd"))
      scan_spd(computer);
    status("memory");
    if (enabled("memory"))
      scan_memory(computer);
    status("/proc/cpuinfo");
    if (enabled("cpuinfo"))
      scan_cpuinfo(computer);
    status("CPUID");
    if (enabled("cpuid"))
      scan_cpuid(computer);
    status("PCI");
    if (enabled("pci"))
      scan_pci(computer);
    status("ISA PnP");
    if (enabled("isapnp"))
      scan_isapnp(computer);
    status("PCMCIA");
    if (enabled("pcmcia"))
      scan_pcmcia(computer);
    status("IDE");
    if (enabled("ide"))
      scan_ide(computer);
    status("SCSI");
    if (enabled("scsi"))
      scan_scsi(computer);
    status("Network interfaces");
    if (enabled("network"))
      scan_network(computer);
    status("");

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
