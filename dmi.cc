#include "dmi.h"

#include <map>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

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

static unsigned long dmi_cache_size(u16 n)
{
  if (n & (1 << 15))
    return (n & 0x7FFF) * 64 * 1024;	// value is in 64K blocks
  else
    return (n & 0x7FFF) * 1024;	// value is in 1K blocks
}

static void dmi_decode_cache(u16 c)
{
  if (c & (1 << 0))
    printf(" Other");
  if (c & (1 << 1))
    printf(" Unknown");
  if (c & (1 << 2))
    printf(" Non-burst");
  if (c & (1 << 3))
    printf(" Burst");
  if (c & (1 << 4))
    printf(" Pipeline burst");
  if (c & (1 << 5))
    printf(" Synchronous");
  if (c & (1 << 6))
    printf(" Asynchronous");
}

static const char *dmi_bus_name(u8 num)
{
  static const char *bus[] = {
    "",
    "",
    "",
    "ISA ",
    "MCA ",
    "EISA ",
    "PCI ",
    "PCMCIA ",
    "VLB ",
    "Proprietary ",
    "CPU Slot ",
    "Proprietary RAM ",
    "I/O Riser ",
    "NUBUS ",
    "PCI-66 ",
    "AGP ",
    "AGP 2x ",
    "AGP 4x "
  };
  static const char *jpbus[] = {
    "PC98/C20",
    "PC98/C24",
    "PC98/E",
    "PC98/LocalBus",
    "PC98/Card"
  };

  if (num <= 0x11)
    return bus[num];
  if (num >= 0xA0 && num <= 0xA4)
    return jpbus[num - 0xA0];
  return "";
}

static const char *dmi_bus_width(u8 code)
{
  static const char *width[] = {
    "",
    "",
    "",
    "8bit ",
    "16bit ",
    "32bit ",
    "64bit ",
    "128bit "
  };
  if (code > 7)
    return "";
  return width[code];
}

static const char *dmi_card_size(u8 v)
{
  if (v == 3)
    return ("Short ");
  if (v == 4)
    return ("Long ");
  return "";
}

static void dmi_card_props(u8 v)
{
  printf("\t\tSlot Features: ");
  if (v & (1 << 1))
    printf("5v ");
  if (v & (1 << 2))
    printf("3.3v ");
  if (v & (1 << 3))
    printf("Shared ");
  if (v & (1 << 4))
    printf("PCCard16 ");
  if (v & (1 << 5))
    printf("CardBus ");
  if (v & (1 << 6))
    printf("Zoom-Video ");
  if (v & (1 << 7))
    printf("ModemRingResume ");
  printf("\n");
}

static const char *dmi_chassis_type(u8 code)
{
  static const char *chassis_type[] = {
    "",
    "Other",
    "Unknown",
    "Desktop",
    "Low Profile Desktop",
    "Pizza Box",
    "Mini Tower",
    "Tower",
    "Portable",
    "Laptop",
    "Notebook",
    "Hand Held",
    "Docking Station",
    "All in One",
    "Sub Notebook",
    "Space-saving",
    "Lunch Box",
    "Main Server Chassis",
    "Expansion Chassis",
    "SubChassis",
    "Bus Expansion Chassis",
    "Peripheral Chassis",
    "RAID Chassis",
    "Rack Mount Chassis",
    "Sealed-case PC",
  };
  code &= ~0x80;

  if (code > 0x18)
    return "";
  return chassis_type[code];

}

static const char *dmi_port_connector_type(u8 code)
{
  static const char *connector_type[] = {
    "None",
    "Centronics",
    "Mini Centronics",
    "Proprietary",
    "DB-25 pin male",
    "DB-25 pin female",
    "DB-15 pin male",
    "DB-15 pin female",
    "DB-9 pin male",
    "DB-9 pin female",
    "RJ-11",
    "RJ-45",
    "50 Pin MiniSCSI",
    "Mini-DIN",
    "Micro-DIN",
    "PS/2",
    "Infrared",
    "HP-HIL",
    "Access Bus (USB)",
    "SSA SCSI",
    "Circular DIN-8 male",
    "Circular DIN-8 female",
    "On Board IDE",
    "On Board Floppy",
    "9 Pin Dual Inline (pin 10 cut)",
    "25 Pin Dual Inline (pin 26 cut)",
    "50 Pin Dual Inline",
    "68 Pin Dual Inline",
    "On Board Sound Input from CD-ROM",
    "Mini-Centronics Type-14",
    "Mini-Centronics Type-26",
    "Mini-jack (headphones)",
    "BNC",
    "1394",
    "PC-98",
    "PC-98Hireso",
    "PC-H98",
    "PC-98Note",
    "PC98Full",
  };

  if (code == 0xFF)
    return "Other";

  if (code <= 0x21)
    return connector_type[code];

  if ((code >= 0xA0) && (code <= 0xA4))
    return connector_type[code - 0xA0 + 0x22];

  return "";
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

static char *dmi_memory_array_use(u8 num)
{
  static char *memory_array_use[] = {
    "",
    "Other",
    "Unknown",
    "System memory",
    "Video memory",
    "Flash memory",
    "Non-volatile RAM",
    "Cache memory",
  };
  if (num > 0x07)
    return "";
  return memory_array_use[num];
};

static char *dmi_memory_array_error_correction_type(u8 num)
{
  static char *memory_array_error_correction_type[] = {
    "",
    "Other",
    "Unknown",
    "None",
    "Parity",
    "Single-bit ECC",
    "Multi-bit ECC",
    "CRC",
  };
  if (num > 0x07)
    return "";
  return memory_array_error_correction_type[num];
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

static const char *dmi_port_type(u8 code)
{
  static const char *port_type[] = {
    "None",
    "Parallel Port XT/AT Compatible",
    "Parallel Port PS/2",
    "Parallel Port ECP",
    "Parallel Port EPP",
    "Parallel Port ECP/EPP",
    "Serial Port XT/AT Compatible",
    "Serial Port 16450 Compatible",
    "Serial Port 16650 Compatible",
    "Serial Port 16650A Compatible",
    "SCSI Port",
    "MIDI Port",
    "Joy Stick Port",
    "Keyboard Port",
    "Mouse Port",
    "SSA SCSI",
    "USB",
    "FireWire (IEEE P1394)",
    "PCMCIA Type I",
    "PCMCIA Type II",
    "PCMCIA Type III",
    "Cardbus",
    "Access Bus Port",
    "SCSI II",
    "SCSI Wide",
    "PC-98",
    "PC-98-Hireso",
    "PC-H98",
    "Video Port",
    "Audio Port",
    "Modem Port",
    "Network Port",
    "8251 Compatible",
    "8251 FIFO Compatible",
  };

  if (code == 0xFF)
    return "Other";

  if (code <= 0x1F)
    return port_type[code];

  if ((code >= 0xA0) && (code <= 0xA1))
    return port_type[code - 0xA0 + 0x20];

  return "";
}

static const char *dmi_processor_type(u8 code)
{
  static const char *processor_type[] = {
    "",
    "Other",
    "Unknown",
    "Central Processor",
    "Math Processor",
    "DSP Processor",
    "Video Processor"
  };

  if (code == 0xFF)
    return "Other";

  if (code > 6)
    return "";
  return processor_type[code];
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

static const char *dmi_onboard_type(u8 code)
{
  static const char *onboard_type[] = {
    "",
    "Other",
    "Unknown",
    "Video",
    "SCSI Controller",
    "Ethernet",
    "Token Ring",
    "Sound",
  };
  code &= ~0x80;
  if (code > 7)
    return "";
  return onboard_type[code];
}

static const char *dmi_mgmt_dev_type(u8 code)
{
  static const char *type[] = {
    "",
    "Other",
    "Unknown",
    "LM75",
    "LM78",
    "LM79",
    "LM80",
    "LM81",
    "ADM9240",
    "DS1780",
    "MAX1617",
    "GL518SM",
    "W83781D",
    "HT82H791",
  };

  if (code > 0x0d)
    return "";
  return type[code];
}

static const char *dmi_mgmt_addr_type(u8 code)
{
  static const char *type[] = {
    "",
    "Other",
    "Unknown",
    "I/O",
    "Memory",
    "SMBus",
  };

  if (code > 5)
    return "";
  return type[code];
}

static const char *dmi_fan_type(u8 code)
{
  static const char *type[] = {
    "",
    "Other",
    "Unknown",
    "Fan",
    "Centrifugal Blower",
    "Chip Fan",
    "Cabinet Fan",
    "Power Supply Fan",
    "Heat Pipe",
    "Integrated Refrigeration",
    "",
    "",
    "",
    "",
    "",
    "",
    "Active Cooling",
    "Passive Cooling",
  };

  if (code > 0x11)
    return "";
  return type[code];
}

static const char *dmi_volt_loc(u8 code)
{
  static const char *type[] = {
    "",
    "Other",
    "Unknown",
    "Processor",
    "Disk",
    "Peripheral Bay",
    "System Management Module",
    "Motherboard",
    "Memory Module",
    "Processor Module",
    "Power Unit",
    "Add-in Card",
  };

  if (code > 0x0b)
    return "";
  return type[code];
}

static const char *dmi_temp_loc(u8 code)
{
  static const char *type[] = {
    "Front Panel Board",
    "Back Panel Board",
    "Power System Board",
    "Drive Back Plane",
  };

  if (code <= 0x0b)
    return dmi_volt_loc(code);
  if (code <= 0x0f)
    return type[code - 0x0c];
  return "";
}

static const char *dmi_status(u8 code)
{
  static const char *type[] = {
    "",
    "Other",
    "Unknown",
    "OK",
    "Non-critical",
    "Critical",
    "Non-recoverable",
  };

  if (code > 6)
    return "";
  return type[code];
}

/* 3 dec. places */
static const char *dmi_millivolt(u8 * data,
				 int indx)
{
  static char buffer[20];
  short int d;

  if (data[indx + 1] == 0x80 && data[indx] == 0)
    return "Unknown";
  d = data[indx + 1] << 8 | data[indx];
  sprintf(buffer, "%0.3f", d / 1000.0);
  return buffer;
}

/* 2 dec. places */
static const char *dmi_accuracy(u8 * data,
				int indx)
{
  static char buffer[20];
  short int d;

  if (data[indx + 1] == 0x80 && data[indx] == 0)
    return "Unknown";
  d = data[indx + 1] << 8 | data[indx];
  sprintf(buffer, "%0.2f", d / 100.0);
  return buffer;
}

/* 1 dec. place */
static const char *dmi_temp(u8 * data,
			    int indx)
{
  static char buffer[20];
  short int d;

  if (data[indx + 1] == 0x80 && data[indx] == 0)
    return "Unknown";
  d = data[indx + 1] << 8 | data[indx];
  sprintf(buffer, "%0.1f", d / 10.0);
  return buffer;
}

/* 0 dec. place */
static const char *dmi_speed(u8 * data,
			     int indx)
{
  static char buffer[20];
  short int d;

  if (data[indx + 1] == 0x80 && data[indx] == 0)
    return "Unknown";
  d = data[indx + 1] << 8 | data[indx];
  sprintf(buffer, "%d", d);
  return buffer;
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

    hardwarenode = node.getChild("hardware");
    if (!hardwarenode)
    {
      node.addChild(hwNode("hardware", hw::bus));
      hardwarenode = node.getChild("hardware");
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
	hwNode newnode("bios",
		       hw::memory,
		       dmi_string(dm,
				  data[4]));
	newnode.setVersion(dmi_string(dm, data[5]));
	newnode.setCapacity(64 * data[9] * 1024);
	newnode.setSize(16 * (0x10000 - (data[7] << 8 | data[6])));
	newnode.setDescription("BIOS");

	dmi_bios_features(data[13] << 24 | data[12] << 16 | data[11] << 8 |
			  data[10],
			  data[17] << 24 | data[16] << 16 | data[15] << 8 |
			  data[14], newnode);

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
	  hardwarenode->setDescription(dmi_board_type(data[0x0D]));
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
	newnode.setProduct(dmi_processor_family(data[6]));
	newnode.setVersion(dmi_string(dm, data[0x10]));
	newnode.setVendor(dmi_string(dm, data[7]));
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
	  //printf("\t\tAsset Tag: %s\n", dmi_string(dm, data[0x21]));
	  if (dmi_string(dm, data[0x22]) != "")
	    newnode.setProduct(newnode.getProduct() + " (" +
			       string(dmi_string(dm, data[0x22])) + ")");
	}
	//printf("\t\tProcessor Type: %s\n", dmi_processor_type(data[5]));

	// external clock
	u = data[0x13] << 8 | data[0x12];
	newnode.setClock(u * 1000000);
	// maximum speed
	u = data[0x15] << 8 | data[0x14];
	newnode.setCapacity(u * 1000000);
	// current speed
	u = data[0x17] << 8 | data[0x16];
	newnode.setSize(u * 1000000);

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
	hwNode newnode("ram",
		       hw::memory);

	newnode.setHandle(handle);

	size = data[0x0E] * (1 << data[8]) * 1024 * 1024;
	newnode.setCapacity(size);

	// loop through the controller's slots and link them to us
	for (i = 0; i < data[0x0E]; i++)
	{
	  char slotref[10];
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
	hwNode *controllernode = NULL;
	unsigned long long clock = 0;
	unsigned long long capacity = 0;
	unsigned long long size = 0;

	newnode.setSlot(dmi_string(dm, data[4]).c_str());
	if (data[6])
	  clock = 1000000000 / data[6];	// convert value from ns to Hz
	newnode.setClock(clock);
	newnode.setProduct(dmi_decode_ram(data[8] << 8 | data[7]));
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

	/*
	 * 
	 * if(links.find(hw::strip(handle)) != links.end())
	 * controllernode = node.findChildByHandle(links[hw::strip(handle)]);
	 * 
	 * if(!controllernode)
	 * controllernode = node.findChildByHandle("DMI:FAKE:RAM");
	 * 
	 * if(!controllernode)
	 * {
	 * hwNode *memorynode = node.getChild("memory");
	 * 
	 * if (!memorynode)
	 * memorynode = node.addChild(hwNode("memory", hw::memory));
	 * 
	 * if(memorynode)
	 * {
	 * hwNode fakecontrollernode("ram", hw::memory, "Memory Controller");
	 * 
	 * fakecontrollernode.setHandle("DMI:FAKE:RAM");
	 * controllernode = memorynode->addChild(fakecontrollernode);
	 * }
	 * }
	 * else
	 * if(controllernode)
	 * controllernode->addChild(newnode);
	 */

	hardwarenode->addChild(newnode);
      }
      break;
    case 7:
      // Cache
      {
	hwNode newnode("cache",
		       hw::memory);
	int level;

	static const char *types[4] = {
	  "Internal ", "External ",
	  "", ""
	};
	static const char *modes[4] = {
	  "write-through",
	  "write-back",
	  "", ""
	};

	newnode.setSlot(dmi_string(dm, data[4]));
	u = data[6] << 8 | data[5];
	level = 1 + (u & 7);
	printf("\t\tL%d %s%sCache: ",
	       1 + (u & 7), (u & (1 << 3)) ? "socketed " : "",
	       types[(u >> 5) & 3]);
	if (u & (1 << 7))
	  printf("%s\n", modes[(u >> 8) & 3]);
	else
	  printf("disabled\n");
	printf("\t\tL%d Cache Size: ", 1 + (u & 7));
	newnode.setSize(dmi_cache_size(data[7] | data[8] << 8));
	newnode.setCapacity(dmi_cache_size(data[9] | data[10] << 8));
	if (newnode.getCapacity() < newnode.getSize())
	  newnode.setCapacity(newnode.getCapacity() * 64);
	printf("\t\tL%d Cache Type: ", 1 + (u & 7));
	dmi_decode_cache(data[13]);
	printf("\n");

	newnode.setHandle(handle);

	hardwarenode->addChild(newnode);
      }
      break;

    case 8:
      printf("\tPort Connector\n");
      printf("\t\tInternal Designator: %s\n",
	     dmi_string(dm, data[4]).c_str());
      printf("\t\tInternal Connector Type: %s\n",
	     dmi_port_connector_type(data[5]));
      printf("\t\tExternal Designator: %s\n",
	     dmi_string(dm, data[6]).c_str());
      printf("\t\tExternal Connector Type: %s\n",
	     dmi_port_connector_type(data[7]));
      printf("\t\tPort Type: %s\n", dmi_port_type(data[8]));
      break;

    case 9:
      printf("\tCard Slot\n");
      printf("\t\tSlot: %s\n", dmi_string(dm, data[4]).c_str());
      printf("\t\tType: %s%s%s\n",
	     dmi_bus_width(data[6]),
	     dmi_card_size(data[8]), dmi_bus_name(data[5]));
      if (data[7] == 3)
	printf("\t\tStatus: Available.\n");
      if (data[7] == 4)
	printf("\t\tStatus: In use.\n");
      if (data[11] & 0xFE)
	dmi_card_props(data[11]);
      break;

    case 10:
      printf("\tOn Board Devices Information\n");
      for (u = 2; u * 2 + 1 < dm->length; u++)
      {
	printf("\t\tDescription: %s : %s\n",
	       dmi_string(dm, data[1 + 2 * u]).c_str(),
	       (data[2 * u]) & 0x80 ? "Enabled" : "Disabled");
	printf("\t\tType: %s\n", dmi_onboard_type(data[2 * u]));

      }

      break;

    case 11:
      // OEM Data
      break;
    case 12:
      // Configuration Information
      break;

    case 13:
      printf("\tBIOS Language Information\n");
      printf("\t\tInstallable Languages: %u\n", data[4]);
      for (u = 1; u <= data[4]; u++)
      {
	printf("\t\t\t%s\n", dmi_string(dm, u).c_str());
      }
      printf("\t\tCurrently Installed Language: %s\n",
	     dmi_string(dm, data[21]).c_str());
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
	  id = "ram";
	  break;
	case 0x04:
	  id = "video";
	  break;
	case 0x05:
	  id = "flash";
	  break;
	case 0x06:
	  id = "nvram";
	  break;
	case 0x07:
	  id = "cache";
	  break;
	default:
	  id = "genericmemory";
	}

	hwNode newnode(id,
		       hw::memory);

	newnode.setHandle(handle);

	newnode.setSlot(dmi_memory_array_location(data[4]));
	printf("\t\tError Correction Type: %s\n",
	       dmi_memory_array_error_correction_type(data[6]));
	u2 = data[10] << 24 | data[9] << 16 | data[8] << 8 | data[7];
	if (u2 != 0x80000000)	// magic value for "unknown"
	  newnode.setCapacity(u2 * 1024);
	printf("\t\tNumber of Devices: %u\n", data[14] << 8 | data[13]);

	hwNode *memorynode = node.getChild("memory");
	if (!memorynode)
	{
	  hardwarenode->addChild(hwNode("memory", hw::memory));
	  memorynode = node.getChild("memory");
	}

	if (memorynode)
	  memorynode->addChild(newnode);
      }
      break;
    case 17:
      // Memory Device
      {
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
	    snprintf(buffer, sizeof(buffer), "%u MHz (%.1f ns)", u,
		     (1000.0 / u));
	  description += " " + string(buffer);
	}

	hwNode newnode("bank",
		       hw::memory);

	newnode.setHandle(handle);
	newnode.setSlot(slot);

	if (dm->length > 23)
	  newnode.setVendor(dmi_string(dm, data[23]));
	if (dm->length > 24)
	  newnode.setSerial(dmi_string(dm, data[24]));
	if (dm->length > 26)
	  newnode.setProduct(dmi_string(dm, data[26]));

	if (strlen(bits))
	  description += " " + string(bits) + " bits";

	newnode.setDescription(description);
	newnode.setSize(size);
	newnode.setClock(clock);

	if (size == 0)
	  newnode.setProduct("");

	hardwarenode->addChild(newnode);
      }
      break;
    case 18:
      // 32-bit Memory Error Information
      break;
    case 19:
      // Memory Array Mapped Address
      break;
    case 20:
      // Memory Device Mapped Address
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
  int fd = open("/dev/mem", O_RDONLY);
  long fp = 0xE0000L;
  u8 smmajver = 0, smminver = 0;

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
      u16 dmimaj, dmimin;

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

  return true;
}
