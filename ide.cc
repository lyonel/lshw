#include "cpuinfo.h"
#include "osutils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <vector>

#define PROC_IDE "/proc/ide"

#define PCI_SLOT(devfn)         (((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)         ((devfn) & 0x07)

/* device types */
/* ------------ */
#define NO_DEV                  0xffff
#define ATA_DEV                 0x0000
#define ATAPI_DEV               0x0001

/* word definitions */
/* ---------------- */
#define GEN_CONFIG		0	/* general configuration */
#define LCYLS			1	/* number of logical cylinders */
#define CONFIG			2	/* specific configuration */
#define LHEADS			3	/* number of logical heads */
#define TRACK_BYTES		4	/* number of bytes/track (ATA-1) */
#define SECT_BYTES		5	/* number of bytes/sector (ATA-1) */
#define LSECTS			6	/* number of logical sectors/track */
#define START_SERIAL            10	/* ASCII serial number */
#define LENGTH_SERIAL           10	/* 10 words (20 bytes or characters) */
#define BUF_TYPE		20	/* buffer type (ATA-1) */
#define BUF_SIZE		21	/* buffer size (ATA-1) */
#define RW_LONG			22	/* extra bytes in R/W LONG cmd ( < ATA-4) */
#define START_FW_REV            23	/* ASCII firmware revision */
#define LENGTH_FW_REV		 4	/*  4 words (8 bytes or characters) */
#define START_MODEL    		27	/* ASCII model number */
#define LENGTH_MODEL    	20	/* 20 words (40 bytes or characters) */
#define SECTOR_XFER_MAX	        47	/* r/w multiple: max sectors xfered */
#define DWORD_IO		48	/* can do double-word IO (ATA-1 only) */
#define CAPAB_0			49	/* capabilities */
#define CAPAB_1			50
#define PIO_MODE		51	/* max PIO mode supported (obsolete) */
#define DMA_MODE		52	/* max Singleword DMA mode supported (obs) */
#define WHATS_VALID		53	/* what fields are valid */
#define LCYLS_CUR		54	/* current logical cylinders */
#define LHEADS_CUR		55	/* current logical heads */
#define LSECTS_CUR	        56	/* current logical sectors/track */
#define CAPACITY_LSB		57	/* current capacity in sectors */
#define CAPACITY_MSB		58
#define SECTOR_XFER_CUR		59	/* r/w multiple: current sectors xfered */
#define LBA_SECTS_LSB		60	/* LBA: total number of user */
#define LBA_SECTS_MSB		61	/*      addressable sectors */
#define SINGLE_DMA		62	/* singleword DMA modes */
#define MULTI_DMA		63	/* multiword DMA modes */
#define ADV_PIO_MODES		64	/* advanced PIO modes supported */
				    /*
				     * multiword DMA xfer cycle time: 
				     */
#define DMA_TIME_MIN		65	/*   - minimum */
#define DMA_TIME_NORM		66	/*   - manufacturer's recommended   */
				    /*
				     * minimum PIO xfer cycle time: 
				     */
#define PIO_NO_FLOW		67	/*   - without flow control */
#define PIO_FLOW		68	/*   - with IORDY flow control */
#define PKT_REL			71	/* typical #ns from PKT cmd to bus rel */
#define SVC_NBSY		72	/* typical #ns from SERVICE cmd to !BSY */
#define CDR_MAJOR		73	/* CD ROM: major version number */
#define CDR_MINOR		74	/* CD ROM: minor version number */
#define QUEUE_DEPTH		75	/* queue depth */
#define MAJOR			80	/* major version number */
#define MINOR			81	/* minor version number */
#define CMDS_SUPP_0		82	/* command/feature set(s) supported */
#define CMDS_SUPP_1		83
#define CMDS_SUPP_2		84
#define CMDS_EN_0		85	/* command/feature set(s) enabled */
#define CMDS_EN_1		86
#define CMDS_EN_2		87
#define ULTRA_DMA		88	/* ultra DMA modes */
				    /*
				     * time to complete security erase 
				     */
#define ERASE_TIME		89	/*   - ordinary */
#define ENH_ERASE_TIME		90	/*   - enhanced */
#define ADV_PWR			91	/* current advanced power management level
					 * in low byte, 0x40 in high byte. */
#define PSWD_CODE		92	/* master password revision code    */
#define HWRST_RSLT		93	/* hardware reset result */
#define ACOUSTIC  		94	/* acoustic mgmt values ( >= ATA-6) */
#define LBA_LSB			100	/* LBA: maximum.  Currently only 48 */
#define LBA_MID			101	/*      bits are used, but addr 103 */
#define LBA_48_MSB		102	/*      has been reserved for LBA in */
#define LBA_64_MSB		103	/*      the future. */
#define RM_STAT 		127	/* removable media status notification feature set support */
#define SECU_STATUS		128	/* security status */
#define CFA_PWR_MODE		160	/* CFA power mode 1 */
#define START_MEDIA             176	/* media serial number */
#define LENGTH_MEDIA            20	/* 20 words (40 bytes or characters) */
#define START_MANUF             196	/* media manufacturer I.D. */
#define LENGTH_MANUF            10	/* 10 words (20 bytes or characters) */
#define INTEGRITY		255	/* integrity word */

#define NOT_ATA                 0x8000
#define NOT_ATAPI               0x4000	/* (check only if bit 15 == 1) */
#define MEDIA_REMOVABLE         0x0080
#define CFA_SUPPORT_VAL         0x848a	/* 848a=CFA feature set support */

#define EQPT_TYPE               0x1f00
#define IORDY_SUP               0x0800	/* 1=support; 0=may be supported */
#define IORDY_OFF               0x0400	/* 1=may be disabled */
#define LBA_SUP                 0x0200	/* 1=Logical Block Address support */
#define DMA_SUP                 0x0100	/* 1=Direct Memory Access support */
#define DMA_IL_SUP              0x8000	/* 1=interleaved DMA support (ATAPI) */
#define CMD_Q_SUP               0x4000	/* 1=command queuing support (ATAPI) */
#define OVLP_SUP                0x2000	/* 1=overlap operation support (ATAPI) */

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

static string print_ascii(const char *base,
			  int length)
{
  string result = "";

  for (int i = 0; i < length - 1; i++)
  {
    result += base[i + 1];
    result += base[i];
    i++;
  }
  return result;
}

static bool probe_ide(const string & name,
		      hwNode & device)
{
  string filename =
    string(PROC_IDE) + string("/") + name + string("/identify");
  FILE *in = fopen(filename.c_str(), "r");
  u_int16_t iddata[INTEGRITY + 1];
  int i;

  if (!in)
    return false;

  memset(iddata, 0, sizeof(iddata));
  for (i = 0; i <= INTEGRITY; i++)
    if (fscanf(in, "%x", &iddata[i]) != 1)
      break;
  fclose(in);

  if (i <= INTEGRITY)
    return false;		// couldn't read the whole structure

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

  if (iddata[GEN_CONFIG] & MEDIA_REMOVABLE)
    device.addCapability("removable");
  if (iddata[START_MODEL])
    device.
      setProduct(print_ascii((char *) &iddata[START_MODEL], LENGTH_MODEL));
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

  return true;
}

bool scan_ide(hwNode & n)
{
  struct dirent **namelist;
  int nentries;

  pushd(PROC_IDE);
  nentries = scandir(".", &namelist, selectdir, NULL);
  popd();

  if (nentries < 0)
    return false;

  for (int i = 0; i < nentries; i++)
  {
    vector < string > config;
    hwNode ide("bus",
	       hw::storage);

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
	ndevices = scandir(".", &devicelist, selectdir, NULL);
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

	  probe_ide(devicelist[j]->d_name, idedevice);

	  ide.addChild(idedevice);
	  free(devicelist[j]);
	}
	free(devicelist);

	ide.setLogicalName(namelist[i]->d_name);
	ide.setHandle("IDE:" + string(namelist[i]->d_name));

	if (identify[0] == "pci" && identify.size() == 11)
	{
	  string pciid = get_pciid(identify[2], identify[4]);
	  hwNode *parent = n.findChildByHandle(pciid);

	  ide.setDescription("Channel " + identify[10]);

	  if (parent)
	  {
	    parent->claim();
	    ide.setProduct(parent->getProduct());
	    ide.setVendor(parent->getVendor());
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

static char *id = "@(#) $Id: ide.cc,v 1.3 2003/02/05 14:09:07 ezix Exp $";
