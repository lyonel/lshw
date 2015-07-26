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
#include "pcmcia-legacy.h"
#include "ide.h"
#include "scsi.h"
#include "spd.h"
#include "network.h"
#include "isapnp.h"
#include "fb.h"
#include "usb.h"
#include "sysfs.h"
#include "display.h"
#include "parisc.h"
#include "cpufreq.h"
#include "ideraid.h"
#include "mounts.h"
#include "virtio.h"
#include "smp.h"
#include "abi.h"
#include "dasd.h"

#include <unistd.h>
#include <stdio.h>

__ID("@(#) $Id$");

bool scan_system(hwNode & system)
{
  char hostname[80];

  if (gethostname(hostname, sizeof(hostname)) == 0)
  {
    hwNode computer(::enabled("output:sanitize")?"computer":hostname,
      hw::system);

    status("DMI");
    if (enabled("dmi"))
      scan_dmi(computer);
    status("SMP");
    if (enabled("smp"))
      scan_smp(computer);
    status("PA-RISC");
    if (enabled("parisc"))
      scan_parisc(computer);
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
    status("PCI (sysfs)");
    if (enabled("pci"))
    {
      if(!scan_pci(computer))
      {
        if (enabled("pcilegacy"))
          scan_pci_legacy(computer);
      }
    }
    else
    {
      status("PCI (legacy)");
      if (enabled("pcilegacy"))
        scan_pci_legacy(computer);
    }
    status("ISA PnP");
    if (enabled("isapnp"))
      scan_isapnp(computer);
    status("PCMCIA");
    if (enabled("pcmcia"))
      scan_pcmcia(computer);
    status("PCMCIA");
    if (enabled("pcmcia-legacy"))
      scan_pcmcialegacy(computer);
    status("Virtual I/O (VIRTIO) devices");
    if (enabled("virtio"))
      scan_virtio(computer);
    status("kernel device tree (sysfs)");
    if (enabled("sysfs"))
      scan_sysfs(computer);
    status("USB");
    if (enabled("usb"))
      scan_usb(computer);
    status("IDE");
    if (enabled("ide"))
      scan_ide(computer);
    if (enabled("ideraid"))
      scan_ideraid(computer);
    status("SCSI");
    if (enabled("scsi"))
      scan_scsi(computer);
    if (enabled("dasd"))
      scan_dasd(computer);
    if (enabled("mounts"))
      scan_mounts(computer);
    status("Network interfaces");
    if (enabled("network"))
      scan_network(computer);
    status("Framebuffer devices");
    if (enabled("fb"))
      scan_fb(computer);
    status("Display");
    if (enabled("display"))
      scan_display(computer);
    status("CPUFreq");
    if (enabled("cpufreq"))
      scan_cpufreq(computer);
    status("ABI");
    if (enabled("abi"))
      scan_abi(computer);
    status("");

    if (computer.getDescription() == "")
      computer.setDescription("Computer");
    computer.assignPhysIds();
    computer.fixInconsistencies();

    system = computer;
  }
  else
    return false;

  return true;
}
