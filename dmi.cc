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

#include "dmi.h"

#include <map>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

static char *id = "@(#) $Id: dmi.cc,v 1.70 2003/10/13 09:57:00 ezix Exp $";

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct dmi_header
{
  u8 type;
  u8 length;
  u16 handle;
};

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
  return string(bp);
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
  static const char *boardtypes[] = {
    "",
    "",
    "",
    "Server Blade",
    "Connectivitiy Switch",
    "System Management Module",
    "Processor Module",
    "I/O Module",
    "Memory Module",
    "Daughter Board",
    "Mother Board",
    "Processor/Memory Module",
    "Processor/IO Module ",
    "Interconnect Board",
  };
  if (data > 0x0D)
    return "";
  else
    return boardtypes[data];
}

static void dmi_bios_features(u32 data1,
			      u32 data2,
			      hwNode & bios)
{
  if (data1 & (1 << 3))		// BIOS characteristics not supported
    return;

  if (data1 & (1 << 4))		// ISA
    bios.addCapability("ISA");
  if (data1 & (1 << 5))		// MCA
    bios.addCapability("MCA");
  if (data1 & (1 << 6))		// EISA
    bios.addCapability("EISA");
  if (data1 & (1 << 7))		// PCI
    bios.addCapability("PCI");
  if (data1 & (1 << 8))		// PCMCIA
    bios.addCapability("PCMCIA");
  if (data1 & (1 << 9))		// PNP
    bios.addCapability("PNP");
  if (data1 & (1 << 10))	// APM
    bios.addCapability("APM");
  if (data1 & (1 << 11))	// flashable BIOS
    bios.addCapability("upgrade");
  if (data1 & (1 << 12))	// BIOS shadowing
    bios.addCapability("shadowing");
  if (data1 & (1 << 13))	// VL-VESA
    bios.addCapability("VESA");
  if (data1 & (1 << 14))	// ESCD
    bios.addCapability("ESCD");
  if (data1 & (1 << 15))	// boot from CD
    bios.addCapability("cdboot");
  if (data1 & (1 << 16))	// selectable boot
    bios.addCapability("bootselect");
  if (data1 & (1 << 17))	// BIOS ROM is socketed
    bios.addCapability("socketedrom");
  if (data1 & (1 << 18))	// PCMCIA boot
    bios.addCapability("pcmciaboot");
  if (data1 & (1 << 19))	// Enhanced Disk Drive
    bios.addCapability("EDD");
  if (data1 & (1 << 20))	// NEC 9800 floppy
    bios.addCapability("int13floppynec");
  if (data1 & (1 << 21))	// Toshiba floppy
    bios.addCapability("int13floppytoshiba");
  if (data1 & (1 << 22))	// 5.25" 360KB floppy
    bios.addCapability("int13floppy360");
  if (data1 & (1 << 23))	// 5.25" 1.2MB floppy
    bios.addCapability("int13floppy1200");
  if (data1 & (1 << 24))	// 3.5" 720KB floppy
    bios.addCapability("int13floppy720");
  if (data1 & (1 << 25))	// 3.5" 2.88MB floppy
    bios.addCapability("int13floppy2880");
  if (data1 & (1 << 26))	// print screen key
    bios.addCapability("int5printscreen");
  if (data1 & (1 << 27))	// 8042 kbd controller
    bios.addCapability("int9keyboard");
  if (data1 & (1 << 28))	// serial line control
    bios.addCapability("int14serial");
  if (data1 & (1 << 29))	// printer
    bios.addCapability("int17printer");
  if (data1 & (1 << 30))	// CGA/Mono video
    bios.addCapability("int10video");
  if (data1 & (1 << 31))	// NEC PC-98
    bios.addCapability("pc98");
}

static void dmi_bios_features_ext(u8 * data,
				  int len,
				  hwNode & bios)
{
  if (len < 1)
    return;

  if (data[0] & (1 << 0))	// ACPI
    bios.addCapability("ACPI");
  if (data[0] & (1 << 1))	// USB
    bios.addCapability("USB");
  if (data[0] & (1 << 2))	// AGP
    bios.addCapability("AGP");
  if (data[0] & (1 << 3))	// I2O boot
    bios.addCapability("I2Oboot");
  if (data[0] & (1 << 4))	// LS-120 boot
    bios.addCapability("LS120boot");
  if (data[0] & (1 << 5))	// ATAPI ZIP boot
    bios.addCapability("ZIPboot");
  if (data[0] & (1 << 6))	// 1394 boot
    bios.addCapability("IEEE1394boot");
  if (data[0] & (1 << 7))	// smart battery
    bios.addCapability("smartbattery");

  if (len < 1)
    return;

  if (data[1] & (1 << 0))	// BIOS boot specification
    bios.addCapability("biosbootspecification");
  if (data[1] & (1 << 1))	// function-key initiated network service boot
    bios.addCapability("netboot");
}

static unsigned long dmi_cache_size(u16 n)
{
  if (n & (1 << 15))
    return (n & 0x7FFF) * 64 * 1024;	// value is in 64K blocks
  else
    return (n & 0x7FFF) * 1024;	// value is in 1K blocks
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
    n.addCapability("non-burst");
  if (c & (1 << 3))
    n.addCapability("burst");
  if (c & (1 << 4))
    n.addCapability("pipeline-burst");
  if (c & (1 << 5))
    n.addCapability("synchronous");
  if (c & (1 << 6))
    n.addCapability("asynchronous");
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
    n.addCapability("internal");
    break;
  case 1:
    n.addCapability("external");
    break;
  }
  snprintf(buffer, sizeof(buffer), "L%d ", (config & 7) + 1);
  result += " " + string(buffer);

  switch ((config >> 8) & 3)
  {
  case 0:
    n.addCapability("write-through");
    break;
  case 1:
    n.addCapability("write-back");
    break;
  }

  result += "cache";

  switch (cachetype)
  {
  case 3:
    n.addCapability("instruction");
    break;
  case 4:
    n.addCapability("data");
    break;
  }

  n.setDescription(hw::strip(result));
}

static char *dmi_memory_array_location(u8 num)
{
  static char *memory_array_location[] = {
    "",
    "",
    "",
    "System board or motherboard",
    "ISA add-on card",
    "EISA add-on card",
    "PCI add-on card",
    "MCA add-on card",
    "PCMCIA add-on card",
    "Proprietary add-on card",
    "NuBus",
  };
  static char *jp_memory_array_location[] = {
    "PC-98/C20 add-on card",
    "PC-98/C24 add-on card",
    "PC-98/E add-on card",
    "PC-98/Local bus add-on card",
  };
  if (num <= 0x0A)
    return memory_array_location[num];
  if (num >= 0xA0 && num < 0xA3)
    return jp_memory_array_location[num];
  return "";
}

static char *dmi_memory_device_form_factor(u8 num)
{
  static char *memory_device_form_factor[] = {
    "",
    "",
    "",
    " SIMM",
    " SIP",
    " Chip",
    " DIP",
    " ZIP",
    " Proprietary Card",
    " DIMM",
    " TSOP",
    " Row of chips",
    " RIMM",
    " SODIMM",
    " SRIMM",
  };
  if (num > 0x0E)
    return "";
  return memory_device_form_factor[num];
}

static char *dmi_memory_device_type(u8 num)
{
  static char *memory_device_type[] = {
    "",
    "",
    "",
    " DRAM",
    " EDRAM",
    " VRAM",
    " SRAM",
    " RAM",
    " ROM",
    " FLASH",
    " EEPROM",
    " FEPROM",
    " EPROM",
    " CDRAM",
    " 3DRAM",
    " SDRAM",
    " SGRAM",
    " RDRAM",
  };
  if (num > 0x11)
    return "";
  return memory_device_type[num];
}

static string dmi_memory_device_detail(u16 v)
{
  string result = "";

  if (v & (1 << 3))
    result += " Fast-paged";
  if (v & (1 << 4))
    result += " Static column";
  if (v & (1 << 5))
    result += " Pseudo-static";
  if (v & (1 << 6))
    result += " RAMBUS";
  if (v & (1 << 7))
    result += " Synchronous";
  if (v & (1 << 8))
    result += " CMOS";
  if (v & (1 << 9))
    result += " EDO";
  if (v & (1 << 10))
    result += " Window DRAM";
  if (v & (1 << 11))
    result += " Cache DRAM";
  if (v & (1 << 12))
    result += " Non-volatile";

  return result;
}

static const char *dmi_processor_family(u8 code)
{
  static const char *processor_family[] = {
    "",
    "Other",
    "Unknown",
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
    "",
    "",
    "",
    "",
    "Duron",
    "K5",
    "K6",
    "K6-2",
    "K6-3",
    "Athlon",
    "AMD2900",
    "K6-2+",
    "Power PC",
    "Power PC 601",
    "Power PC 603",
    "Power PC 603+",
    "Power PC 604",
    "Power PC 620",
    "Power PC x704",
    "Power PC 750",
  };

  if (code == 0xFF)
    return "Other";

  if (code > 0x24)
    return "";
  return processor_family[code];
}

static string dmi_handle(u16 handle)
{
  char buffer[10];
  snprintf(buffer, sizeof(buffer), "DMI:%04X", handle);

  return string(buffer);
}

static void dmi_table(int fd,
		      u32 base,
		      int len,
		      int num,
		      hwNode & node,
		      int dmiversionmaj,
		      int dmiversionmin)
{
  unsigned char *buf = (unsigned char *) malloc(len);
  struct dmi_header *dm;
  hwNode *hardwarenode = NULL;
  u8 *data;
  int i = 0;
  int r = 0, r2 = 0;
  string handle;

  if (len == 0)
    // no data
    return;

  if (buf == NULL)
    // memory exhausted
    return;

  if (lseek(fd, (long) base, SEEK_SET) == -1)
    // i/o error
    return;

  while (r2 != len && (r = read(fd, buf + r2, len - r2)) != 0)
    r2 += r;
  if (r == 0)
    // i/o error
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
	newnode.setDescription("BIOS");
	newnode.claim();

	dmi_bios_features(data[13] << 24 | data[12] << 16 | data[11] << 8 |
			  data[10],
			  data[17] << 24 | data[16] << 16 | data[15] << 8 |
			  data[14], newnode);

	if (dm->length > 0x12)
	  dmi_bios_features_ext(&data[0x12], dm->length - 0x12, newnode);

	if (release != "")
	  newnode.setVersion(newnode.getVersion() + " (" + release + ")");
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
      break;

    case 2:
      // Board Information Block
      {

	if ((dm->length <= 0x0E) || (data[0x0E] == 0))	// we are the only system board on the computer so connect everything to us
	{
	  hardwarenode->setVendor(dmi_string(dm, data[4]));
	  hardwarenode->setProduct(dmi_string(dm, data[5]));
	  hardwarenode->setVersion(dmi_string(dm, data[6]));
	  hardwarenode->setSerial(dmi_string(dm, data[7]));
	  hardwarenode->setSlot(dmi_string(dm, data[0x0A]));
	  hardwarenode->setHandle(handle);
	  hardwarenode->setDescription("Motherboard");
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
      break;

    case 4:
      // Processor
      {
	hwNode newnode("cpu",
		       hw::processor);

	newnode.setSlot(dmi_string(dm, data[4]));
	newnode.setDescription("CPU");
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
			   " Memory Controller");

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

	newnode.setSlot(dmi_string(dm, data[4]).c_str());
	if (data[6])
	  clock = 1000000000 / data[6];	// convert value from ns to Hz
	newnode.setClock(clock);
	newnode.setDescription(dmi_decode_ram(data[8] << 8 | data[7]));
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

	newnode.setSize(dmi_cache_size(data[9] | data[10] << 8));
	newnode.setCapacity(dmi_cache_size(data[7] | (data[8] << 8)));
	if ((dm->length > 0x0F) && (data[0x0F] != 0))
	{
	  newnode.setClock(1000000000 / data[0x0F]);	// convert from ns to Hz
	}

	newnode.setHandle(handle);
	newnode.setPhysId(dm->handle);
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
	string id = "";
	string description = "";
	switch (data[5])
	{
	case 0x03:
	  description = "System Memory";
	  break;
	case 0x04:
	  description = "Video Memory";
	  break;
	case 0x05:
	  description = "Flash Memory";
	  break;
	case 0x06:
	  description = "NVRAM";
	  break;
	case 0x07:
	  description = "Cache Memory";
	  break;
	default:
	  description = "Generic Memory";
	}

	hwNode newnode("memory",
		       hw::memory);
	newnode.setHandle(handle);
	newnode.setPhysId(dm->handle);
	newnode.setDescription(description);
	newnode.setSlot(dmi_memory_array_location(data[4]));
	//printf("\t\tError Correction Type: %s\n",
	//dmi_memory_array_error_correction_type(data[6]));
	u2 = data[10] << 24 | data[9] << 16 | data[8] << 8 | data[7];
	if (u2 != 0x80000000)	// magic value for "unknown"
	  newnode.setCapacity(u2 * 1024);
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
	    snprintf(bits, sizeof(bits), "%d", u);
	  else
	    snprintf(bits, sizeof(bits), "%d/%d", width, u);
	}
	else
	{
	  if (width != 0)
	    snprintf(bits, sizeof(bits), "%d", width);
	}

	// size
	u = data[13] << 8 | data[12];
	if (u != 0xffff)
	  size = (1024 * (u & 0x7fff) * ((u & 0x8000) ? 1 : 1024));
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
	  clock = u * 1000000;	// u is a frequency in MHz
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
	if (strlen(bits))
	  newnode.setConfig("width", hw::strip(bits));
	newnode.setDescription(description);
	newnode.setSize(size);
	newnode.setClock(clock);
	hwNode *memoryarray = hardwarenode->findChildByHandle(arrayhandle);
	if (memoryarray)
	  memoryarray->addChild(newnode);
	else
	{
	  hwNode ramnode("memory",
			 hw::memory);
	  ramnode.addChild(newnode);
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
	if (end - start < 512)	// memory range is smaller thant 512KB
	{
	  // consider that values were expressed in megagytes
	  start *= 1024;
	  end *= 1024;
	}

	newnode.setStart(start * 1024);	// values are in KB
	if (end != start)
	  newnode.setSize((end - start + 1) * 1024);	// values are in KB
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
	if (end - start < 512)	// memory range is smaller than 512KB
	{
	  // consider that values were expressed in megagytes
	  start *= 1024;
	  end *= 1024;
	}

	newnode.setStart(start * 1024);	// values are in KB
	if (end != start)
	  newnode.setSize((end - start + 1) * 1024);	// values are in KB
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
      break;
    case 23:
      // System Reset
      break;
    case 24:
      // Hardware Security
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
      break;
    case 31:
      // Boot Integrity Services Entry Point
      break;
    case 32:
      // System Boot Information
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
  free(buf);
}

bool scan_dmi(hwNode & n)
{
  unsigned char buf[20];
  int fd = open("/dev/mem",
		O_RDONLY);
  long fp = 0xE0000L;
  u8 smmajver = 0, smminver = 0;
  u16 dmimaj = 0, dmimin = 0;
  if (sizeof(u8) != 1 || sizeof(u16) != 2 || sizeof(u32) != 4)
    // compiler incompatibility
    return false;
  if (fd == -1)
    return false;
  if (lseek(fd, fp, SEEK_SET) == -1)
  {
    close(fd);
    return false;
  }

  fp -= 16;
  while (fp < 0xFFFFF)
  {
    fp += 16;
    if (read(fd, buf, 16) != 16)
    {
      close(fd);
      return false;
    }
    else if (memcmp(buf, "_SM_", 4) == 0)
    {
      // SMBIOS
      smmajver = buf[6];
      smminver = buf[7];
    }
    else if (memcmp(buf, "_DMI_", 5) == 0)
    {
      u16 num = buf[13] << 8 | buf[12];
      u16 len = buf[7] << 8 | buf[6];
      u32 base = buf[11] << 24 | buf[10] << 16 | buf[9] << 8 | buf[8];
      dmimaj = buf[14] ? buf[14] >> 4 : smmajver;
      dmimin = buf[14] ? buf[14] & 0x0F : smminver;
      dmi_table(fd, base, len, num, n, dmimaj, dmimin);
      /*
       * dmi_table moved us far away 
       */
      lseek(fd, fp + 16, SEEK_SET);
    }
  }
  close(fd);
  if (smmajver != 0)
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "smbios-%d.%d", smmajver, smminver);
    n.addCapability(string(buffer));
  }
  if (dmimaj != 0)
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "dmi-%d.%d", dmimaj, dmimin);
    n.addCapability(string(buffer));
  }

  (void) &id;			// avoid "id defined but not used" warning

  return true;
}
