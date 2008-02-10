/*
 * pnp.cc
 *
 *
 *
 *
 *
 */
#include "version.h"
#include "pnp.h"

#include <stdlib.h>
#include <string.h>

__ID("@(#) $Id$");

static const char *pnp_vendors[] =
{
  "ABP", "Advansys",
  "ACC", "Accton",
  "ACE", "Accton",
  "ACR", "Acer",
  "ADP", "Adaptec",
  "ADV", "AMD",
  "AIR", "AIR",
  "AMI", "AMI",
  "ASU", "ASUS",
  "ATI", "ATI",
  "ATK", "Allied Telesyn",
  "AZT", "Aztech",
  "BAN", "Banya",
  "BRI", "Boca Research",
  "BUS", "Buslogic",
  "CCI", "Cache Computers Inc.",
  "CHA", "Chase",
  "CMD", "CMD Technology, Inc.",
  "COG", "Cogent",
  "CPQ", "Compaq",
  "CRS", "Crescendo",
  "CSC", "Crystal",
  "CSI", "CSI",
  "CTL", "Creative Labs",
  "DBI", "Digi",
  "DEC", "Digital Equipment",
  "DEL", "Dell",
  "DBK", "Databook",
  "EGL", "Eagle Technology",
  "ELS", "ELSA",
  "ESS", "ESS",
  "FAR", "Farallon",
  "FDC", "Future Domain",
  "HWP", "Hewlett-Packard",
  "IBM", "IBM",
  "INT", "Intel",
  "ISA", "Iomega",
  "MDG", "Madge",
  "MDY", "Microdyne",
  "MET", "Metheus",
  "MIC", "Micronics",
  "MLX", "Mylex",
  "NEC", "NEC",
  "NVL", "Novell",
  "OLC", "Olicom",
  "PRO", "Proteon",
  "RII", "Racal",
  "RTL", "Realtek",
  "SCM", "SCM",
  "SEC", "SAMSUNG",
  "SKD", "SysKonnect",
  "SMC", "SMC",
  "SNI", "Siemens Nixdorf",
  "STL", "Stallion Technologies",
  "SUP", "SupraExpress",
  "SVE", "SVEC",
  "TCC", "Thomas-Conrad",
  "TCI", "Tulip",
  "TCM", "3Com",
  "TCO", "Thomas-Conrad",
  "TEC", "Tecmar",
  "TRU", "Truevision",
  "TOS", "Toshiba",
  "TYN", "Tyan",
  "UBI", "Ungermann-Bass",
  "USC", "UltraStor",
  "VDM", "Vadem",
  "VMI", "Vermont",
  "WDC", "Western Digital",
  "ZDS", "Zeos",
  NULL, NULL
};

const char *vendorname(const char *id)
{
  int i = 0;
  if (id == NULL)
    return "";

  while (pnp_vendors[i])
  {
    if (strncmp(pnp_vendors[i], id, strlen(pnp_vendors[i])) == 0)
      return pnp_vendors[i + 1];
    i += 2;
  }

  return "";
}


hw::hwClass pnp_class(const string & pnpid)
{
  long numericid = 0;

  if (pnpid.substr(0, 3) != "PNP")
    return hw::generic;

  numericid = strtol(pnpid.substr(3).c_str(), NULL, 16);

  if (numericid < 0x0300)
    return hw::system;
  if (numericid >= 0x300 && numericid < 0x0400)
    return hw::input;                             // keyboards
  if (numericid >= 0x400 && numericid < 0x0500)
    return hw::printer;                           // parallel
  if (numericid >= 0x500 && numericid < 0x0600)
    return hw::communication;                     // serial
  if (numericid >= 0x600 && numericid < 0x0800)
    return hw::storage;                           // ide
  if (numericid >= 0x900 && numericid < 0x0A00)
    return hw::display;                           // vga
  if (numericid == 0x802)
    return hw::multimedia;                        // Microsoft Sound System compatible device
  if (numericid >= 0xA00 && numericid < 0x0B00)
    return hw::bridge;                            // isa,pci...
  if (numericid >= 0xE00 && numericid < 0x0F00)
    return hw::bridge;                            // pcmcia
  if (numericid >= 0xF00 && numericid < 0x1000)
    return hw::input;                             // mice
  if (numericid < 0x8000)
    return hw::system;

  if (numericid >= 0x8000 && numericid < 0x9000)
    return hw::network;                           // network
  if (numericid >= 0xA000 && numericid < 0xB000)
    return hw::storage;                           // scsi&cd
  if (numericid >= 0xB000 && numericid < 0xC000)
    return hw::multimedia;                        // multimedia
  if (numericid >= 0xC000 && numericid < 0xD000)
    return hw::communication;                     // modems

  return hw::generic;
}
