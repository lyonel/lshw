#include "dmi.h"

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

static char *dmi_string(struct dmi_header *dm,
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
  return bp;
}

static void dmi_decode_ram(u8 data)
{
  if (data & (1 << 0))
    printf("OTHER ");
  if (data & (1 << 1))
    printf("UNKNOWN ");
  if (data & (1 << 2))
    printf("STANDARD ");
  if (data & (1 << 3))
    printf("FPM ");
  if (data & (1 << 4))
    printf("EDO ");
  if (data & (1 << 5))
    printf("PARITY ");
  if (data & (1 << 6))
    printf("ECC ");
  if (data & (1 << 7))
    printf("SIMM ");
  if (data & (1 << 8))
    printf("DIMM ");
  if (data & (1 << 9))
    printf("Burst EDO ");
  if (data & (1 << 10))
    printf("SDRAM ");
}

static void dmi_cache_size(u16 n)
{
  if (n & (1 << 15))
    printf("%dK\n", (n & 0x7FFF) * 64);
  else
    printf("%dK\n", n & 0x7FFF);
}

static void dmi_decode_cache(u16 c)
{
  if (c & (1 << 0))
    printf("Other ");
  if (c & (1 << 1))
    printf("Unknown ");
  if (c & (1 << 2))
    printf("Non-burst ");
  if (c & (1 << 3))
    printf("Burst ");
  if (c & (1 << 4))
    printf("Pipeline burst ");
  if (c & (1 << 5))
    printf("Synchronous ");
  if (c & (1 << 6))
    printf("Asynchronous ");
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
    "Other",
    "Unknown",
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
    "PC-98/Local buss add-on card",
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
    "Other",
    "Unknown",
    "SIMM",
    "SIP",
    "Chip",
    "DIP",
    "ZIP",
    "Proprietary Card",
    "DIMM",
    "TSOP",
    "Row of chips",
    "RIMM",
    "SODIMM",
    "SRIMM",
  };
  if (num > 0x0E)
    return "";
  return memory_device_form_factor[num];
}

static char *dmi_memory_device_type(u8 num)
{
  static char *memory_device_type[] = {
    "",
    "Other",
    "Unknown",
    "DRAM",
    "EDRAM",
    "VRAM",
    "SRAM",
    "RAM",
    "ROM",
    "FLASH",
    "EEPROM",
    "FEPROM",
    "EPROM",
    "CDRAM",
    "3DRAM",
    "SDRAM",
    "SGRAM",
    "RDRAM",
  };
  if (num > 0x11)
    return "";
  return memory_device_type[num];
}

static string dmi_memory_device_detail(u16 v)
{
  string result = " ";

  if (v & (1 << 3))
    result += "Fast-paged ";
  if (v & (1 << 4))
    result += "Static column ";
  if (v & (1 << 5))
    result += "Pseudo-static ";
  if (v & (1 << 6))
    result += "RAMBUS ";
  if (v & (1 << 7))
    result += "Synchronous ";
  if (v & (1 << 8))
    result += "CMOS ";
  if (v & (1 << 9))
    result += "EDO ";
  if (v & (1 << 10))
    result += "Window DRAM ";
  if (v & (1 << 11))
    result += "Cache DRAM ";
  if (v & (1 << 12))
    result += "Non-volatile ";

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
    "Intel386 processor",
    "Intel486 processor",
    "8087",
    "80287",
    "80387",
    "80487",
    "Pentium processor Family",
    "Pentium Pro processor",
    "Pentium II processor",
    "Pentium processor with MMX technology",
    "Celeron processor",
    "Pentium II Xeon processor",
    "Pentium III processor",
    "M1 Family",
    "M1", "M1", "M1", "M1", "M1", "M1",	/* 13h - 18h */
    "K5 Family",
    "K5", "K5", "K5", "K5", "K5", "K5",	/* 1Ah - 1Fh */
    "Power PC Family",
    "Power PC 601",
    "Power PC 603",
    "Power PC 603+",
    "Power PC 604",
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

static void dmi_table(int fd,
		      u32 base,
		      int len,
		      int num,
		      hwNode & node)
{
  unsigned char *buf = (unsigned char *) malloc(len);
  struct dmi_header *dm;
  u8 *data;
  int i = 0;
  int r = 0, r2 = 0;

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

    switch (dm->type)
    {
    case 0:
      // BIOS Information Block
      {
	string release(dmi_string(dm,
				  data[8]));
	hwNode newnode("rom",
		       hw::memory,
		       dmi_string(dm,
				  data[4]));
	newnode.setVersion(dmi_string(dm, data[5]));
	newnode.setSize(64 * data[9] * 1024);

	if (release != "")
	  newnode.setVersion(newnode.getVersion() + " (" + release + ")");
	node.addChild(newnode);
      }
      break;

    case 1:
      // System Information Block
      node.setVendor(dmi_string(dm, data[4]));
      node.setProduct(dmi_string(dm, data[5]));
      node.setVersion(dmi_string(dm, data[6]));
      node.setSerial(dmi_string(dm, data[7]));
      break;

    case 2:
      // Board Information Block
      //
      // special case: if the system characteristics are still unknown,
      // use values from the motherboard
      if (node.getVendor() == "")
	node.setVendor(dmi_string(dm, data[4]));
      if (node.getProduct() == "")
	node.setProduct(dmi_string(dm, data[5]));
      if (node.getVersion() == "")
	node.setVersion(dmi_string(dm, data[6]));
      if (node.getSerial() == "")
	node.setSerial(dmi_string(dm, data[7]));
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
      printf("\tProcessor\n");
      printf("\t\tSocket Designation: %s\n", dmi_string(dm, data[4]));
      printf("\t\tProcessor Type: %s\n", dmi_processor_type(data[5]));
      printf("\t\tProcessor Family: %s\n", dmi_processor_family(data[6]));
      printf("\t\tProcessor Manufacturer: %s\n", dmi_string(dm, data[7]));
      printf("\t\tProcessor Version: %s\n", dmi_string(dm, data[0x10]));
      if (dm->length <= 0x20)
	break;
      printf("\t\tSerial Number: %s\n", dmi_string(dm, data[0x20]));
      printf("\t\tAsset Tag: %s\n", dmi_string(dm, data[0x21]));
      printf("\t\tVendor Part Number: %s\n", dmi_string(dm, data[0x22]));
      break;

    case 5:
      printf("\tMemory Controller\n");
      break;

    case 6:
      printf("\tMemory Bank\n");
      printf("\t\tSocket: %s\n", dmi_string(dm, data[4]));
      if (data[5] != 0xFF)
      {
	printf("\t\tBanks: ");
	if ((data[5] & 0xF0) != 0xF0)
	  printf("%d ", data[5] >> 4);
	if ((data[5] & 0x0F) != 0x0F)
	  printf("%d", data[5] & 0x0F);
	printf("\n");
      }
      if (data[6])
	printf("\t\tSpeed: %dnS\n", data[6]);
      printf("\t\tType: ");
      dmi_decode_ram(data[8] << 8 | data[7]);
      printf("\n");
      printf("\t\tInstalled Size: ");
      switch (data[9] & 0x7F)
      {
      case 0x7D:
	printf("Unknown");
	break;
      case 0x7E:
	printf("Disabled");
	break;
      case 0x7F:
	printf("Not Installed");
	break;
      default:
	printf("%dMbyte", (1 << (data[9] & 0x7F)));
      }
      if (data[9] & 0x80)
	printf(" (Double sided)");
      printf("\n");
      printf("\t\tEnabled Size: ");
      switch (data[10] & 0x7F)
      {
      case 0x7D:
	printf("Unknown");
	break;
      case 0x7E:
	printf("Disabled");
	break;
      case 0x7F:
	printf("Not Installed");
	break;
      default:
	printf("%dMbyte", (1 << (data[10] & 0x7F)));
      }
      if (data[10] & 0x80)
	printf(" (Double sided)");
      printf("\n");
      if ((data[11] & 4) == 0)
      {
	if (data[11] & (1 << 0))
	  printf("\t\t*** BANK HAS UNCORRECTABLE ERRORS (BIOS DISABLED)\n");
	if (data[11] & (1 << 1))
	  printf("\t\t*** BANK LOGGED CORRECTABLE ERRORS AT BOOT\n");
      }
      break;
    case 7:
      {
	static const char *types[4] = {
	  "Internal ", "External ",
	  "", ""
	};
	static const char *modes[4] = {
	  "write-through",
	  "write-back",
	  "", ""
	};

	printf("\tCache\n");
	printf("\t\tSocket: %s\n", dmi_string(dm, data[4]));
	u = data[6] << 8 | data[5];
	printf("\t\tL%d %s%sCache: ",
	       1 + (u & 7), (u & (1 << 3)) ? "socketed " : "",
	       types[(u >> 5) & 3]);
	if (u & (1 << 7))
	  printf("%s\n", modes[(u >> 8) & 3]);
	else
	  printf("disabled\n");
	printf("\t\tL%d Cache Size: ", 1 + (u & 7));
	dmi_cache_size(data[7] | data[8] << 8);
	printf("\t\tL%d Cache Maximum: ", 1 + (u & 7));
	dmi_cache_size(data[9] | data[10] << 8);
	printf("\t\tL%d Cache Type: ", 1 + (u & 7));
	dmi_decode_cache(data[13]);
	printf("\n");
      }
      break;

    case 8:
      printf("\tPort Connector\n");
      printf("\t\tInternal Designator: %s\n", dmi_string(dm, data[4]));
      printf("\t\tInternal Connector Type: %s\n",
	     dmi_port_connector_type(data[5]));
      printf("\t\tExternal Designator: %s\n", dmi_string(dm, data[6]));
      printf("\t\tExternal Connector Type: %s\n",
	     dmi_port_connector_type(data[7]));
      printf("\t\tPort Type: %s\n", dmi_port_type(data[8]));
      break;

    case 9:
      printf("\tCard Slot\n");
      printf("\t\tSlot: %s\n", dmi_string(dm, data[4]));
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
	       dmi_string(dm, data[1 + 2 * u]),
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
	printf("\t\t\t%s\n", dmi_string(dm, u));
      }
      printf("\t\tCurrently Installed Language: %s\n",
	     dmi_string(dm, data[21]));
      break;

    case 14:
      // Group Associations
      break;

    case 15:
      // Event Log
      break;

    case 16:
      printf("\tPhysical Memory Array\n");
      printf("\t\tLocation: %s\n", dmi_memory_array_location(data[4]));
      printf("\t\tUse: %s\n", dmi_memory_array_use(data[5]));
      printf("\t\tError Correction Type: %s\n",
	     dmi_memory_array_error_correction_type(data[6]));
      u2 = data[10] << 24 | data[9] << 16 | data[8] << 8 | data[7];
      printf("\t\tMaximum Capacity: ");
      if (u2 == 0x80000000)
	printf("Unknown\n");
      else
	printf("%u Kbyte\n", u2);
      printf("\t\tError Information Handle: ");
      u = data[12] << 8 | data[11];
      if (u == 0xffff)
      {
	printf("None\n");
      }
      else if (u == 0xfffe)
      {
	printf("Not Provided\n");
      }
      else
      {
	printf("0x%04X\n", u);
      }
      printf("\t\tNumber of Devices: %u\n", data[14] << 8 | data[13]);
      break;
    case 17:
      // Memory Device
      {
	string id;
	string description;
	unsigned long long size = 0;
	u16 width = 0;
	char bits[10];

	// printf("\t\tArray Handle: 0x%04X\n", data[5] << 8 | data[4]);

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

	description += " " + string(dmi_memory_device_form_factor(data[14]));
	id = string(dmi_string(dm, data[16]));
	//printf("\t\tBank Locator: %s\n", dmi_string(dm, data[17]));
	description += " " + string(dmi_memory_device_type(data[18]));

	u = data[20] << 8 | data[19];
	if (u & 0x1ffe)
	  description += dmi_memory_device_detail(u);

	if (dm->length > 21)
	{
	  char buffer[80];

	  u = data[22] << 8 | data[21];
	  // speed
	  if (u == 0)
	    strcpy(buffer, "");
	  else
	    snprintf(buffer, sizeof(buffer), "%u MHz (%.1f ns)", u,
		     (1000.0 / u));
	  description += " " + string(buffer);
	}

	hwNode newnode(id,
		       hw::memory);

	if (dm->length > 23)
	  newnode.setVendor(dmi_string(dm, data[23]));
	if (dm->length > 24)
	  newnode.setSerial(dmi_string(dm, data[24]));
	if (dm->length > 25)
	  printf("\t\tAsset Tag: %s\n", dmi_string(dm, data[25]));
	if (dm->length > 26)
	  description += " P/N: " + string(dmi_string(dm, data[26]));

	if (strlen(bits))
	  description += string(bits) + " bits";

	newnode.setProduct(description);
	newnode.setSize(size);

	node.addChild(newnode);
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
      dmi_table(fd, base, len, num, n);

      /*
       * dmi_table moved us far away 
       */
      lseek(fd, fp + 16, SEEK_SET);
    }
  }
  close(fd);

  return true;
}
