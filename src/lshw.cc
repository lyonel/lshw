#include "hw.h"
#include "print.h"
#include "main.h"
#include "version.h"
#include "options.h"
#include "osutils.h"
#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#ifndef NONLS
#include <locale.h>
#endif

__ID("@(#) $Id$");

void usage(const char *progname)
{
  fprintf(stderr, "Hardware Lister (lshw) - %s\n", getpackageversion());
  fprintf(stderr, _("usage: %s [-format] [-options ...]\n"), progname);
  fprintf(stderr, _("       %s -version\n"), progname);
  fprintf(stderr, "\n");
  fprintf(stderr, _("\t-version        print program version (%s)\n"), getpackageversion());
  fprintf(stderr, _("\nformat can be\n"));
  fprintf(stderr, _("\t-html           output hardware tree as HTML\n"));
  fprintf(stderr, _("\t-xml            output hardware tree as XML\n"));
  fprintf(stderr, _("\t-short          output hardware paths\n"));
  fprintf(stderr, _("\t-businfo        output bus information\n"));
  if(getenv("DISPLAY") && exists(SBINDIR"/gtk-lshw"))
    fprintf(stderr, _("\t-X              use graphical interface\n"));
  fprintf(stderr, _("\noptions can be\n"));
#ifdef SQLITE
  fprintf(stderr, _("\t-dump OUTFILE   save hardware tree to a file\n"));
#endif
  fprintf(stderr, _("\t-class CLASS    only show a certain class of hardware\n"));
  fprintf(stderr, _("\t-C CLASS        same as '-class CLASS'\n"));
  fprintf(stderr, _("\t-c CLASS        same as '-class CLASS'\n"));
  fprintf(stderr,
    _("\t-disable TEST   disable a test (like pci, isapnp, cpuid, etc. )\n"));
  fprintf(stderr,
    _("\t-enable TEST    enable a test (like pci, isapnp, cpuid, etc. )\n"));
  fprintf(stderr, _("\t-quiet          don't display status\n"));
  fprintf(stderr, _("\t-sanitize       sanitize output (remove sensitive information like serial numbers, etc.)\n"));
  fprintf(stderr, _("\t-numeric        output numeric IDs (for PCI, USB, etc.)\n"));
  fprintf(stderr, _("\t-notime         exclude volatile attributes (timestamps) from output\n"));
  fprintf(stderr, "\n");
}


void status(const char *message)
{
  static size_t lastlen = 0;

  if(enabled("output:quiet") || disabled("output:verbose"))
    return;

  if (isatty(2))
  {
    fprintf(stderr, "\r");
    for (size_t i = 0; i < lastlen; i++)
      fprintf(stderr, " ");
    fprintf(stderr, "\r%s\r", message);
    fflush(stderr);
    lastlen = strlen(message);
  }
}


int main(int argc,
char **argv)
{

#ifndef NONLS
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
#endif

  disable("isapnp");

  disable("output:json");
  disable("output:db");
  disable("output:xml");
  disable("output:html");
  disable("output:hwpath");
  disable("output:businfo");
  disable("output:X");
  disable("output:quiet");
  disable("output:sanitize");
  disable("output:numeric");
  enable("output:time");

// define some aliases for nodes classes
  alias("disc", "disk");
  alias("cpu", "processor");
  alias("lan", "network");
  alias("net", "network");
  alias("video", "display");
  alias("sound", "multimedia");
  alias("modem", "communication");

  if (!parse_options(argc, argv))
  {
    usage(argv[0]);
    exit(1);
  }

  while (argc >= 2)
  {
    bool validoption = false;

    if (strcmp(argv[1], "-version") == 0)
    {
      const char *latest = checkupdates();
      printf("%s\n", getpackageversion());
      if(latest)
      {
        if(strcmp(latest, getpackageversion()) != 0)
          fprintf(stderr, _("the latest version is %s\n"), latest);
      }
      exit(0);
    }

    if (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0)
    {
      usage(argv[0]);
      exit(0);
    }

    if (strcmp(argv[1], "-verbose") == 0)
    {
      disable("output:quiet");
      enable("output:verbose");
      validoption = true;
    }

    if (strcmp(argv[1], "-quiet") == 0)
    {
      disable("output:verbose");
      enable("output:quiet");
      validoption = true;
    }

    if (strcmp(argv[1], "-json") == 0)
    {
      enable("output:json");
      validoption = true;
    }

    if (strcmp(argv[1], "-xml") == 0)
    {
      enable("output:xml");
      validoption = true;
    }

    if (strcmp(argv[1], "-html") == 0)
    {
      enable("output:html");
      validoption = true;
    }

    if (strcmp(argv[1], "-short") == 0)
    {
      enable("output:hwpath");
      validoption = true;
    }

    if (strcmp(argv[1], "-businfo") == 0)
    {
      enable("output:businfo");
      validoption = true;
    }

    if (strcmp(argv[1], "-X") == 0)
    {
      enable("output:X");
      validoption = true;
    }

    if ((strcmp(argv[1], "-sanitize") == 0) ||
       (strcmp(argv[1], "-sanitise") == 0))
    {
      enable("output:sanitize");
      validoption = true;
    }

    if (strcmp(argv[1], "-numeric") == 0)
    {
      enable("output:numeric");
      validoption = true;
    }

    if (strcmp(argv[1], "-notime") == 0)
    {
        disable("output:time");
        validoption = true;
    }

    if(validoption)
    {	/* shift */
      memmove(argv+1, argv+2, (argc-1)*(sizeof(argv[0])));
      argc--;
    }
    else
    {
      usage(argv[0]);
      exit(1);
    }
  }

  if (argc >= 2)
  {
    usage(argv[0]);
    exit(1);
  }

  if(enabled("output:X")) execl(SBINDIR"/gtk-lshw", SBINDIR"/gtk-lshw", NULL);

  if (geteuid() != 0)
  {
    fprintf(stderr, _("WARNING: you should run this program as super-user.\n"));
  }

  {
    hwNode computer("computer",
      hw::system);

    scan_system(computer);

    if (enabled("output:hwpath"))
      printhwpath(computer);
    else
    if (enabled("output:businfo"))
      printbusinfo(computer);
    else
    {
      if (enabled("output:json"))
        cout << computer.asJSON() << endl;
      else
      if (enabled("output:xml"))
        cout << computer.asXML();
      else
        print(computer, enabled("output:html"));
    }

    if(enabled("output:db"))
      computer.dump(getenv("OUTFILE"));
  }

  if (geteuid() != 0)
  {
    fprintf(stderr, _("WARNING: output may be incomplete or inaccurate, you should run this program as super-user.\n"));
  }


  return 0;
}
