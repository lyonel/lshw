#include "mem.h"
#include "cdrom.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>

#include <string>
#include <map>

#define SG_X "/dev/sg%d"

#ifndef SCSI_IOCTL_GET_PCI
#define SCSI_IOCTL_GET_PCI 0x5387
#endif

#define OPEN_FLAG O_RDONLY

#ifdef SG_GET_RESERVED_SIZE
#define OPEN_FLAG O_RDWR
#else
#define OPEN_FLAG O_RDWR
#endif

#define INQ_REPLY_LEN 96
#define INQ_CMD_CODE 0x12
#define INQ_CMD_LEN 6

typedef struct my_sg_scsi_id
{
  int host_no;			/* as in "scsi<n>" where 'n' is one of 0, 1, 2 etc */
  int channel;
  int scsi_id;			/* scsi id of target device */
  int lun;
  int scsi_type;		/* TYPE_... defined in scsi/scsi.h */
  short h_cmd_per_lun;		/* host (adapter) maximum commands per lun */
  short d_queue_depth;		/* device (or adapter) maximum queue length */
  int unused1;			/* probably find a good use, set 0 for now */
  int unused2;			/* ditto */
}
My_sg_scsi_id;

typedef struct my_scsi_idlun
{
  int mux4;
  int host_unique_id;
}
My_scsi_idlun;

static const char *devices[] = {
  "/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd", "/dev/sde", "/dev/sdf",
  "/dev/sdg", "/dev/sdh", "/dev/sdi", "/dev/sdj", "/dev/sdk", "/dev/sdl",
  "/dev/sdm", "/dev/sdn", "/dev/sdo", "/dev/sdp", "/dev/sdq", "/dev/sdr",
  "/dev/sds", "/dev/sdt", "/dev/sdu", "/dev/sdv", "/dev/sdw", "/dev/sdx",
  "/dev/sdy", "/dev/sdz", "/dev/sdaa", "/dev/sdab", "/dev/sdac", "/dev/sdad",
  "/dev/scd0", "/dev/scd1", "/dev/scd2", "/dev/scd3", "/dev/scd4",
  "/dev/scd5",
  "/dev/scd6", "/dev/scd7", "/dev/scd8", "/dev/scd9", "/dev/scd10",
  "/dev/scd11", "/dev/sr0", "/dev/sr1", "/dev/sr2", "/dev/sr3", "/dev/sr4",
  "/dev/sr5",
  "/dev/sr6", "/dev/sr7", "/dev/sr8", "/dev/sr9", "/dev/sr10", "/dev/sr11",
  "/dev/st0", "/dev/st1", "/dev/st2", "/dev/st3", "/dev/st4", "/dev/st5",
  "/dev/nst0", "/dev/nst1", "/dev/nst2", "/dev/nst3", "/dev/nst4",
  "/dev/nst5",
  "/dev/nosst0", "/dev/nosst1", "/dev/nosst2", "/dev/nosst3", "/dev/nosst4",
  NULL
};

static map < string, string > sg_map;

static string scsi_handle(unsigned int host,
			  int channel = -1,
			  int id = -1,
			  int lun = -1)
{
  char buffer[10];
  string result = "SCSI:";

  snprintf(buffer, sizeof(buffer), "%02d", host);
  result += string(buffer);

  if (channel < 0)
    return result;

  snprintf(buffer, sizeof(buffer), "%02d", channel);
  result += string(":") + string(buffer);

  if (id < 0)
    return result;

  snprintf(buffer, sizeof(buffer), "%02d", id);
  result += string(":") + string(buffer);

  if (lun < 0)
    return result;

  snprintf(buffer, sizeof(buffer), "%02d", lun);
  result += string(":") + string(buffer);

  return result;
}

static const char *scsi_type(int type)
{
  switch (type)
  {
  case 0:
    return "Hard Disk";
  case 1:
    return "Tape";
  case 3:
    return "Processor";
  case 4:
    return "Write-Once Read-Only Memory";
  case 5:
    return "CD-ROM";
  case 6:
    return "Scanner";
  case 7:
    return "Magneto-optical Disk";
  case 8:
    return "Medium Changer";
  case 0xd:
    return "Enclosure";
  default:
    return "";
  }
}

static bool do_inquiry(int sg_fd,
		       hwNode & node)
{
  unsigned char inqCmdBlk[INQ_CMD_LEN] =
    { INQ_CMD_CODE, 0, 0, 0, INQ_REPLY_LEN, 0 };
  unsigned char inqBuff[INQ_REPLY_LEN];
  unsigned char sense_buffer[32];
  char version[5];
  sg_io_hdr_t io_hdr;
  int k;

  if ((ioctl(sg_fd, SG_GET_VERSION_NUM, &k) < 0) || (k < 30000))
    return false;

  memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
  io_hdr.interface_id = 'S';
  io_hdr.cmd_len = sizeof(inqCmdBlk);
  /*
   * io_hdr.iovec_count = 0; 
 *//*
 * * * memset takes care of this 
 */
  io_hdr.mx_sb_len = sizeof(sense_buffer);
  io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
  io_hdr.dxfer_len = INQ_REPLY_LEN;
  io_hdr.dxferp = inqBuff;
  io_hdr.cmdp = inqCmdBlk;
  io_hdr.sbp = sense_buffer;
  io_hdr.timeout = 20000;	/* 20000 millisecs == 20 seconds */
  /*
   * io_hdr.flags = 0; 
 *//*
 * * * take defaults: indirect IO, etc 
 */
  /*
   * io_hdr.pack_id = 0; 
   */
  /*
   * io_hdr.usr_ptr = NULL; 
   */

  if (ioctl(sg_fd, SG_IO, &io_hdr) < 0)
    return false;

  if ((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK)
    return false;

  char *p = (char *) inqBuff;
  int f = (int) *(p + 7);
  unsigned g = (unsigned char) *(p + 1);
  unsigned ansiversion = ((unsigned char) *(p + 2)) & 7;

  node.setVendor(string(p + 8, 8));
  node.setProduct(string(p + 16, 16));
  node.setVersion(string(p + 32, 4));

  node.setConfig("width", "8");
  if (!(f & 0x40))
    node.setConfig("width", "32");
  if (!(f & 0x20))
    node.setConfig("width", "16");
  if (!(f & 0x10))
    node.addCapability("sync");
  if (g & 0x80)
    node.addCapability("removable");

  snprintf(version, sizeof(version), "%d", ansiversion);
  if (ansiversion)
    node.setConfig("ansiversion", version);

  return true;
}

static void scan_devices()
{
  int fd = -1;
  int i = 0;
  My_scsi_idlun m_idlun;

  for (i = 0; devices[i] != NULL; i++)
  {
    fd = open(devices[i], O_RDONLY | O_NONBLOCK);
    if (fd >= 0)
    {
      int bus = -1;
      if (ioctl(fd, SCSI_IOCTL_GET_BUS_NUMBER, &bus) >= 0)
      {
	memset(&m_idlun, 0, sizeof(m_idlun));
	if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &m_idlun) >= 0)
	{
	  sg_map[scsi_handle(bus, (m_idlun.mux4 >> 16) & 0xff,
			     m_idlun.mux4 & 0xff,
			     (m_idlun.mux4 >> 8) & 0xff)] =
	    string(devices[i]);
	}
      }
      close(fd);
    }
  }
}

static void find_logicalname(hwNode & n)
{
  map < string, string >::iterator i = sg_map.begin();

  for (i = sg_map.begin(); i != sg_map.end(); i++)
    if (i->first == n.getHandle())
    {
      n.setLogicalName(i->second);
      n.claim();
      return;
    }
}

static bool scan_sg(int sg,
		    hwNode & n)
{
  char buffer[20];
  int fd = -1;
  My_sg_scsi_id m_id;
  char slot_name[16];
  char host[20];
  hwNode *parent = NULL;
  hwNode *channel = NULL;
  int emulated = 0;

  snprintf(buffer, sizeof(buffer), SG_X, sg);

  fd = open(buffer, OPEN_FLAG | O_NONBLOCK);
  if (fd < 0)
    return false;

  memset(&m_id, 0, sizeof(m_id));
  if (ioctl(fd, SG_GET_SCSI_ID, &m_id) >= 0)
  {
    printf("%d:%d:%d:%d\n", m_id.host_no, m_id.channel, m_id.scsi_id,
	   m_id.lun);
  }
  else
  {
    close(fd);
    return true;		// we failed to get info but still hope we can continue
  }

  snprintf(host, sizeof(host), "scsi%d", m_id.host_no);

  memset(slot_name, 0, sizeof(slot_name));
  if (ioctl(fd, SCSI_IOCTL_GET_PCI, slot_name) >= 0)
  {
    string parent_handle = string("PCI:") + string(slot_name);

    parent = n.findChildByHandle(parent_handle);
  }

  if (!parent)
    parent = n.findChildByLogicalName(string(host));

  if (!parent)
    parent = n.addChild(hwNode("scsi", hw::bus));

  if (!parent)
  {
    close(fd);
    return true;
  }

  parent->setLogicalName(string(host));
  parent->claim();

  ioctl(fd, SG_EMULATED_HOST, &emulated);

  if (emulated)
  {
    parent->setConfig("emulated", "true");
    if (parent->getDescription() == "")
      parent->setDescription("Virtual SCSI adapter");
  }

  channel =
    parent->findChildByHandle(scsi_handle(m_id.host_no, m_id.channel));
  if (!channel)
    channel = parent->addChild(hwNode("channel", hw::storage));

  if (!channel)
  {
    close(fd);
    return true;
  }

  snprintf(buffer, sizeof(buffer), "Channel %d", m_id.channel);
  channel->setDescription(buffer);
  channel->setHandle(scsi_handle(m_id.host_no, m_id.channel));
  channel->claim();

  hwNode device = hwNode("generic");

  switch (m_id.scsi_type)
  {
  case 0:
    device = hwNode("disk", hw::storage);
    break;
  case 1:
    device = hwNode("tape", hw::storage);
    break;
  case 3:
    device = hwNode("processor", hw::processor);
    break;
  case 4:
  case 5:
    device = hwNode("cdrom", hw::storage);
    break;
  case 6:
    device = hwNode("scanner", hw::generic);
    break;
  case 7:
    device = hwNode("magnetooptical", hw::storage);
    break;
  case 8:
    device = hwNode("changer", hw::generic);
    break;
  case 0xd:
    device = hwNode("enclosure", hw::generic);
    break;
  }

  device.setDescription(scsi_type(m_id.scsi_type));
  device.setHandle(scsi_handle(m_id.host_no,
			       m_id.channel, m_id.scsi_id, m_id.lun));
  find_logicalname(device);
  do_inquiry(fd, device);
  if ((m_id.scsi_type == 4) || (m_id.scsi_type == 5))
    scan_cdrom(device);

  channel->addChild(device);

  close(fd);

  return true;
}

bool scan_scsi(hwNode & n)
{
  int i = 0;

  scan_devices();

  while (scan_sg(i, n))
    i++;

  return false;
}

static char *id = "@(#) $Id: scsi.cc,v 1.10 2003/02/17 21:41:43 ezix Exp $";
