/*
 * dmi.cc
 *
 * This file is based on Alan Cox's excellent DMI decoder rev 1.7
 * it has endured severe modifications so don't blame Alan for the
 * bugs, they're probably mine.
 *
 * This scan searches the BIOS memory for SMBIOS (DMI) extensions and reports
 * DMI information like CPU type, memory banks and size, serial numbers, BIOS
 * capabilities, etc.
 * As the BIOS is supposed to be the most authoritative source of information,
 * this scan should be executed first.
 *
 * Original credits for dmidecode.c:
 *
 * DMI decode rev 1.7
 *
 *	(C) 2000-2002 Alan Cox <alan@redhat.com>
 *
 *      2-July-2001 Matt Domsch <Matt_Domsch@dell.com>
 *      Additional structures displayed per SMBIOS 2.3.1 spec
 *
 *      13-December-2001 Arjan van de Ven <arjanv@redhat.com>
 *      Fix memory bank type (DMI case 6)
 *
 *      3-August-2002 Mark D. Studebaker <mds@paradyne.com>
 *      Better indent in dump_raw_data
 *      Fix return value in dmi_bus_name
 *      Additional sensor fields decoded
 *      Fix compilation warnings
 *
 *      6-August-2002 Jean Delvare <khali@linux-fr.org>
 *      Reposition file pointer after DMI table display
 *      Disable first RSD PTR checksum (was not correct anyway)
 *      Show actual DMI struct count and occupied size
 *      Check for NULL after malloc
 *      Use SEEK_* constants instead of numeric values
 *      Code optimization (and warning fix) in DMI cases 10 and 14
 *      Add else's to avoid unneeded cascaded if's in main loop
 *      Code optimization in DMI information display
 *      Fix all compilation warnings
 *
 *      9-August-2002 Jean Delvare <khali@linux-fr.org>
 *      Better DMI struct count/size error display
 *      More careful memory access in dmi_table
 *      DMI case 13 (Language) decoded
 *      C++ style comments removed
 *      Commented out code removed
 *      DMI 0.0 case handled
 *      Fix return value of dmi_port_type and dmi_port_connector_type
 *
 *      23-August-2002 Alan Cox <alan@redhat.com>
 *      Make the code pass -Wall -pedantic by fixing a few harmless sign of
 *        pointer mismatches
 *      Correct main prototype
 *      Check for compiles with wrong type sizes
 *
 *      17-Sep-2002 Larry Lile <llile@dreamworks.com>
 *      Type 16 & 17 structures displayed per SMBIOS 2.3.1 spec
 *
 *      20-September-2002 Dave Johnson <ddj@cascv.brown.edu>
 *      Fix comparisons in dmi_bus_name
 *      Fix comparison in dmi_processor_type
 *      Fix bitmasking in dmi_onboard_type
 *      Fix return value of dmi_temp_loc
 *
 *      28-September-2002 Jean Delvare <khali@linux-fr.org>
 *      Fix missing coma in dmi_bus_name
 *      Remove unwanted bitmaskings in dmi_mgmt_dev_type, dmi_mgmt_addr_type,
 *        dmi_fan_type, dmi_volt_loc, dmi_temp_loc and dmi_status
 *      Fix DMI table read bug ("dmi: read: Success")
 *      Make the code pass -W again
 *      Fix return value of dmi_card_size
 *
 */

#include "version.h"
#include "config.h"
#include "options.h"
#include "dmi.h"
#include "osutils.h"

#include <map>
#include <vector>
#include <fstream>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

__ID("@(#) $Id$");

#define SYSFSDMI "/sys/firmware/dmi/tables"

static int currentcpu = 0;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct dmi_header
{
  u8 type;
  u8 length;
  u16 handle;
};

static const char *dmi_hardware_security_status(u8 code)
{
  static const char *status[]=
  {
    "disabled",                                   /* 0x00 */
    "enabled",
    "",                                           // not implemented
    "unknown"                                     /* 0x03 */
  };

  return status[code & 0x03];
}

static int checksum(const u8 *buf, size_t len)
{
  u8 sum = 0;
  size_t a;

  for (a = 0; a < len; a++)
    sum += buf[a];
  return (sum == 0);
}

static string dmi_battery_voltage(u16 code)
{
  char buffer[20];
  if(code==0) return "";

  snprintf(buffer, sizeof(buffer), "%.1fV", ((float)code/1000.0));
  return buffer;
}


static unsigned long long dmi_battery_capacity(u16 code, u8 multiplier)
{
  return (unsigned long long)code * (unsigned long long)multiplier;
}


static const char *dmi_battery_chemistry(u8 code)
{
  static const char *chemistry[]=
  {
    N_("Other"),                                      /* 0x01 */
    N_("Unknown"),
    N_("Lead Acid"),
    N_("Nickel Cadmium"),
    N_("Nickel Metal Hydride"),
    N_("Lithium Ion"),
    N_("Zinc Air"),
    N_("Lithium Polymer")                             /* 0x08 */
  };

  if(code>=0x01 && code<=0x08)
    return _(chemistry[code-0x01]);
  return "";
}


static string cpubusinfo(int cpu)
{
  char buffer[20];

  snprintf(buffer, sizeof(buffer), "cpu@%d", cpu);

  return string(buffer);
}


static string dmi_uuid(const u8 * p)
{
  unsigned int i = 0;
  bool valid = false;
  char buffer[60];

  for (i = 0; i < 16; i++)
    if (p[i] != p[0])
      valid = true;

  if (!valid)
    return "";

  if(::enabled("output:sanitize"))
    return string(REMOVED);

  snprintf(buffer, sizeof(buffer),
    "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10],
    p[11], p[12], p[13], p[14], p[15]);

  return hw::strip(string(buffer));
}


static string dmi_bootinfo(u8 code)
{
  static const char *status[] =
  {
    "normal",                                     /* 0 */
    "no-bootable-media",
    "os-error",
    "hardware-failure-fw",
    "hardware-failure-os",
    "user-requested",
    "security-violation",
    "image",
    "watchdog"                                    /* 8 */
  };
  if (code <= 8)
    return string(status[code]);
  else
    return "oem-specific";
}


static string dmi_string(struct dmi_header *dm,
u8 s)
{
  char *bp = (char *) dm;
  if (!s)
    return "";

  bp += dm->length;
  while (s > 1)
  {
    bp += strlen(bp);
    bp++;
    s--;
  }
  return hw::strip(bp);
}


static string dmi_decode_ram(u16 data)
{
  string result = "";

  if (data & (1 << 2))
    result += "Standard ";
  if (data & (1 << 3))
    result += "FPM ";
  if (data & (1 << 4))
    result += "EDO ";
  if (data & (1 << 5))
    result += "PARITY ";
  if (data & (1 << 6))
    result += "ECC ";
  if (data & (1 << 7))
    result += "SIMM ";
  if (data & (1 << 8))
    result += "DIMM ";
  if (data & (1 << 9))
    result += "Burst EDO ";
  if (data & (1 << 10))
    result += "SDRAM ";

  return hw::strip(result);
}


static const char *dmi_board_type(u8 data)
{
  static const char *boardtypes[] =
  {
    "",
    "",
    "",
    N_("Server Blade"),
    N_("Connectivitiy Switch"),
    N_("System Management Module"),
    N_("Processor Module"),
    N_("I/O Module"),
    N_("Memory Module"),
    N_("Daughter Board"),
    N_("Motherboard"),
    N_("Processor/Memory Module"),
    N_("Processor/IO Module "),
    N_("Interconnect Board"),
  };
  if (data > 0x0D)
    return "";
  else
    return _(boardtypes[data]);
}


static void dmi_bios_features(u32 data1,
u32 data2,
hwNode & bios)
{
  if (data1 & (1 << 3))                           // BIOS characteristics not supported
    return;

  if (data1 & (1 << 4))                           // ISA
    bios.addCapability("ISA", _("ISA bus"));
  if (data1 & (1 << 5))                           // MCA
    bios.addCapability("MCA", _("MCA bus"));
  if (data1 & (1 << 6))                           // EISA
    bios.addCapability("EISA", _("EISA bus"));
  if (data1 & (1 << 7))                           // PCI
    bios.addCapability("PCI", _("PCI bus"));
  if (data1 & (1 << 8))                           // PCMCIA
    bios.addCapability("PCMCIA", _("PCMCIA/PCCard"));
  if (data1 & (1 << 9))                           // PNP
    bios.addCapability("PNP", _("Plug-and-Play"));
  if (data1 & (1 << 10))                          // APM
    bios.addCapability("APM", _("Advanced Power Management"));
  if (data1 & (1 << 11))                          // flashable BIOS
    bios.addCapability("upgrade", _("BIOS EEPROM can be upgraded"));
  if (data1 & (1 << 12))                          // BIOS shadowing
    bios.addCapability("shadowing", _("BIOS shadowing"));
  if (data1 & (1 << 13))                          // VL-VESA
    bios.addCapability("VESA", _("VESA video extensions"));
  if (data1 & (1 << 14))                          // ESCD
    bios.addCapability("ESCD", _("ESCD"));
  if (data1 & (1 << 15))                          // boot from CD
    bios.addCapability("cdboot", _("Booting from CD-ROM/DVD"));
  if (data1 & (1 << 16))                          // selectable boot
    bios.addCapability("bootselect", _("Selectable boot path"));
  if (data1 & (1 << 17))                          // BIOS ROM is socketed
    bios.addCapability("socketedrom", _("BIOS ROM is socketed"));
  if (data1 & (1 << 18))                          // PCMCIA boot
    bios.addCapability("pcmciaboot", _("Booting from PCMCIA"));
  if (data1 & (1 << 19))                          // Enhanced Disk Drive
    bios.addCapability("EDD", _("Enhanced Disk Drive extensions"));
  if (data1 & (1 << 20))                          // NEC 9800 floppy
    bios.addCapability("int13floppynec", _("NEC 9800 floppy"));
  if (data1 & (1 << 21))                          // Toshiba floppy
    bios.addCapability("int13floppytoshiba", _("Toshiba floppy"));
  if (data1 & (1 << 22))                          // 5.25" 360KB floppy
    bios.addCapability("int13floppy360", _("5.25\" 360KB floppy"));
  if (data1 & (1 << 23))                          // 5.25" 1.2MB floppy
    bios.addCapability("int13floppy1200", _("5.25\" 1.2MB floppy"));
  if (data1 & (1 << 24))                          // 3.5" 720KB floppy
    bios.addCapability("int13floppy720", _("3.5\" 720KB floppy"));
  if (data1 & (1 << 25))                          // 3.5" 2.88MB floppy
    bios.addCapability("int13floppy2880", _("3.5\" 2.88MB floppy"));
  if (data1 & (1 << 26))                          // print screen key
    bios.addCapability("int5printscreen", _("Print Screen key"));
  if (data1 & (1 << 27))                          // 8042 kbd controller
    bios.addCapability("int9keyboard", _("i8042 keyboard controller"));
  if (data1 & (1 << 28))                          // serial line control
    bios.addCapability("int14serial", _("INT14 serial line control"));
  if (data1 & (1 << 29))                          // printer
    bios.addCapability("int17printer", _("INT17 printer control"));
  if (data1 & (1 << 30))                          // CGA/Mono video
    bios.addCapability("int10video", _("INT10 CGA/Mono video"));
  if (data1 & (1 << 31))                          // NEC PC-98
    bios.addCapability("pc98", _("NEC PC-98"));
}


static void dmi_bios_features_ext(const u8 * data,
int len,
hwNode & bios)
{
  if (len < 1)
    return;

  if (data[0] & (1 << 0))                         // ACPI
    bios.addCapability("ACPI", _("ACPI"));
  if (data[0] & (1 << 1))                         // USB
    bios.addCapability("USB", _("USB legacy emulation"));
  if (data[0] & (1 << 2))                         // AGP
    bios.addCapability("AGP", _("AGP"));
  if (data[0] & (1 << 3))                         // I2O boot
    bios.addCapability("I2Oboot", _("I2O booting"));
  if (data[0] & (1 << 4))                         // LS-120 boot
    bios.addCapability("LS120boot", _("Booting from LS-120"));
  if (data[0] & (1 << 5))                         // ATAPI ZIP boot
    bios.addCapability("ZIPboot", _("Booting from ATAPI ZIP"));
  if (data[0] & (1 << 6))                         // 1394 boot
    bios.addCapability("IEEE1394boot", _("Booting from IEEE1394 (Firewire)"));
  if (data[0] & (1 << 7))                         // smart battery
    bios.addCapability("smartbattery", _("Smart battery"));

  if (len < 1)
    return;

  if (data[1] & (1 << 0))                         // BIOS boot specification
    bios.addCapability("biosbootspecification", _("BIOS boot specification"));
  if (data[1] & (1 << 1))                         // function-key initiated network service boot
    bios.addCapability("netboot",
      _("Function-key initiated network service boot"));
  if (data[1] & (1 << 3))
    bios.addCapability("uefi", _("UEFI specification is supported"));
  if (data[1] & (1 << 4))
    bios.addCapability("virtualmachine", _("This machine is a virtual machine"));
}


static unsigned long dmi_cache_size(u16 n)
{
  if (n & (1 << 15))
    return (n & 0x7FFF) * 64 * 1024;              // value is in 64K blocks
  else
    return (n & 0x7FFF) * 1024;                   // value is in 1K blocks
}


static void dmi_cache_sramtype(u16 c,
hwNode & n)
{
  string result = "";

//if (c & (1 << 0))
//result += "";
//if (c & (1 << 1))
//result += "";
  if (c & (1 << 2))
    n.addCapability("non-burst", _("Non-burst"));
  if (c & (1 << 3))
    n.addCapability("burst", _("Burst"));
  if (c & (1 << 4))
    n.addCapability("pipeline-burst", _("Pipeline burst"));
  if (c & (1 << 5))
    n.addCapability("synchronous", _("Synchronous"));
  if (c & (1 << 6))
    n.addCapability("asynchronous", _("Asynchronous"));
}


static void dmi_cache_describe(hwNode & n,
u16 config,
u16 sramtype = 0,
u8 cachetype = 0)
{
  string result = "";
  char buffer[10];

  dmi_cache_sramtype(sramtype, n);
  switch ((config >> 5) & 3)
  {
    case 0:
      n.addCapability("internal", _("Internal"));
      break;
    case 1:
      n.addCapability("external", _("External"));
      break;
  }
  snprintf(buffer, sizeof(buffer), "L%d ", (config & 7) + 1);
  result += " " + string(buffer);

  switch ((config >> 8) & 3)
  {
    case 0:
      n.addCapability("write-through", _("Write-trough"));
      break;
    case 1:
      n.addCapability("write-back", _("Write-back"));
      break;
    case 2:
      n.addCapability("varies", _("Varies With Memory Address"));
      break;
  }

  result += _("cache");

  switch (cachetype)
  {
    case 3:
      n.addCapability("instruction", _("Instruction cache"));
      break;
    case 4:
      n.addCapability("data", _("Data cache"));
      break;
    case 5:
      n.addCapability("unified", _("Unified cache"));
      break;
  }

  n.setDescription(hw::strip(result));
}


static const char *dmi_memory_array_location(u8 num)
{
  static const char *memory_array_location[] =
  {
    "",                                           /* 0x00 */
    "",
    "",
    N_("System board or motherboard"),
    N_("ISA add-on card"),
    N_("EISA add-on card"),
    N_("PCI add-on card"),
    N_("MCA add-on card"),
    N_("PCMCIA add-on card"),
    N_("Proprietary add-on card"),
    N_("NuBus"),                                      /* 0x0A , master.mif says 16 */
  };
  static const char *jp_memory_array_location[] =
  {
    N_("PC-98/C20 add-on card"),                      /* 0xA0 */
    N_("PC-98/C24 add-on card"),
    N_("PC-98/E add-on card"),
    N_("PC-98/Local bus add-on card"),
    N_("PC-98/Card slot add-on card"),                /* 0xA4, from master.mif */
  };
  if (num <= 0x0A)
    return _(memory_array_location[num]);
  if (num >= 0xA0 && num < 0xA4)
    return _(jp_memory_array_location[num]);
  return "";
}


static const char *dmi_memory_device_form_factor(u8 num)
{
  static const char *memory_device_form_factor[] =
  {
    "",
    "",
    "",
    N_(" SIMM"),
    N_(" SIP"),
    N_(" Chip"),
    N_(" DIP"),
    N_(" ZIP"),
    N_(" Proprietary Card"),
    N_(" DIMM"),
    N_(" TSOP"),
    N_(" Row of chips"),
    N_(" RIMM"),
    N_(" SODIMM"),
    N_(" SRIMM"),
    N_(" FB-DIMM"),
  };
  if (num > 0x0F)
    return "";
  return _(memory_device_form_factor[num]);
}


static const char *dmi_memory_device_type(u8 num)
{
  static const char *memory_device_type[] =
  {
    "",                                           /* 0x00 */
    "",
    "",
    N_(" DRAM"),
    N_(" EDRAM"),
    N_(" VRAM"),
    N_(" SRAM"),
    N_(" RAM"),
    N_(" ROM"),
    N_(" FLASH"),
    N_(" EEPROM"),
    N_(" FEPROM"),
    N_(" EPROM"),
    N_(" CDRAM"),
    N_(" 3DRAM"),
    N_(" SDRAM"),
    N_(" SGRAM"),
    N_(" RDRAM"),
    N_(" DDR"),                                       /* 0x12 */
    N_(" DDR2"),                                      /* 0x13 */
    N_(" DDR2 FB-DIMM"),                              /* 0x14 */
    "",
    "",
    "",
    " DDR3",
    " FBD2",					/* 0x19 */
    " DDR4",
    " LPDDR",
    " LPDDR2",
    " LPDDR3",
    " LPDDR4",                                        /* 0x1E */
  };
  if (num > 0x1E)
    return "";
  return _(memory_device_type[num]);
}


static string dmi_memory_device_detail(u16 v)
{
  string result = "";

  if (v & (1 << 3))
    result += " " + string(_("Fast-paged"));
  if (v & (1 << 4))
    result += " " + string(_("Static column"));
  if (v & (1 << 5))
    result += " " + string(_("Pseudo-static"));
  if (v & (1 << 6))
    result += " " + string(_("RAMBUS"));
  if (v & (1 << 7))
    result += " " + string(_("Synchronous"));
  if (v & (1 << 8))
    result += " " + string(_("CMOS"));
  if (v & (1 << 9))
    result += " " + string(_("EDO"));
  if (v & (1 << 10))
    result += " " + string(_("Window DRAM"));
  if (v & (1 << 11))
    result += " " + string(_("Cache DRAM"));
  if (v & (1 << 12))
    result += " " + string(_("Non-volatile"));
  if (v & (1 << 13))
    result += " " + string(_("Registered (Buffered)"));
  if (v & (1 << 14))
    result += " " + string(_("Unbuffered (Unregistered)"));
  if (v & (1 << 15))
    result += " " + string(_("LRDIMM"));

  return result;
}


void dmi_chassis(u8 code, hwNode & n)
{
  static const char *chassis_type[] =
  {
    "", "", NULL,                                 /* 0x00 */
    "", "", NULL,
    "", "", NULL,
    "desktop", N_("Desktop Computer"), "desktopcomputer",
    "low-profile", N_("Low Profile Desktop Computer"), "desktopcomputer",
    "pizzabox", N_("Pizza Box Computer"), "pizzabox",
    "mini-tower", N_("Mini Tower Computer"), "towercomputer",
    "tower", N_("Tower Computer"), "towercomputer",
    "portable", N_("Portable Computer"), "laptop",
    "laptop", N_("Laptop"), "laptop",
    "notebook", N_("Notebook"), "laptop",
    "handheld", N_("Hand Held Computer"), "pda",
    "docking", N_("Docking Station"), NULL,
    "all-in-one", N_("All In One"), NULL,
    "sub-notebook", N_("Sub Notebook"), "laptop",
    "space-saving", N_("Space-saving Computer"), NULL,
    "lunchbox", N_("Lunch Box Computer"), NULL,
    "server", N_("System"), "server",
    "expansion", N_("Expansion Chassis"), NULL,
    "sub", N_("Sub Chassis"), NULL,
    "bus-expansion", N_("Bus Expansion Chassis"), NULL,
    "peripheral", N_("Peripheral Chassis"), NULL,
    "raid", N_("RAID Chassis"), "md",
    "rackmount", N_("Rack Mount Chassis"), "rackmount",
    "sealed", N_("Sealed-case PC"), NULL,
    "multi-system", N_("Multi-system"), "cluster",     /* 0x19 */
    "pci", N_("Compact PCI"), NULL,
    "tca", N_("Advanced TCA"), NULL,
    "blade", N_("Blade"), NULL,		/* 0x1C */
    "enclosure", N_("Blade enclosure"), NULL,		/* 0x1D */
    "tablet", N_("Tablet"), NULL,
    "convertible", N_("Convertible"), NULL,
    "detachable", N_("Detachable"), NULL,               /* 0x20 */
  };

  if(code <= 0x20)
  {
    if(n.getDescription()=="") n.setDescription(_(chassis_type[1+3*code]));

    if(code >= 3)
    {
      n.setConfig("chassis", chassis_type[3*code] );
      if(chassis_type[2+3*code])
        n.addHint("icon", string(chassis_type[2+3*code]));
    }
  }
};
static const char *dmi_processor_family(uint16_t code)
{
  static const char *processor_family[] =
  {
    "",                                           /* 0x00 */
    "",
    "",
    "8086",
    "80286",
    "i386",
    "i486",
    "8087",
    "80287",
    "80387",
    "80487",
    "Pentium",
    "Pentium Pro",
    "Pentium II",
    "Pentium MMX",
    "Celeron",
    "Pentium II Xeon",
    "Pentium III",
    "M1",
    "M2",
    "Celeron M",                                           /* 0x14 */
    "Pentium 4 HT",
    "",
    "",
    "Duron",
    "K5",
    "K6",
    "K6-2",
    "K6-3",
    "Athlon",
    "AMD29000",
    "K6-2+",
    "Power PC",
    "Power PC 601",
    "Power PC 603",
    "Power PC 603+",
    "Power PC 604",
    "Power PC 620",
    "Power PC x704",
    "Power PC 750",
    "Core Duo",                                           /* 0x28 */
    "Core Duo mobile",
    "Core Solo mobile",
    "Atom",
    "Core M",
    "",
    "",
    "",
    "Alpha",                                      /* 0x30 */
    "Alpha 21064",
    "Alpha 21066",
    "Alpha 21164",
    "Alpha 21164PC",
    "Alpha 21164a",
    "Alpha 21264",
    "Alpha 21364",
    "Turion II Ultra Dual-Core Mobile M",         /* 0x38 */
    "Turion II Dual-Core Mobile M",
    "Athlon II Dual-Core M",
    "Opteron 6100",
    "Opteron 4100",
    "Opteron 6200",
    "Opteron 4200",
    "FX",
    "MIPS",                                       /* 0x40 */
    "MIPS R4000",
    "MIPS R4200",
    "MIPS R4400",
    "MIPS R4600",
    "MIPS R10000",
    "C-Series",                                   /* 0x46 */
    "E-Series",
    "A-Series",
    "G-Series",
    "Z-Series",
    "R-Series",
    "Opteron 4300",
    "Opteron 6300",
    "Opteron 3300",
    "FirePro",
    "SPARC",
    "SuperSPARC",
    "MicroSPARC II",
    "MicroSPARC IIep",
    "UltraSPARC",
    "UltraSPARC II",
    "UltraSPARC IIi",
    "UltraSPARC III",
    "UltraSPARC IIIi",
    "",                                           /* 0x59 */
    "",
    "",
    "",
    "",
    "",
    "",                                           /* 0x5F */
    "68040",
    "68xxx",
    "68000",
    "68010",
    "68020",
    "68030",
    "Athlon X4",                                  /* 0x66 */
    "Opteron X1000",
    "Opteron X2000 APU",
    "",
    "",
    "",
    "",
    "",
    "",
    "",                                           /* 0x6F */
    "Hobbit",
    "",                                           /* 0x71 */
    "",
    "",
    "",
    "",
    "",
    "",                                           /* 0x77 */
    "Crusoe TM5000",
    "Crusoe TM3000",
    "Efficeon TM8000",                                           /* 0x7A */
    "",
    "",
    "",
    "",
    "",                                           /* 0x7F */
    "Weitek",
    "",                                           /* 0x81 */
    "Itanium",
    "Athlon 64",
    "Opteron",
    "Sempron",                                           /* 0x85 */
    "Turion 64 Mobile",
    "Dual-Core Opteron",
    "Athlon 64 X2 Dual-Core",
    "Turion 64 X2 Mobile",
    "Quad-Core Opteron",
    "3rd-generation Opteron",
    "Phenom FX Quad-Core",
    "Phenom X4 Quad-Core",
    "Phenom X2 Dual-Core",
    "Athlon X2 Dual-Core",                                           /* 0x8F */
    "PA-RISC",
    "PA-RISC 8500",
    "PA-RISC 8000",
    "PA-RISC 7300LC",
    "PA-RISC 7200",
    "PA-RISC 7100LC",
    "PA-RISC 7100",
    "",                                           /* 0x97 */
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",                                           /* 0x9F */
    "V30",
    "Xeon 3200 Quad-Core",                        /* 0xA1 */
    "Xeon 3000 Dual-Core",
    "Xeon 5300 Quad-Core",
    "Xeon 5100 Dual-Core",
    "Xeon 5000 Dual-Core",
    "Xeon LV Dual-Core",
    "Xeon ULV Dual-Core",
    "Xeon 7100 Dual-Core",
    "Xeon 5400 Quad-Core",
    "Xeon Quad-Core",
    "Xeon 5200 Dual-Core",
    "Xeon 7200 Dual-Core",
    "Xeon 7300 Quad-Core",
    "Xeon 7400 Quad-Core",
    "Xeon 7400 Multi-Core",                       /* 0xAF */
    "Pentium III Xeon",
    "Pentium III Speedstep",
    "Pentium 4",
    "Xeon",
    "AS400",
    "Xeon MP",
    "Athlon XP",
    "Athlon MP",
    "Itanium 2",
    "Pentium M",
    "Celeron D",                                           /* 0xBA */
    "Pentium D",
    "Pentium Extreme Edition",
    "Core Solo",
    "",
    "Core 2 Duo",
    "Core 2 Solo",
    "Core 2 Extreme",
    "Core 2 Quad",
    "Core 2 Extreme mobile",
    "Core 2 Duo mobile",
    "Core 2 Solo mobile",
    "Core i7",
    "Dual-Core Celeron",                                           /* 0xC7 */
    "IBM390",
    "G4",
    "G5",
    "ESA/390 G6",                                           /* 0xCB */
    "z/Architecture base",
    "Core i5",
    "Core i3",
    "",
    "",
    "",
    "VIA C7-M",
    "VIA C7-D",
    "VIA C7",
    "VIA Eden",
    "Multi-Core Xeon",
    "Dual-Core Xeon 3xxx",
    "Quad-Core Xeon 3xxx",
    "VIA Nano",
    "Dual-Core Xeon 5xxx",
    "Quad-Core Xeon 5xxx",	/* 0xDB */
    "",
    "Dual-Core Xeon 7xxx",
    "Quad-Core Xeon 7xxx",
    "Multi-Core Xeon 7xxx",
    "Multi-Core Xeon 3400",
    "",
    "",
    "",
    "Opteron 3000",
    "AMD Sempron II",
    "Embedded Opteron Quad-Core",
    "Phenom Triple-Core",
    "Turion Ultra Dual-Core Mobile",
    "Turion Dual-Core Mobile",
    "Athlon Dual-Core",
    "Sempron SI",
    "Phenom II",
    "Athlon II",
    "Six-Core Opteron",
    "Sempron M",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",                                           /* 0xF9 */
    "i860",
    "i960",
    "",                                           /* 0xFC */
    "",
    "",
    ""                                            /* 0xFF */
  };

  if (code <= 0xFF)
    return processor_family[code];

  switch (code)
  {
    case 0x104: return "SH-3";
    case 0x105: return "SH-4";
    case 0x118: return "ARM";
    case 0x119: return "StrongARM";
    case 0x12C: return "6x86";
    case 0x12D: return "MediaGX";
    case 0x12E: return "MII";
    case 0x140: return "WinChip";
    case 0x15E: return "DSP";
    case 0x1F4: return "Video Processor";
    default: return "";
  }
}


static string dmi_handle(u16 handle)
{
  char buffer[10];
  snprintf(buffer, sizeof(buffer), "DMI:%04X", handle);

  return string(buffer);
}


static void dmi_table(const u8 *buf,
int len,
hwNode & node,
int dmiversionmaj,
int dmiversionmin)
{
  struct dmi_header *dm;
  hwNode *hardwarenode = NULL;
  const u8 *data;
  int i = 0;
  string handle;

  if (len == 0)
// no data
    return;

  data = buf;
  while (data + sizeof(struct dmi_header) <= (u8 *) buf + len)
  {
    u32 u, u2;
    dm = (struct dmi_header *) data;

/*
 * we won't read beyond allocated memory
 */
    if (data + dm->length > (u8 *) buf + len)
// incomplete structure, abort decoding
      break;

    handle = dmi_handle(dm->handle);

    hardwarenode = node.getChild("core");
    if (!hardwarenode)
    {
      node.addChild(hwNode("core", hw::bus));
      hardwarenode = node.getChild("core");
    }
    if (!hardwarenode)
      hardwarenode = &node;

    switch (dm->type)
    {
      case 0:
// BIOS Information Block
        {
          string release(dmi_string(dm,
            data[8]));
          hwNode newnode("firmware",
            hw::memory,
            dmi_string(dm,
            data[4]));
          newnode.setVersion(dmi_string(dm, data[5]));
          newnode.setCapacity(64 * data[9] * 1024);
          newnode.setSize(16 * (0x10000 - (data[7] << 8 | data[6])));
//newnode.setPhysId(16 * (data[7] << 8 | data[6]));
          newnode.setPhysId(dm->handle);
          newnode.setDescription(_("BIOS"));
          newnode.addHint("icon", string("chip"));
          newnode.claim();

          dmi_bios_features(data[13] << 24 | data[12] << 16 | data[11] << 8 |
            data[10],
            data[17] << 24 | data[16] << 16 | data[15] << 8 |
            data[14], newnode);

          if (dm->length > 0x12)
            dmi_bios_features_ext(&data[0x12], dm->length - 0x12, newnode);

          if (release != "")
            newnode.setDate(release);
          hardwarenode->addChild(newnode);
        }
        break;

      case 1:
// System Information Block
        node.setHandle(handle);
        node.setVendor(dmi_string(dm, data[4]));
        node.setProduct(dmi_string(dm, data[5]));
        node.setVersion(dmi_string(dm, data[6]));
        node.setSerial(dmi_string(dm, data[7]));
        if (dm->length >= 0x19)
          node.setConfig("uuid", dmi_uuid(data + 8));
        if (dm->length >= 0x1B)
        {
          node.setConfig("sku", dmi_string(dm, data[0x19]));
	  if (dmi_string(dm, data[0x19]) != "")
	    node.setProduct(node.getProduct() + " (" + dmi_string(dm, data[0x19]) + ")");
          node.setConfig("family", dmi_string(dm, data[0x1A]));
        }
        break;

      case 2:
// Board Information Block
        {

                                                  // we are the only system board on the computer so connect everything to us
          if ((dm->length <= 0x0E) || (data[0x0E] == 0))
          {
            hardwarenode->setVendor(dmi_string(dm, data[4]));
            hardwarenode->setProduct(dmi_string(dm, data[5]));
            hardwarenode->setVersion(dmi_string(dm, data[6]));
            hardwarenode->setSerial(dmi_string(dm, data[7]));
            if(dm->length >= 0x0a)
              hardwarenode->setSlot(dmi_string(dm, data[0x0A]));
            hardwarenode->setHandle(handle);
            hardwarenode->setDescription(_("Motherboard"));
            hardwarenode->addHint("icon", string("motherboard"));
          }
          else
          {
            hwNode newnode("board",
              hw::bus);

            if (dm->length >= 0x0E)
              for (int i = 0; i < data[0x0E]; i++)
                newnode.
                  attractHandle(dmi_handle
                  (data[0x0F + 2 * i + 1] << 8 |
                  data[0x0F + 2 * i]));

            newnode.setVendor(dmi_string(dm, data[4]));
            newnode.setProduct(dmi_string(dm, data[5]));
            newnode.setVersion(dmi_string(dm, data[6]));
            newnode.setSerial(dmi_string(dm, data[7]));
            newnode.setSlot(dmi_string(dm, data[0x0A]));
            newnode.setHandle(handle);
            newnode.setPhysId(dm->handle);
            newnode.setDescription(dmi_board_type(data[0x0D]));
            hardwarenode->addChild(newnode);
          }
        }
        break;

      case 3:
// Chassis Information Block
//
// special case: if the system characteristics are still unknown,
// use values from the chassis
        if (node.getVendor() == "")
          node.setVendor(dmi_string(dm, data[4]));
        if (node.getProduct() == "")
          node.setProduct(dmi_string(dm, data[5]));
        if (node.getVersion() == "")
          node.setVersion(dmi_string(dm, data[6]));
        if (node.getSerial() == "")
          node.setSerial(dmi_string(dm, data[7]));
        dmi_chassis(data[5] & 0x7F, node);
        break;

      case 4:
// Processor
        {
          hwNode newnode("cpu",
            hw::processor);

          newnode.claim();
          newnode.setBusInfo(cpubusinfo(currentcpu++));
          newnode.setSlot(dmi_string(dm, data[4]));
          newnode.setDescription(_("CPU"));
          newnode.addHint("icon", string("cpu"));
          if (dm->length >= 0x2A)
            newnode.setProduct(dmi_processor_family(
                (((uint16_t) data[0x29]) << 8) + data[0x28]));
          else
            newnode.setProduct(dmi_processor_family(data[6]));
          newnode.setVersion(dmi_string(dm, data[0x10]));
          newnode.setVendor(dmi_string(dm, data[7]));
          newnode.setPhysId(dm->handle);
          if (dm->length > 0x1A)
          {
// L1 cache
            newnode.attractHandle(dmi_handle(data[0x1B] << 8 | data[0x1A]));
// L2 cache
            newnode.attractHandle(dmi_handle(data[0x1D] << 8 | data[0x1C]));
// L3 cache
            newnode.attractHandle(dmi_handle(data[0x1F] << 8 | data[0x1E]));
          }
          if (dm->length > 0x20)
          {
            newnode.setSerial(dmi_string(dm, data[0x20]));
            if (dmi_string(dm, data[0x22]) != "")
              newnode.setProduct(newnode.getProduct() + " (" +
                string(dmi_string(dm, data[0x22])) + ")");
          }

// CPU socket populated ?
          if (data[0x18] & 0x40)
          {
	// external clock
          	u = data[0x13] << 8 | data[0x12];
          	newnode.setClock(u * 1000000);
	// maximum speed
          	u = data[0x15] << 8 | data[0x14];
          	newnode.setCapacity(u * 1000000);
	// current speed
          	u = data[0x17] << 8 | data[0x16];
          	newnode.setSize(u * 1000000);

          	if (newnode.getCapacity() < newnode.getSize())
            	newnode.setCapacity(0);

	// CPU enabled/disabled by BIOS?
          	u = data[0x18] & 0x07;
          	if ((u == 2) || (u == 3) || (u == 4))
            	newnode.disable();
          }
          else
          {
            newnode.setBusInfo("");	// blank businfo to make sure further detections can't confuse this empty CPU slot with a real CPU
            newnode.setDescription(newnode.getDescription() + " " + _("[empty]"));
            newnode.disable();
          }

          if (dm->length >= 0x28)
          { 
            if (data[0x23] != 0)
		newnode.setConfig("cores", data[0x23]);
            if (data[0x24] != 0)
		newnode.setConfig("enabledcores", data[0x24]);
            if (data[0x25] != 0)
		newnode.setConfig("threads", data[0x25]);
            if (data[0x26] & 0x4)
		newnode.addCapability("x86-64", "64bits extensions (x86-64)");
          }

          newnode.setHandle(handle);

          hardwarenode->addChild(newnode);
        }
        break;

      case 5:
// Memory Controller (obsolete in DMI 2.1+)
// therefore ignore the entry if the DMI version is recent enough
        if ((dmiversionmaj < 2)
          || ((dmiversionmaj == 2) && (dmiversionmin < 1)))
        {
          unsigned long long size = 0;
          hwNode newnode("memory",
            hw::memory);

          newnode.setHandle(handle);
          newnode.setPhysId(dm->handle);

          size = data[0x0E] * (1 << data[8]) * 1024 * 1024;
          newnode.setCapacity(size);

// loop through the controller's slots and link them to us
          for (i = 0; i < data[0x0E]; i++)
          {
            u16 slothandle = data[0x0F + 2 * i + 1] << 8 | data[0x0F + 2 * i];
            newnode.attractHandle(dmi_handle(slothandle));
          }

          newnode.setProduct(dmi_decode_ram(data[0x0C] << 8 | data[0x0B]) +
            _(" Memory Controller"));

          hardwarenode->addChild(newnode);
        }
        break;

      case 6:
// Memory Bank (obsolete in DMI 2.1+)
// therefore ignore the entry if the DMI version is recent enough
        if ((dmiversionmaj < 2)
          || ((dmiversionmaj == 2) && (dmiversionmin < 1)))
        {
          hwNode newnode("bank",
            hw::memory);
          unsigned long long clock = 0;
          unsigned long long capacity = 0;
          unsigned long long size = 0;

          newnode.setDescription(_("empty memory bank"));
          newnode.setSlot(dmi_string(dm, data[4]).c_str());
          if (data[6])
            clock = 1000000000 / data[6];         // convert value from ns to Hz
          newnode.setClock(clock);
          newnode.setDescription(dmi_decode_ram(data[8] << 8 | data[7]));
          newnode.addHint("icon", string("memory"));
// installed size
          switch (data[9] & 0x7F)
          {
            case 0x7D:
            case 0x7E:
            case 0x7F:
              break;
            default:
              size = (1 << (data[9] & 0x7F)) << 20;
          }
          if (data[9] & 0x80)
            size *= 2;
// enabled size
          switch (data[10] & 0x7F)
          {
            case 0x7D:
            case 0x7E:
            case 0x7F:
              break;
            default:
              capacity = (1 << (data[10] & 0x7F)) << 20;
          }
          if (data[10] & 0x80)
            capacity *= 2;

          newnode.setCapacity(capacity);
          newnode.setSize(size);
          if(newnode.getSize()==0)
            newnode.setDescription(newnode.getDescription() + " " + _("[empty]"));
          if ((data[11] & 4) == 0)
          {
            if (data[11] & (1 << 0))
// bank has uncorrectable errors (BIOS disabled)
              newnode.disable();
          }

          newnode.setHandle(handle);

          hardwarenode->addChild(newnode);
        }
        break;
      case 7:
// Cache
        {
          hwNode newnode("cache",
            hw::memory);
	  int level;

          newnode.setSlot(dmi_string(dm, data[4]));
          u = data[6] << 8 | data[5];
          level = 1 + (u & 7);

          if (dm->length > 0x11)
            dmi_cache_describe(newnode, u, data[0x0E] << 8 | data[0x0D],
              data[0x11]);
          else
            dmi_cache_describe(newnode, u, data[0x0E] << 8 | data[0x0D]);

          if (!(u & (1 << 7)))
            newnode.disable();

          newnode.setConfig("level", level);
          newnode.setSize(dmi_cache_size(data[9] | data[10] << 8));
          newnode.setCapacity(dmi_cache_size(data[7] | (data[8] << 8)));
          if ((dm->length > 0x0F) && (data[0x0F] != 0))
          {
                                                  // convert from ns to Hz
            newnode.setClock(1000000000 / data[0x0F]);
          }

          newnode.setHandle(handle);
          newnode.setPhysId(dm->handle);
          newnode.claim();
          if(newnode.getSize()!=0)
            hardwarenode->addChild(newnode);
        }
        break;
      case 8:
//printf("\tPort Connector\n");
//printf("\t\tInternal Designator: %s\n",
//dmi_string(dm, data[4]).c_str());
//printf("\t\tInternal Connector Type: %s\n",
//dmi_port_connector_type(data[5]));
//printf("\t\tExternal Designator: %s\n",
//dmi_string(dm, data[6]).c_str());
//printf("\t\tExternal Connector Type: %s\n",
//dmi_port_connector_type(data[7]));
//printf("\t\tPort Type: %s\n", dmi_port_type(data[8]));
        break;
      case 9:
#if 0
        {
          hwNode newnode("cardslot",
            hw::bus);
          newnode.setHandle(handle);
          newnode.setSlot(dmi_string(dm, data[4]));
          printf("\t\tType: %s%s%s\n",
            dmi_bus_width(data[6]),
            dmi_card_size(data[8]), dmi_bus_name(data[5]));
          if (data[7] == 3)
            printf("\t\tStatus: Available.\n");
          if (data[7] == 4)
            printf("\t\tStatus: In use.\n");
          if (data[11] & 0xFE)
            dmi_card_props(data[11]);
//hardwarenode->addChild(newnode);
        }
#endif
        break;
      case 10:
#if 0
        printf("\tOn Board Devices Information\n");
        for (u = 2; u * 2 + 1 < dm->length; u++)
        {
          printf("\t\tDescription: %s : %s\n",
            dmi_string(dm, data[1 + 2 * u]).c_str(),
            (data[2 * u]) & 0x80 ? "Enabled" : "Disabled");
          printf("\t\tType: %s\n", dmi_onboard_type(data[2 * u]));
        }
#endif

        break;
      case 11:
// OEM Data
        break;
      case 12:
// Configuration Information
        break;
      case 13:
#if 0
        printf("\tBIOS Language Information\n");
        printf("\t\tInstallable Languages: %u\n", data[4]);
        for (u = 1; u <= data[4]; u++)
        {
          printf("\t\t\t%s\n", dmi_string(dm, u).c_str());
        }
        printf("\t\tCurrently Installed Language: %s\n",
          dmi_string(dm, data[21]).c_str());
#endif
        break;
      case 14:
// Group Associations
        break;
      case 15:
// Event Log
        break;
      case 16:
// Physical Memory Array
        {
          hwNode newnode("memory",
            hw::memory);
          string id = "";
          string description = "";
          switch (data[5])
          {
            case 0x03:
              description = _("System Memory");
              newnode.claim();
              newnode.addHint("icon", string("memory"));
              break;
            case 0x04:
              description = _("Video Memory");
              break;
            case 0x05:
              description = _("Flash Memory");
              break;
            case 0x06:
              description = _("NVRAM");
              break;
            case 0x07:
              description = _("Cache Memory");
              newnode.addHint("icon", string("memory"));
              break;
            default:
              description = _("Generic Memory");
              newnode.addHint("icon", string("memory"));
          }

          newnode.setHandle(handle);
          newnode.setPhysId(dm->handle);
          newnode.setDescription(description);
          newnode.setSlot(dmi_memory_array_location(data[4]));
          switch (data[6])
          {
            case 0x04:
              newnode.addCapability("parity", _("Parity error correction"));
              newnode.setConfig("errordetection", "parity");
              break;
            case 0x05:
              newnode.addCapability("ecc", _("Single-bit error-correcting code (ECC)"));
              newnode.setConfig("errordetection", "ecc");
              break;
            case 0x06:
              newnode.addCapability("ecc", _("Multi-bit error-correcting code (ECC)"));
              newnode.setConfig("errordetection", "multi-bit-ecc");
              break;
            case 0x07:
              newnode.addCapability("crc", _("CRC error correction"));
              newnode.setConfig("errordetection", "crc");
              break;
          }
          u2 = data[10] << 24 | data[9] << 16 | data[8] << 8 | data[7];
          if (u2 != 0x80000000)                   // magic value for "unknown"
            newnode.setCapacity(u2 * 1024);
          else if (dm->length >= 0x17)
          {
            uint64_t capacity = (((uint64_t) data[0x16]) << 56) +
                                (((uint64_t) data[0x15]) << 48) +
                                (((uint64_t) data[0x14]) << 40) +
                                (((uint64_t) data[0x13]) << 32) +
                                (((uint64_t) data[0x12]) << 24) +
                                (((uint64_t) data[0x11]) << 16) +
                                (((uint64_t) data[0x10]) << 8) +
                                data[0x0F];
            newnode.setCapacity(capacity);
          }
          hardwarenode->addChild(newnode);
        }
        break;
      case 17:
// Memory Device
        {
          hwNode newnode("bank",
            hw::memory);
          string slot = "";
          string description = "";
          unsigned long long size = 0;
          unsigned long long clock = 0;
          u16 width = 0;
          char bits[10];
          string arrayhandle;
          newnode.setDescription(_("empty memory bank"));
          newnode.addHint("icon", string("memory"));
          arrayhandle = dmi_handle(data[5] << 8 | data[4]);
          strcpy(bits, "");
// total width
          u = data[9] << 8 | data[8];
          if (u != 0xffff)
            width = u;
//data width
          u = data[11] << 8 | data[10];
          if ((u != 0xffff) && (u != 0))
          {
            if ((u == width) || (width == 0))
            {
              snprintf(bits, sizeof(bits), "%d", u);
              newnode.setWidth(width);
            }
            else
            {
              snprintf(bits, sizeof(bits), "%d/%d", width, u);
              newnode.setWidth(u<width?u:width);
            }
          }
          else
          {
            if (width != 0)
            {
              snprintf(bits, sizeof(bits), "%d", width);
              newnode.setWidth(width);
            }
          }

// size
          u = data[13] << 8 | data[12];
          if(u == 0x7fff) {
             unsigned long long extendsize = (data[0x1f] << 24) | (data[0x1e] << 16) | (data[0x1d] << 8) | data[0x1c];
             extendsize &= 0x7fffffffUL;
             size = extendsize * 1024ULL * 1024ULL;
          }
	  else
          if (u != 0xffff)
            size = (1024ULL * (u & 0x7fff) * ((u & 0x8000) ? 1 : 1024ULL));
          description += string(dmi_memory_device_form_factor(data[14]));
          slot = dmi_string(dm, data[16]);
//printf("\t\tBank Locator: %s\n", dmi_string(dm, data[17]));
          description += string(dmi_memory_device_type(data[18]));
          u = data[20] << 8 | data[19];
          if (u & 0x1ffe)
            description += dmi_memory_device_detail(u);
          if (dm->length > 21)
          {
            char buffer[80];
            u = data[22] << 8 | data[21];
// speed
            clock = u * 1000000;                  // u is a frequency in MHz
            if (u == 0)
              strcpy(buffer, "");
            else
              snprintf(buffer, sizeof(buffer),
                "%u MHz (%.1f ns)", u, (1000.0 / u));
            description += " " + string(buffer);
          }

          newnode.setHandle(handle);
//newnode.setPhysId(dm->handle);
          newnode.setSlot(slot);
          if (dm->length > 23)
            newnode.setVendor(dmi_string(dm, data[23]));
          if (dm->length > 24)
            newnode.setSerial(dmi_string(dm, data[24]));
          if (dm->length > 26)
            newnode.setProduct(dmi_string(dm, data[26]));
          newnode.setDescription(description);
          newnode.setSize(size);
          if(newnode.getSize()==0)
            newnode.setDescription(newnode.getDescription() + " " + _("[empty]"));
          newnode.setClock(clock);
          hwNode *memoryarray = hardwarenode->findChildByHandle(arrayhandle);
          if (memoryarray)
            memoryarray->addChild(newnode);
          else
          {
            hwNode ramnode("memory",
              hw::memory);
            ramnode.addChild(newnode);
            ramnode.addHint("icon", string("memory"));
            hardwarenode->addChild(ramnode);
          }
        }
        break;
      case 18:
// 32-bit Memory Error Information
        break;
      case 19:
// Memory Array Mapped Address
        {
          hwNode newnode("range",
            hw::address);
          unsigned long start, end;
          string arrayhandle = dmi_handle(data[0x0D] << 8 | data[0x0C]);
          start = ((data[4] | data[5] << 8) | (data[6] | data[7] << 8) << 16);
          end = ((data[8] | data[9] << 8) | (data[10] | data[11] << 8) << 16);
          if (end - start < 512)                  // memory range is smaller thant 512KB
          {
// consider that values were expressed in megagytes
            start *= 1024;
            end *= 1024;
          }

          newnode.setStart(start * 1024);         // values are in KB
          if (end != start)
                                                  // values are in KB
              newnode.setSize((end - start + 1) * 1024);
          newnode.setHandle(handle);
#if 0
          hwNode *memoryarray = hardwarenode->findChildByHandle(arrayhandle);
          if (memoryarray)
            memoryarray->setPhysId(newnode.getStart());
#endif
        }
        break;
      case 20:
// Memory Device Mapped Address
        {
          hwNode newnode("range",
            hw::address);
          unsigned long start, end;
          string devicehandle = dmi_handle(data[0x0D] << 8 | data[0x0C]);
          start = ((data[4] | data[5] << 8) | (data[6] | data[7] << 8) << 16);
          end = ((data[8] | data[9] << 8) | (data[10] | data[11] << 8) << 16);
          if (end - start < 512)                  // memory range is smaller than 512KB
          {
// consider that values were expressed in megagytes
            start *= 1024;
            end *= 1024;
          }

          newnode.setStart(start * 1024);         // values are in KB
          if (end != start)
                                                  // values are in KB
              newnode.setSize((end - start + 1) * 1024);
          newnode.setHandle(handle);
#if 0
          hwNode *memorydevice = hardwarenode->findChildByHandle(devicehandle);
          if (memorydevice && (newnode.getSize() != 0)
            && (newnode.getSize() <= memorydevice->getSize()))
            memorydevice->setPhysId(newnode.getStart());
#endif
        }
        break;
      case 21:
// Built-In Pointing Device
        break;
      case 22:
// Portable Battery
        if (dm->length < 0x10)
          break;
        else
        {
          hwNode batt("battery", hw::power);

          batt.addHint("icon", string("battery"));
          batt.claim();
          batt.setHandle(handle);
          batt.setVendor(dmi_string(dm, data[0x05]));
                                                  // name
          batt.setProduct(dmi_string(dm, data[0x08]));
                                                  // location
          batt.setSlot(dmi_string(dm, data[0x04]));
          if(data[0x06] || dm->length<0x1A)       // manufacture date
            batt.setVersion(dmi_string(dm, data[0x06]));
          if(data[0x07] || dm->length<0x1A)
            batt.setSerial(dmi_string(dm, data[0x07]));
          batt.setConfig("voltage", dmi_battery_voltage(data[0x0c] + 256*data[0x0d]));
          if(dm->length<0x1A)
            batt.setCapacity(dmi_battery_capacity(data[0x0a] + 256*data[0x0b], 1));
          else
            batt.setCapacity(dmi_battery_capacity(data[0x0a] + 256*data[0x0b], data[0x15]));
          if(data[0x09]!=0x02 || dm->length<0x1A)
            batt.setDescription(hw::strip(string(dmi_battery_chemistry(data[0x09])) + " Battery"));

          node.addChild(batt);
        }
        break;
      case 23:
// System Reset
        break;
      case 24:
// Hardware Security
        if (dm->length < 0x05)
          break;
        node.setConfig("power-on_password",
          dmi_hardware_security_status(data[0x04]>>6));
        node.setConfig("keyboard_password",
          dmi_hardware_security_status((data[0x04]>>4)&0x3));
        node.setConfig("administrator_password",
          dmi_hardware_security_status((data[0x04]>>2)&0x3));
        node.setConfig("frontpanel_password",
          dmi_hardware_security_status(data[0x04]&0x3));
        break;
      case 25:
// System Power Controls
        break;
      case 26:
// Voltage Sensor
        break;
      case 27:
// Cooling Device
        break;
      case 28:
// Temperature Sensor
        break;
      case 29:
// Current Sensor
        break;
      case 30:
// Out-of-Band Remote Access
        if (dm->length < 0x06)
          break;
        else
        {
          hwNode oob("remoteaccess", hw::system);

          oob.setVendor(dmi_string(dm, data[0x04]));
          if(data[0x05] & 0x2) oob.addCapability("outbound", _("make outbound connections"));
          if(data[0x05] & 0x1) oob.addCapability("inbound", _("receive inbound connections"));
          node.addChild(oob);
        }
        break;
      case 31:
// Boot Integrity Services Entry Point
        break;
      case 32:
// System Boot Information
        if (dm->length < 0x0B)
          break;
        node.setConfig("boot", dmi_bootinfo(data[0x0a]));
        break;
      case 33:
// 64-bit Memory Error Information
        break;
      case 34:
// Management Device
        break;
      case 35:
// Management Device Component
        break;
      case 36:
// Management Device Threshold Data
        break;
      case 37:
// Memory Channel
        break;
      case 38:
// IPMI Device
        break;
      case 39:
// Power Supply
        if (dm->length < 0x15)
          break;
        else
        {
          hwNode power("power", hw::power);

          power.setDescription(dmi_string(dm, data[0x06]));
          power.setVendor(dmi_string(dm, data[0x07]));
          power.setSerial(dmi_string(dm, data[0x08]));
          power.setProduct(dmi_string(dm, data[0x0a]));
          power.setVersion(dmi_string(dm, data[0x0b]));
          power.setCapacity(data[0x0c] + 256*data[0x0d]);
          node.addChild(power);
        }
        break;
      case 126:
// Inactive
        break;
      case 127:
// End-of-Table
        break;
      default:
        break;
    }
    data += dm->length;
    while (*data || data[1])
      data++;
    data += 2;
    i++;
  }
}


static bool smbios_entry_point(const u8 *buf, size_t len,
    hwNode & n, u16 & dmimaj, u16 & dmimin,
    uint32_t & table_len, uint64_t & table_base)
{
  u8 smmajver = 0;
  u8 smminver = 0;

  if (len >= 24 && memcmp(buf, "_SM3_", 5) == 0 && checksum(buf, buf[6]))
  {
    // SMBIOS 3.0 entry point structure (64-bit)
    dmimaj = smmajver = buf[7];
    dmimin = smminver = buf[8];
    table_len = (((uint32_t) buf[15]) << 24) +
                (((uint32_t) buf[14]) << 16) +
                (((uint32_t) buf[13]) << 8) +
                (uint32_t) buf[12];
    table_base = (((uint64_t) buf[23]) << 56) +
                 (((uint64_t) buf[22]) << 48) +
                 (((uint64_t) buf[21]) << 40) +
                 (((uint64_t) buf[20]) << 32) +
                 (((uint64_t) buf[19]) << 24) +
                 (((uint64_t) buf[18]) << 16) +
                 (((uint64_t) buf[17]) << 8) +
                 (uint64_t) buf[16];
  }
  else if (len >= 31 &&
      memcmp(buf, "_SM_", 4) == 0 &&
      memcmp(buf + 16, "_DMI_", 5) == 0 &&
      checksum(buf + 16, 15))
  {
    // SMBIOS 2.1 entry point structure
    dmimaj = smmajver = buf[6];
    dmimin = smminver = buf[7];
    table_len = (((uint16_t) buf[23]) << 8) +
                (uint16_t) buf[22];
    table_base = (((uint32_t) buf[27]) << 24) +
                 (((uint32_t) buf[26]) << 16) +
                 (((uint32_t) buf[25]) << 8) +
                 (uint32_t) buf[24];
  }

  if (smmajver > 0)
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d.%d", smmajver, smminver);
    n.addCapability("smbios-"+string(buffer), "SMBIOS version "+string(buffer));
    snprintf(buffer, sizeof(buffer), "%d.%d", dmimaj, dmimin);
    n.addCapability("dmi-"+string(buffer), "DMI version "+string(buffer));

    return true;
  }

  return false;
}


static bool scan_dmi_sysfs(hwNode & n)
{
  if (!exists(SYSFSDMI "/smbios_entry_point") || !exists(SYSFSDMI "/DMI"))
    return false;

  uint32_t table_len = 0;
  uint64_t table_base = 0;
  u16 dmimaj = 0, dmimin = 0;

  ifstream ep_stream(SYSFSDMI "/smbios_entry_point",
      ifstream::in | ifstream::binary | ifstream::ate);
  ifstream::pos_type ep_len = ep_stream.tellg();
  vector < u8 > ep_buf(ep_len);
  ep_stream.seekg(0, ifstream::beg);
  ep_stream.read((char *)ep_buf.data(), ep_len);
  if (!ep_stream)
    return false;
  if (!smbios_entry_point(ep_buf.data(), ep_len, n,
        dmimaj, dmimin, table_len, table_base))
    return false;

  ifstream dmi_stream(SYSFSDMI "/DMI",
      ifstream::in | ifstream::binary | ifstream::ate);
  ifstream::pos_type dmi_len = dmi_stream.tellg();
  vector < u8 > dmi_buf(dmi_len);
  dmi_stream.seekg(0, ifstream::beg);
  dmi_stream.read((char *)dmi_buf.data(), dmi_len);
  if (!dmi_stream)
    return false;
  dmi_table(dmi_buf.data(), dmi_len, n, dmimaj, dmimin);

  return true;
}


#if defined(__i386__) || defined(__x86_64__) || defined(__ia64__)
long get_efi_systab_smbios()
{
  long result = 0;
  vector < string > sysvars;

  if (loadfile("/sys/firmware/efi/systab", sysvars) || loadfile("/proc/efi/systab", sysvars))
    for (unsigned int i = 0; i < sysvars.size(); i++)
  {
    vector < string > variable;

    splitlines(sysvars[i], variable, '=');

    if ((variable[0] == "SMBIOS") && (variable.size() == 2))
    {
      sscanf(variable[1].c_str(), "%lx", &result);
    }
  }

  return result;
}


static bool scan_dmi_devmem(hwNode & n)
{
  unsigned char buf[31];
  int fd = open("/dev/mem",
    O_RDONLY);
  long fp = get_efi_systab_smbios();
  u32 mmoffset = 0;
  void *mmp = NULL;
  bool efi = true;
  u16 dmimaj = 0, dmimin = 0;

  if (fd == -1)
    return false;

  if (fp <= 0)
  {
    efi = false;
    fp = 0xE0000L;                                /* default value for non-EFI capable platforms */
  }

  fp -= 16;
  while (efi || (fp < 0xFFFE0))
  {
    fp += 16;
    mmoffset = fp % getpagesize();
    mmp = mmap(0, mmoffset + 0x20, PROT_READ, MAP_SHARED, fd, fp - mmoffset);
    memset(buf, 0, sizeof(buf));
    if (mmp != MAP_FAILED)
    {
      memcpy(buf, (u8 *) mmp + mmoffset, sizeof(buf));
      munmap(mmp, mmoffset + 0x20);
    }
    if (mmp == MAP_FAILED)
    {
      close(fd);
      return false;
    }
    uint32_t len;
    uint64_t base;
    if (smbios_entry_point(buf, sizeof(buf), n, dmimaj, dmimin, len, base))
    {
      u8 *dmi_buf = (u8 *)malloc(len);
      mmoffset = base % getpagesize();
      mmp = mmap(0, mmoffset + len, PROT_READ, MAP_SHARED, fd, base - mmoffset);
      if (mmp == MAP_FAILED)
      {
        free(dmi_buf);
        return false;
      }
      memcpy(dmi_buf, (u8 *) mmp + mmoffset, len);
      munmap(mmp, mmoffset + len);
      dmi_table(dmi_buf, len, n, dmimaj, dmimin);
      free(dmi_buf);
      break;
    }

    if (efi)
      break;                                    // we don't need to search the memory for EFI systems
  }
  close(fd);

  return true;
}
#endif // defined(__i386__) || defined(__x86_64__) || defined(__ia64__)


bool scan_dmi(hwNode & n)
{
  if (scan_dmi_sysfs(n))
    return true;
#if defined(__i386__) || defined(__x86_64__) || defined(__ia64__)
  if (scan_dmi_devmem(n))
    return true;
#endif
  return false;
}
