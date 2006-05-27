/*
 * burner.cc
 *
 * This file was originally part of dvd+rw-tools by Andy Polyakov but it went
 * through severe modifications. Don't blame Andy for any bug.
 *
 * Original notice below.
 *
 * This is part of dvd+rw-tools by Andy Polyakov <appro@fy.chalmers.se>
 *
 * Use-it-on-your-own-risk, GPL bless...
 *
 * For further details see http://fy.chalmers.se/~appro/linux/DVD+RW/
 */

#define CREAM_ON_ERRNO(s) do { \
  switch ((s)[2]&0x0F) \
  { \
    case 2: if ((s)[12]==4) errno=EAGAIN; break; \
      case 5: errno=EINVAL; \
        if ((s)[13]==0) \
        { \
          if ((s)[12]==0x21)    errno=ENOSPC; \
          else if ((s)[12]==0x20) errno=ENODEV; \
        } \
        break; \
      } \
    } while(0)
    #define ERRCODE(s)  ((((s)[2]&0x0F)<<16)|((s)[12]<<8)|((s)[13]))
    #define SK(errcode) (((errcode)>>16)&0xF)
    #define ASC(errcode)  (((errcode)>>8)&0xFF)
    #define ASCQ(errcode) ((errcode)&0xFF)

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <errno.h>
#include <string.h>
#include <mntent.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <poll.h>
#include <sys/time.h>

#include "version.h"
#include "burner.h"

      typedef enum
      {
        NONE = CGC_DATA_NONE,                     // 3
        READ = CGC_DATA_READ,                     // 2
        WRITE = CGC_DATA_WRITE                    // 1
      } Direction;

typedef struct ScsiCommand ScsiCommand;

struct ScsiCommand
{
  int fd;
  int autoclose;
  char *filename;
  struct cdrom_generic_command cgc;
  union
  {
    struct request_sense s;
    unsigned char u[18];
  } _sense;
  struct sg_io_hdr sg_io;
};

#define DIRECTION(i) (Dir_xlate[i]);

/* 1,CGC_DATA_WRITE
 * 2,CGC_DATA_READ
 * 3,CGC_DATA_NONE
 */
const int Dir_xlate[4] =
{
  0,                                              // implementation-dependent...
  SG_DXFER_TO_DEV,                                // 1,CGC_DATA_WRITE
  SG_DXFER_FROM_DEV,                              // 2,CGC_DATA_READ
  SG_DXFER_NONE                                   // 3,CGC_DATA_NONE
};

static ScsiCommand * scsi_command_new (void)
{
  ScsiCommand *cmd;

  cmd = (ScsiCommand *) malloc (sizeof (ScsiCommand));
  memset (cmd, 0, sizeof (ScsiCommand));
  cmd->fd = -1;
  cmd->filename = NULL;
  cmd->autoclose = 1;

  return cmd;
}


static ScsiCommand * scsi_command_new_from_fd (int f)
{
  ScsiCommand *cmd;

  cmd = scsi_command_new ();
  cmd->fd = f;
  cmd->autoclose = 0;

  return cmd;
}


static void scsi_command_free (ScsiCommand * cmd)
{
  if (cmd->fd >= 0 && cmd->autoclose)
  {
    close (cmd->fd);
    cmd->fd = -1;
  }
  if (cmd->filename)
  {
    free (cmd->filename);
    cmd->filename = NULL;
  }

  free (cmd);
}


static int scsi_command_transport (ScsiCommand * cmd, Direction dir, void *buf,
size_t sz)
{
  int ret = 0;

  cmd->sg_io.dxferp = buf;
  cmd->sg_io.dxfer_len = sz;
  cmd->sg_io.dxfer_direction = DIRECTION (dir);

  if (ioctl (cmd->fd, SG_IO, &cmd->sg_io))
    return -1;

  if ((cmd->sg_io.info & SG_INFO_OK_MASK) != SG_INFO_OK)
  {
    errno = EIO;
    ret = -1;
    if (cmd->sg_io.masked_status & CHECK_CONDITION)
    {
      CREAM_ON_ERRNO (cmd->sg_io.sbp);
      ret = ERRCODE (cmd->sg_io.sbp);
      if (ret == 0)
        ret = -1;
    }
  }

  return ret;
}


static void scsi_command_init (ScsiCommand * cmd, size_t i, int arg)
{
  if (i == 0)
  {
    memset (&cmd->cgc, 0, sizeof (cmd->cgc));
    memset (&cmd->_sense, 0, sizeof (cmd->_sense));
    cmd->cgc.quiet = 1;
    cmd->cgc.sense = &cmd->_sense.s;
    memset (&cmd->sg_io, 0, sizeof (cmd->sg_io));
    cmd->sg_io.interface_id = 'S';
    cmd->sg_io.mx_sb_len = sizeof (cmd->_sense);
    cmd->sg_io.cmdp = cmd->cgc.cmd;
    cmd->sg_io.sbp = cmd->_sense.u;
    cmd->sg_io.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;
  }
  cmd->sg_io.cmd_len = i + 1;
  cmd->cgc.cmd[i] = arg;
}


int get_dvd_r_rw_profile (int fd)
{
  ScsiCommand *cmd;
  int retval = -1;
  unsigned char page[20];
  unsigned char *list;
  int i, len;

  cmd = scsi_command_new_from_fd (fd);

  scsi_command_init (cmd, 0, 0x46);
  scsi_command_init (cmd, 1, 2);
  scsi_command_init (cmd, 8, 8);
  scsi_command_init (cmd, 9, 0);
  if (scsi_command_transport (cmd, READ, page, 8))
  {
/* GET CONFIGURATION failed */
    scsi_command_free (cmd);
    return -1;
  }

/* See if it's 2 gen drive by checking if DVD+R profile is an option */
  len = 4 + (page[0] << 24 | page[1] << 16 | page[2] << 8 | page[3]);
  if (len > 264)
  {
    scsi_command_free (cmd);
/* insane profile list length */
    return -1;
  }

  list = (unsigned char *) malloc (len);

  scsi_command_init (cmd, 0, 0x46);
  scsi_command_init (cmd, 1, 2);
  scsi_command_init (cmd, 7, len >> 8);
  scsi_command_init (cmd, 8, len);
  scsi_command_init (cmd, 9, 0);
  if (scsi_command_transport (cmd, READ, list, len))
  {
/* GET CONFIGURATION failed */
    scsi_command_free (cmd);
    free (list);
    return -1;
  }

  for (i = 12; i < list[11]; i += 4)
  {
    int profile = (list[i] << 8 | list[i + 1]);
/* 0x1B: DVD+R
 * 0x1A: DVD+RW */
    if (profile == 0x1B)
    {
      if (retval == 1)
        retval = 2;
      else
        retval = 0;
    }
    else if (profile == 0x1A)
    {
      if (retval == 0)
        retval = 2;
      else
        retval = 1;
    }
  }

  scsi_command_free (cmd);
  free (list);

  return retval;
}


static unsigned char * pull_page2a_from_fd (int fd)
{
  ScsiCommand *cmd;
  unsigned char header[12], *page2A;
  unsigned int len, bdlen;

  cmd = scsi_command_new_from_fd (fd);

  scsi_command_init (cmd, 0, 0x5A);               /* MODE SENSE */
  scsi_command_init (cmd, 1, 0x08);               /* Disable Block Descriptors */
  scsi_command_init (cmd, 2, 0x2A);               /* Capabilities and Mechanical Status */
  scsi_command_init (cmd, 8, sizeof (header));    /* header only to start with */
  scsi_command_init (cmd, 9, 0);

  if (scsi_command_transport (cmd, READ, header, sizeof (header)))
  {
/* MODE SENSE failed */
    scsi_command_free (cmd);
    return NULL;
  }

  len = (header[0] << 8 | header[1]) + 2;
  bdlen = header[6] << 8 | header[7];

/* should never happen as we set "DBD" above */
  if (bdlen)
  {
    if (len < (8 + bdlen + 30))
    {
/* LUN impossible to bear with */
      scsi_command_free (cmd);
      return NULL;
    }
  }
  else if (len < (8 + 2 + (unsigned int) header[9]))
  {
/* SANYO does this. */
    len = 8 + 2 + header[9];
  }

  page2A = (unsigned char *) malloc (len);
  if (page2A == NULL)
  {
/* ENOMEM */
    scsi_command_free (cmd);
    return NULL;
  }

  scsi_command_init (cmd, 0, 0x5A);               /* MODE SENSE */
  scsi_command_init (cmd, 1, 0x08);               /* Disable Block Descriptors */
  scsi_command_init (cmd, 2, 0x2A);               /* Capabilities and Mechanical Status */
  scsi_command_init (cmd, 7, len >> 8);
  scsi_command_init (cmd, 8, len);                /* Real length */
  scsi_command_init (cmd, 9, 0);
  if (scsi_command_transport (cmd, READ, page2A, len))
  {
/* MODE SENSE failed */
    scsi_command_free (cmd);
    free (page2A);
    return NULL;
  }

  scsi_command_free (cmd);

  len -= 2;
/* paranoia */
  if (len < ((unsigned int) page2A[0] << 8 | page2A[1]))
  {
    page2A[0] = len >> 8;
    page2A[1] = len;
  }

  return page2A;
}


static int get_read_write_speed (int fd, int *read_speed, int *write_speed)
{
  unsigned char *page2A;
  int len, hlen;
  unsigned char *p;

  *read_speed = 0;
  *write_speed = 0;

  page2A = pull_page2a_from_fd (fd);
  if (page2A == NULL)
  {
    printf ("Failed to get Page 2A\n");
/* Failed to get Page 2A */
    return -1;
  }

  len = (page2A[0] << 8 | page2A[1]) + 2;
  hlen = 8 + (page2A[6] << 8 | page2A[7]);
  p = page2A + hlen;

/* Values guessed from the cd_mode_page_2A struct
 * in cdrecord's libscg/scg/scsireg.h */
  if (len < (hlen + 30) || p[1] < (30 - 2))
  {
/* no MMC-3 "Current Write Speed" present,
 * try to use the MMC-2 one */
    if (len < (hlen + 20) || p[1] < (20 - 2))
      *write_speed = 0;
    else
      *write_speed = p[18] << 8 | p[19];
  }
  else
  {
    *write_speed = p[28] << 8 | p[29];
  }

  if (len >= hlen+9)
    *read_speed = p[8] << 8 | p[9];
  else
    *read_speed = 0;

  free (page2A);

  return 0;
}


static int get_disc_type (int fd)
{
  ScsiCommand *cmd;
  int retval = -1;
  unsigned char header[8];

  cmd = scsi_command_new_from_fd (fd);

  scsi_command_init (cmd, 0, 0x46);
  scsi_command_init (cmd, 1, 1);
  scsi_command_init (cmd, 8, 8);
  scsi_command_init (cmd, 9, 0);
  if (scsi_command_transport (cmd, READ, header, 8))
  {
/* GET CONFIGURATION failed */
    scsi_command_free (cmd);
    return -1;
  }

  retval = (header[6]<<8)|(header[7]);

  scsi_command_free (cmd);
  return retval;
}


static int disc_is_appendable (int fd)
{
  ScsiCommand *cmd;
  int retval = -1;
  unsigned char header[32];

  cmd = scsi_command_new_from_fd (fd);

/* see section 5.19 of MMC-3 from http://www.t10.org/drafts.htm#mmc3 */
  scsi_command_init (cmd, 0, 0x51);               /* READ_DISC_INFORMATION */
  scsi_command_init (cmd, 8, 32);
  scsi_command_init (cmd, 9, 0);
  if (scsi_command_transport (cmd, READ, header, 32))
  {
/* READ_DISC_INFORMATION failed */
    scsi_command_free (cmd);
    return 0;
  }

  retval = ((header[2]&0x03) == 0x01);

  scsi_command_free (cmd);
  return retval;
}


static int disc_is_rewritable (int fd)
{
  ScsiCommand *cmd;
  int retval = -1;
  unsigned char header[32];

  cmd = scsi_command_new_from_fd (fd);

/* see section 5.19 of MMC-3 from http://www.t10.org/drafts.htm#mmc3 */
  scsi_command_init (cmd, 0, 0x51);               /* READ_DISC_INFORMATION */
  scsi_command_init (cmd, 8, 32);
  scsi_command_init (cmd, 9, 0);
  if (scsi_command_transport (cmd, READ, header, 32))
  {
/* READ_DISC_INFORMATION failed */
    scsi_command_free (cmd);
    return 0;
  }

  retval = ((header[2]&0x10) != 0);

  scsi_command_free (cmd);
  return retval;
}
