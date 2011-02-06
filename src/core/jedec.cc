#include "jedec.h"
#include "hw.h"
#include "osutils.h"
#include <regex.h>
#include <string>
#include <iostream>

using namespace std;

static const char * jedec_id[] = {
	"01",	"AMD",
	"8001",	"AMD",
	"02",	"AMI",
	"8002",	"AMI",
	"04",	"Fujitsu",
	"8004",	"Fujitsu",
	"0500",	"Elpida",
	"07",	"Hitachi",
	"8007",	"Hitachi",
	"08",	"Inmos",
	"8008",	"Inmos",
	"0B",	"Intersil",
	"800B",	"Intersil",
	"0D",	"Mostek",
	"800D",	"Mostek",
	"0E",	"Freescale (Motorola)",
	"800E",	"Freescale (Motorola)",
	"10",	"NEC",
	"8010",	"NEC",
	"13",	"Conexant (Rockwell)",
	"8013",	"Conexant (Rockwell)",
	"15",	"NXP (Philips Semi, Signetics)",
	"8015",	"NXP (Philips Semi, Signetics)",
	"16",	"Synertek",
	"8016",	"Synertek",
	"19",	"Xicor",
	"8019",	"Xicor",
	"1A",	"Zilog",
	"801A",	"Zilog",
	"1C",	"Mitsubishi",
	"801C",	"Mitsubishi",
	"2C",	"Micron Technology",
	"802C",	"Micron Technology",
	"40",	"ProMOS/Mosel Vitelic",
	"8040",	"ProMOS/Mosel Vitelic",
	"6A",	"Goldenram",
	"806A",	"Goldenram",
	"7F7F7F0B",	"Nanya Technology",
	"830B",	"Nanya Technology",
	"83",	"Fairchild",
	"8083",	"Fairchild",
	"89",	"Numonyx (Intel)",
	"8089",	"Numonyx (Intel)",
	"91",	"DATARAM",
	"8091",	"DATARAM",
	"98",	"Toshiba",
	"8098",	"Toshiba",
	"A4",	"IBM",
	"80A4",	"IBM",
	"AD",	"Hynix Semiconductor (Hyundai Electronics)",
	"80AD",	"Hynix Semiconductor (Hyundai Electronics)",
	"C1",	"Infineon (Siemens)",
	"80C1",	"Infineon (Siemens)",
	"7F7F7F7F7F51",	"Infineon (Siemens)",
	"CE",	"Samsung",
	"80CE",	"Samsung",
	"DE",	"Winbond Electronics",
	"80DE",	"Winbond Electronics",
	"E0",	"LG Semiconductor (Goldstar)",
	"80E0",	"LG Semiconductor (Goldstar)",
	"7F45",	"Micron CMS",
	"7F98",	"Kingston",
	"7F7F7F7F43", "Ramaxel Technology",
	NULL,	NULL
};

string jedec_resolve(const string & s)
{
  string result = hw::strip(s);

  if(matches(result, "^0x"))
    result.erase(0, 2);
  if(!matches(result, "^[0-9a-fA-F]+$")) return s;

  for(int i=0; jedec_id[i]; i+=2) {
    if(matches(result, "^" + string(jedec_id[i]), REG_ICASE)) return string(jedec_id[i+1]);
  }

  return s;
}
