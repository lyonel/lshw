/*
 * main.cc
 *
 * this module is shared between the command-line and graphical interfaces of
 * lshw (currently, only the text interface is available).
 *
 * It calls all the defined scans in a certain order that tries to ensure
 * that devices are only reported once and that information coming from
 * different sources about a given device is kept consistent.
 *
 * Individual tests can be disabled on the command-line by using the -disable
 * option.
 * Status is reported during the execution of tests.
 *
 */

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
#include "fb.h"
#include "usb.h"
#include "sysfs.h"
#include "display.h"

#include <unistd.h>
#include <stdio.h>

static char *id = "@(#) $Id$";

bool scan_system(hwNode & system)
{
  char hostname[80];

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
    status("kernel device tree (sysfs)");
    if (enabled("sysfs"))
      scan_sysfs(computer);
    status("USB");
    if (enabled("usb"))
      scan_usb(computer);
    status("IDE");
    if (enabled("ide"))
      scan_ide(computer);
    status("SCSI");
    if (enabled("scsi"))
      scan_scsi(computer);
    status("Network interfaces");
    if (enabled("network"))
      scan_network(computer);
    status("Framebuffer devices");
    if (enabled("fb"))
      scan_fb(computer);
    status("Display");
    if (enabled("display"))
      scan_display(computer);
    status("");

    if (computer.getDescription() == "")
      computer.setDescription("Computer");
    computer.assignPhysIds();
    computer.fixInconsistencies();

    system = computer;
  }
  else
    return false;

  (void) &id;			// avoid warning "id defined but not used"

  return true;
}
