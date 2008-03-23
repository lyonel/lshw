#include "version.h"
#include "spd.h"
#include "osutils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <dirent.h>
#include <stdio.h>

__ID("@(#) $Id$");

/* SPD is 2048-bit long */
#define SPD_MAXSIZE (2048/8)
#define SPD_BLKSIZE 0x10

#define TYPE_EDO  0x02
#define TYPE_SDRAM  0x04

#define PROCSENSORS "/proc/sys/dev/sensors"
#define EEPROMPREFIX "eeprom-"

static unsigned char spd[SPD_MAXSIZE];
static bool spd_page_loaded[SPD_MAXSIZE / SPD_BLKSIZE];
static string current_eeprom = "";
static unsigned int current_bank = 0;

static unsigned char get_spd_byte(unsigned int offset)
{
  if ((offset < 0) || (offset >= SPD_MAXSIZE))
    return 0;

  if (!spd_page_loaded[offset / SPD_BLKSIZE])
  {
    char chunkname[10];
    string name = "";
    FILE *in = NULL;

    snprintf(chunkname, sizeof(chunkname), "%02x",
      (offset / SPD_BLKSIZE) * SPD_BLKSIZE);

    name = current_eeprom + "/" + string(chunkname);

    in = fopen(name.c_str(), "r");
    if (in)
    {
      for (int i = 0; i < SPD_BLKSIZE; i++)
        if(fscanf(in, "%d",
          (int *) &spd[i + (offset / SPD_BLKSIZE) * SPD_BLKSIZE]) < 1)
            break;
      fclose(in);
      spd_page_loaded[offset / SPD_BLKSIZE] = true;
    }
    else
      spd_page_loaded[offset / SPD_BLKSIZE] = false;
  }

  return spd[offset];
}


static int selecteeprom(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  if (!S_ISDIR(buf.st_mode))
    return 0;

  return (strncmp(d->d_name, EEPROMPREFIX, strlen(EEPROMPREFIX)) == 0);
}


static hwNode *get_current_bank(hwNode & memory)
{
  char id[20];
  hwNode *result = NULL;

  if ((current_bank == 0) && (result = memory.getChild("bank")))
    return result;

  snprintf(id, sizeof(id), "bank:%d", current_bank);
  result = memory.getChild(id);

  if (!result)
    return memory.addChild(hwNode(id, hw::memory));
  else
    return result;
}


static bool scan_eeprom(hwNode & memory,
string name)
{
  int memory_type = -1;
  char buff[20];
  unsigned char checksum = 0;
  unsigned char rows = 0;
  unsigned char density = 0;
  unsigned long long size = 0;

  current_eeprom = string(PROCSENSORS) + "/" + name;
  memset(spd, 0, sizeof(spd));
  memset(spd_page_loaded, 0, sizeof(spd_page_loaded));

  for (int i = 0; i < 63; i++)
    checksum += get_spd_byte(i);

  if (checksum != get_spd_byte(63))
    return false;

  memory_type = get_spd_byte(0x02);

  hwNode *bank = get_current_bank(memory);

  if (!bank)
    return false;

  switch (memory_type)
  {
    case TYPE_SDRAM:
      bank->setDescription("SDRAM");
      break;
    case TYPE_EDO:
      bank->setDescription("EDO");
      break;
  }

  rows = get_spd_byte(5);
  snprintf(buff, sizeof(buff), "%d", rows);
  bank->setConfig("rows", buff);

  if (bank->getSize() == 0)
  {
    density = get_spd_byte(31);
    for (int j = 0; (j < 8) && (rows > 0); j++)
      if (density & (1 << j))
    {
      rows--;
      size += (4 << j) * 1024 * 1024;             // MB
      density ^= (1 << j);
      if (density == 0)
        size += rows * (4 << j) * 1024 * 1024;
    }
    bank->setSize(size);
  }

  switch (get_spd_byte(11))                       // error detection and correction scheme
  {
    case 0x00:
      bank->setConfig("errordetection", "none");
      break;
    case 0x01:
      bank->addCapability("parity");
      bank->setConfig("errordetection", "parity");
      break;
    case 0x02:
      bank->addCapability("ecc");
      bank->setConfig("errordetection", "ecc");
      break;
  }

  int version = get_spd_byte(62);

  snprintf(buff, sizeof(buff), "spd-%d.%d", (version & 0xF0) >> 4,
    version & 0x0F);
  bank->addCapability(buff);

  return true;
}


static bool scan_eeproms(hwNode & memory)
{
  struct dirent **namelist;
  int n;

  current_bank = 0;

  pushd(PROCSENSORS);
  n = scandir(".", &namelist, selecteeprom, alphasort);
  popd();

  if (n < 0)
    return false;

  for (int i = 0; i < n; i++)
    if (scan_eeprom(memory, namelist[i]->d_name))
      current_bank++;

  return true;
}


bool scan_spd(hwNode & n)
{
  hwNode *memory = n.getChild("core/memory");

  current_bank = 0;

  if (!memory)
  {
    hwNode *core = n.getChild("core");

    if (!core)
    {
      n.addChild(hwNode("core", hw::bus));
      core = n.getChild("core");
    }

    if (core)
    {
      core->addChild(hwNode("memory", hw::memory));
      memory = core->getChild("memory");
    }
  }

  if (memory)
  {
    memory->claim();

    return scan_eeproms(*memory);
  }

  return false;
}
