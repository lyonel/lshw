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
	"C1",	"Infineon (Siemens)",
	"80C1",	"Infineon (Siemens)",
	"7F7F7F7F7F51",	"Infineon (Siemens)",
	"CE",	"Samsung",
	"80CE",	"Samsung",
	"7F98",	"Kingston",
	NULL,	NULL
};

string jedec_resolve(const string & s)
{
  string result = hw::strip(s);

  if(matches(result, "^0x"))
    result.erase(0, 2);

  for(int i=0; jedec_id[i]; i+=2) {
    if(matches(result, jedec_id[i], REG_ICASE)) return string(jedec_id[i+1]);
  }

  return s;
}
