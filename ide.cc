#include "cpuinfo.h"
#include "osutils.h"
#include "cdrom.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <endian.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <linux/hdreg.h>

#define PROC_IDE "/proc/ide"

#define PCI_SLOT(devfn)         (((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)         ((devfn) & 0x07)

#define IORDY_SUP               0x0800	/* 1=support; 0=may be supported */
#define IORDY_OFF               0x0400	/* 1=may be disabled */
#define LBA_SUP                 0x0200	/* 1=Logical Block Address support */
#define DMA_SUP                 0x0100	/* 1=Direct Memory Access support */
#define DMA_IL_SUP              0x8000	/* 1=interleaved DMA support (ATAPI) */
#define CMD_Q_SUP               0x4000	/* 1=command queuing support (ATAPI) */
#define OVLP_SUP                0x2000	/* 1=overlap operation support (ATAPI) */

#define GEN_CONFIG              0	/* general configuration */
#define CD_ROM                  5
#define NOT_ATA                 0x8000
#define NOT_ATAPI               0x4000	/* (check only if bit 15 == 1) */
#define EQPT_TYPE               0x1f00

static const char *description[] = {
  "Direct-access device",	/* word 0, bits 12-8 = 00 */
  "Sequential-access device",	/* word 0, bits 12-8 = 01 */
  "Printer",			/* word 0, bits 12-8 = 02 */
  "Processor",			/* word 0, bits 12-8 = 03 */
  "Write-once device",		/* word 0, bits 12-8 = 04 */
  "CD-ROM",			/* word 0, bits 12-8 = 05 */
  "Scanner",			/* word 0, bits 12-8 = 06 */
  "Optical memory",		/* word 0, bits 12-8 = 07 */
  "Medium changer",		/* word 0, bits 12-8 = 08 */
  "Communications device",	/* word 0, bits 12-8 = 09 */
  "ACS-IT8 device",		/* word 0, bits 12-8 = 0a */
  "ACS-IT8 device",		/* word 0, bits 12-8 = 0b */
  "Array controller",		/* word 0, bits 12-8 = 0c */
  "Enclosure services",		/* word 0, bits 12-8 = 0d */
  "Reduced block command device",	/* word 0, bits 12-8 = 0e */
  "Optical card reader/writer",	/* word 0, bits 12-8 = 0f */
  "",				/* word 0, bits 12-8 = 10 */
  "",				/* word 0, bits 12-8 = 11 */
  "",				/* word 0, bits 12-8 = 12 */
  "",				/* word 0, bits 12-8 = 13 */
  "",				/* word 0, bits 12-8 = 14 */
  "",				/* word 0, bits 12-8 = 15 */
  "",				/* word 0, bits 12-8 = 16 */
  "",				/* word 0, bits 12-8 = 17 */
  "",				/* word 0, bits 12-8 = 18 */
  "",				/* word 0, bits 12-8 = 19 */
  "",				/* word 0, bits 12-8 = 1a */
  "",				/* word 0, bits 12-8 = 1b */
  "",				/* word 0, bits 12-8 = 1c */
  "",				/* word 0, bits 12-8 = 1d */
  "",				/* word 0, bits 12-8 = 1e */
  "Unknown",			/* word 0, bits 12-8 = 1f */
};

static unsigned long long get_longlong(const string & path)
{
  FILE *in = fopen(path.c_str(), "r");
  unsigned long long l = 0;

  if (in)
  {
    fscanf(in, "%lld", &l);
    fclose(in);
  }

  return l;
}

static int selectdir(const struct dirent *d)
{
  struct stat buf;

  if (d->d_name[0] == '.')
    return 0;

  if (lstat(d->d_name, &buf) != 0)
    return 0;

  return S_ISDIR(buf.st_mode);
}

static string get_pciid(const string & bus,
			const string & device)
{
  char buffer[20];
  int pcibus, pcidevfunc;

  sscanf(bus.c_str(), "%x", &pcibus);
  sscanf(device.c_str(), "%x", &pcidevfunc);
  snprintf(buffer, sizeof(buffer), "PCI:%02x:%02x.%x", pcibus,
	   PCI_SLOT(pcidevfunc), PCI_FUNC(pcidevfunc));

  return string(buffer);
}

static bool probe_ide(const string & name,
		      hwNode & device)
{
  struct hd_driveid id;
  const u_int8_t *id_regs = (const u_int8_t *) &id;
  int fd = open(device.getLogicalName().c_str(), O_RDONLY | O_NONBLOCK);

  if (fd < 0)
    return false;

  memset(&id, 0, sizeof(id));
  if (ioctl(fd, HDIO_GET_IDENTITY, &id) != 0)
  {
    close(fd);
    return false;
  }

  u_int8_t args[4 + 512] = { WIN_IDENTIFY, 0, 0, 1, };
  if (ioctl(fd, HDIO_DRIVE_CMD, &args) != 0)
  {
    args[0] = WIN_PIDENTIFY;
    if (ioctl(fd, HDIO_DRIVE_CMD, &args) != 0)
    {
      close(fd);
      return false;
    }
  }

  close(fd);

  u_int16_t pidentity[256];
  for (int i = 0; i < 256; i++)
    pidentity[i] = args[4 + 2 * i] + (args[4 + 2 * i + 1] << 8);

  if (id.model[0])
    device.setProduct(hw::strip(string((char *) id.model, sizeof(id.model))));
  if (id.fw_rev[0])
    device.
      setVersion(hw::strip(string((char *) id.fw_rev, sizeof(id.fw_rev))));
  if (id.serial_no[0])
    device.
      setSerial(hw::
		strip(string((char *) id.serial_no, sizeof(id.serial_no))));

  if (!(pidentity[GEN_CONFIG] & NOT_ATA))
  {
    device.addCapability("ata");
    device.setDescription("ATA Disk");
  }
  else if (!(pidentity[GEN_CONFIG] & NOT_ATAPI))
  {
    u_int8_t eqpt = (pidentity[GEN_CONFIG] & EQPT_TYPE) >> 8;
    device.addCapability("atapi");

    if (eqpt == CD_ROM)
      device.addCapability("cdrom");

    if (eqpt < 0x20)
      device.setDescription(description[eqpt]);
  }
  if (id.config & (1 << 7))
    device.addCapability("removable");
  if (id.config & (1 << 15))
    device.addCapability("nonmagnetic");
  if (id.capability & 1)
    device.addCapability("dma");
  if (id.capability & 2)
    device.addCapability("lba");
  if (id.capability & 8)
    device.addCapability("iordy");
  if (id.command_set_1 & 1)
    device.addCapability("smart");
  if (id.command_set_1 & 2)
    device.addCapability("security");
  if (id.command_set_1 & 4)
    device.addCapability("removable");
  if (id.command_set_1 & 8)
    device.addCapability("pm");
  if (id.command_set_2 & 8)
    device.addCapability("apm");

  if ((id.capability & 8) || (id.field_valid & 2))
  {
    if (id.field_valid & 4)
    {
      if (id.dma_ultra & 0x100)
	device.setConfig("mode", "udma0");
      if (id.dma_ultra & 0x200)
	device.setConfig("mode", "udma1");
      if (id.dma_ultra & 0x400)
	device.setConfig("mode", "udma2");
#ifdef __NEW_HD_DRIVE_ID
      if (id.hw_config & 0x2000)
#else
      if (id.word93 & 0x2000)
#endif
      {
	if (id.dma_ultra & 0x800)
	  device.setConfig("mode", "udma3");
	if (id.dma_ultra & 0x1000)
	  device.setConfig("mode", "udma4");
	if (id.dma_ultra & 0x2000)
	  device.setConfig("mode", "udma5");
	if (id.dma_ultra & 0x4000)
	  device.setConfig("mode", "udma6");
	if (id.dma_ultra & 0x8000)
	  device.setConfig("mode", "udma7");
      }
    }
  }

  if (id_regs[83] & 8)
    device.addCapability("apm");

  //if (device.isCapable("iordy") && (id.capability & 4))
  //device.setConfig("iordy", "yes");

  if (device.isCapable("smart"))
  {
    if (id.command_set_2 & (1 << 14))
      device.setConfig("smart", "on");
    else
      device.setConfig("smart", "off");
  }

  if (device.isCapable("apm"))
  {
    if (!(id_regs[86] & 8))
      device.setConfig("apm", "off");
    else
      device.setConfig("apm", "on");
  }

  if (device.isCapable("lba"))
    device.setSize((unsigned long long) id.lba_capacity * 512);
  if (device.isCapable("removable"))
    device.setSize(0);		// we'll first have to make sure we have a disk

  if (device.isCapable("cdrom"))
    scan_cdrom(device);

#if 0
  if (!(iddata[GEN_CONFIG] & NOT_ATA))
    device.addCapability("ata");
  else if (iddata[GEN_CONFIG] == CFA_SUPPORT_VAL)
  {
    device.addCapability("ata");
    device.addCapability("compactflash");
  }
  else if (!(iddata[GEN_CONFIG] & NOT_ATAPI))
  {
    device.addCapability("atapi");
    device.setDescription(description[(iddata[GEN_CONFIG] & EQPT_TYPE) >> 8]);
  }

  if (iddata[START_MODEL])
    device.setProduct(print_ascii((char *) &iddata[START_MODEL],
				  LENGTH_MODEL));
  if (iddata[START_SERIAL])
    device.
      setSerial(print_ascii((char *) &iddata[START_SERIAL], LENGTH_SERIAL));
  if (iddata[START_FW_REV])
    device.
      setVersion(print_ascii((char *) &iddata[START_FW_REV], LENGTH_FW_REV));
  if (iddata[CAPAB_0] & LBA_SUP)
    device.addCapability("lba");
  if (iddata[CAPAB_0] & IORDY_SUP)
    device.addCapability("iordy");
  if (iddata[CAPAB_0] & IORDY_OFF)
  {
    device.addCapability("iordy");
    device.addCapability("iordyoff");
  }
  if (iddata[CAPAB_0] & DMA_SUP)
    device.addCapability("dma");
  if (iddata[CAPAB_0] & DMA_IL_SUP)
  {
    device.addCapability("interleaved_dma");
    device.addCapability("dma");
  }
  if (iddata[CAPAB_0] & CMD_Q_SUP)
    device.addCapability("command_queuing");
  if (iddata[CAPAB_0] & OVLP_SUP)
    device.addCapability("overlap_operation");
#endif
  return true;
}

bool scan_ide(hwNode & n)
{
  struct dirent **namelist;
  int nentries;

  pushd(PROC_IDE);
  nentries = scandir(".", &namelist, selectdir, alphasort);
  popd();

  if (nentries < 0)
    return false;

  for (int i = 0; i < nentries; i++)
  {
    vector < string > config;
    hwNode ide("ide",
	       hw::storage);

    ide.setLogicalName(namelist[i]->d_name);
    ide.setHandle("IDE:" + string(namelist[i]->d_name));

    if (loadfile
	(string(PROC_IDE) + "/" + namelist[i]->d_name + "/config", config))
    {
      vector < string > identify;

      splitlines(config[0], identify, ' ');
      config.clear();

      if (identify.size() >= 1)
      {
	struct dirent **devicelist;
	int ndevices;

	pushd(string(PROC_IDE) + "/" + namelist[i]->d_name);
	ndevices = scandir(".", &devicelist, selectdir, alphasort);
	popd();

	for (int j = 0; j < ndevices; j++)
	{
	  string basepath =
	    string(PROC_IDE) + "/" + namelist[i]->d_name + "/" +
	    devicelist[j]->d_name;
	  hwNode idedevice("device",
			   hw::storage);

	  idedevice =
	    hwNode(get_string(basepath + "/media", "disk"), hw::storage);

	  idedevice.setSize(512 * get_longlong(basepath + "/capacity"));
	  idedevice.setLogicalName(string("/dev/") + devicelist[j]->d_name);
	  idedevice.setProduct(get_string(basepath + "/model"));
	  idedevice.claim();
	  idedevice.setHandle(ide.getHandle() + ":" +
			      string(devicelist[j]->d_name));

	  probe_ide(devicelist[j]->d_name, idedevice);

	  ide.addChild(idedevice);
	  free(devicelist[j]);
	}
	free(devicelist);

	if (identify[0] == "pci" && identify.size() == 11)
	{
	  string pciid = get_pciid(identify[2], identify[4]);
	  hwNode *parent = n.findChildByHandle(pciid);

	  ide.setDescription(hw::strip("Channel " + hw::strip(identify[10])));

	  if (parent)
	  {
	    parent->claim();
	    ide.setClock(parent->getClock());
	    parent->addChild(ide);
	  }
	}
      }

    }

    free(namelist[i]);
  }
  free(namelist);

  return false;
}

static char *id = "@(#) $Id: ide.cc,v 1.11 2003/02/07 12:09:40 ezix Exp $";
