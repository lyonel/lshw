#include "mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>

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

  channel->addChild(device);

  close(fd);

  return true;
}

bool scan_scsi(hwNode & n)
{
  int i = 0;

  while (scan_sg(i, n))
    i++;

  return false;
}

static char *id = "@(#) $Id: scsi.cc,v 1.5 2003/02/17 01:19:17 ezix Exp $";
