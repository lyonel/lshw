#include "version.h"
#include "pcmcia-legacy.h"
#include "osutils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

__ID("@(#) $Id$");

/* parts of this code come from the excellent pcmcia-cs package written by
 * David A. Hinds <dahinds@users.sourceforge.net>.
 */

#define VARLIBPCMCIASTAB  "/var/lib/pcmcia/stab"

#ifdef __arm__
typedef u_int ioaddr_t;
#else
typedef u_short ioaddr_t;
#endif

typedef u_char cisdata_t;
typedef u_int event_t;

typedef struct cs_status_t
{
  u_char Function;
  event_t CardState;
  event_t SocketState;
}


cs_status_t;

#define CS_EVENT_CARD_DETECT            0x000080

typedef struct config_info_t
{
  u_char Function;
  u_int Attributes;
  u_int Vcc, Vpp1, Vpp2;
  u_int IntType;
  u_int ConfigBase;
  u_char Status, Pin, Copy, Option, ExtStatus;
  u_int Present;
  u_int CardValues;
  u_int AssignedIRQ;
  u_int IRQAttributes;
  ioaddr_t BasePort1;
  ioaddr_t NumPorts1;
  u_int Attributes1;
  ioaddr_t BasePort2;
  ioaddr_t NumPorts2;
  u_int Attributes2;
  u_int IOAddrLines;
}


config_info_t;

#define DEV_NAME_LEN 32
typedef char dev_info_t[DEV_NAME_LEN];

typedef struct bind_info_t
{
  dev_info_t dev_info;
  u_char function;
/*
 * struct dev_link_t   *instance;
 */
  void *instance;
  char name[DEV_NAME_LEN];
  u_short major, minor;
  void *next;
}


bind_info_t;

#define BIND_FN_ALL 0xff

#define CISTPL_NULL   0x00
#define CISTPL_DEVICE   0x01
#define CISTPL_LONGLINK_CB  0x02
#define CISTPL_INDIRECT   0x03
#define CISTPL_CONFIG_CB  0x04
#define CISTPL_CFTABLE_ENTRY_CB 0x05
#define CISTPL_LONGLINK_MFC 0x06
#define CISTPL_BAR    0x07
#define CISTPL_PWR_MGMNT  0x08
#define CISTPL_EXTDEVICE  0x09
#define CISTPL_CHECKSUM   0x10
#define CISTPL_LONGLINK_A 0x11
#define CISTPL_LONGLINK_C 0x12
#define CISTPL_LINKTARGET 0x13
#define CISTPL_NO_LINK    0x14
#define CISTPL_VERS_1   0x15
#define CISTPL_ALTSTR   0x16
#define CISTPL_DEVICE_A   0x17
#define CISTPL_JEDEC_C    0x18
#define CISTPL_JEDEC_A    0x19
#define CISTPL_CONFIG   0x1a
#define CISTPL_CFTABLE_ENTRY  0x1b
#define CISTPL_DEVICE_OC  0x1c
#define CISTPL_DEVICE_OA  0x1d
#define CISTPL_DEVICE_GEO 0x1e
#define CISTPL_DEVICE_GEO_A 0x1f
#define CISTPL_MANFID   0x20
#define CISTPL_FUNCID   0x21
#define CISTPL_FUNCE    0x22
#define CISTPL_SWIL   0x23
#define CISTPL_END    0xff
/* Layer 2 tuples */
#define CISTPL_VERS_2   0x40
#define CISTPL_FORMAT   0x41
#define CISTPL_GEOMETRY   0x42
#define CISTPL_BYTEORDER  0x43
#define CISTPL_DATE   0x44
#define CISTPL_BATTERY    0x45
#define CISTPL_FORMAT_A   0x47
/* Layer 3 tuples */
#define CISTPL_ORG    0x46
#define CISTPL_SPCL   0x90

typedef struct cistpl_longlink_t
{
  u_int addr;
}


cistpl_longlink_t;

typedef struct cistpl_checksum_t
{
  u_short addr;
  u_short len;
  u_char sum;
}


cistpl_checksum_t;

#define CISTPL_MAX_FUNCTIONS  8
#define CISTPL_MFC_ATTR   0x00
#define CISTPL_MFC_COMMON 0x01

typedef struct cistpl_longlink_mfc_t
{
  u_char nfn;
  struct
  {
    u_char space;
    u_int addr;
  }
  fn[CISTPL_MAX_FUNCTIONS];
}


cistpl_longlink_mfc_t;

#define CISTPL_MAX_ALTSTR_STRINGS 4

typedef struct cistpl_altstr_t
{
  u_char ns;
  u_char ofs[CISTPL_MAX_ALTSTR_STRINGS];
  char str[254];
}


cistpl_altstr_t;

#define CISTPL_DTYPE_NULL 0x00
#define CISTPL_DTYPE_ROM  0x01
#define CISTPL_DTYPE_OTPROM 0x02
#define CISTPL_DTYPE_EPROM  0x03
#define CISTPL_DTYPE_EEPROM 0x04
#define CISTPL_DTYPE_FLASH  0x05
#define CISTPL_DTYPE_SRAM 0x06
#define CISTPL_DTYPE_DRAM 0x07
#define CISTPL_DTYPE_FUNCSPEC 0x0d
#define CISTPL_DTYPE_EXTEND 0x0e

#define CISTPL_MAX_DEVICES  4

typedef struct cistpl_device_t
{
  u_char ndev;
  struct
  {
    u_char type;
    u_char wp;
    u_int speed;
    u_int size;
  }
  dev[CISTPL_MAX_DEVICES];
}


cistpl_device_t;

#define CISTPL_DEVICE_MWAIT 0x01
#define CISTPL_DEVICE_3VCC  0x02

typedef struct cistpl_device_o_t
{
  u_char flags;
  cistpl_device_t device;
}


cistpl_device_o_t;

#define CISTPL_VERS_1_MAX_PROD_STRINGS  4

typedef struct cistpl_vers_1_t
{
  u_char major;
  u_char minor;
  u_char ns;
  u_char ofs[CISTPL_VERS_1_MAX_PROD_STRINGS];
  char str[254];
}


cistpl_vers_1_t;

typedef struct cistpl_jedec_t
{
  u_char nid;
  struct
  {
    u_char mfr;
    u_char info;
  }
  id[CISTPL_MAX_DEVICES];
}


cistpl_jedec_t;

typedef struct cistpl_manfid_t
{
  u_short manf;
  u_short card;
}


cistpl_manfid_t;

#define CISTPL_FUNCID_MULTI 0x00
#define CISTPL_FUNCID_MEMORY  0x01
#define CISTPL_FUNCID_SERIAL  0x02
#define CISTPL_FUNCID_PARALLEL  0x03
#define CISTPL_FUNCID_FIXED 0x04
#define CISTPL_FUNCID_VIDEO 0x05
#define CISTPL_FUNCID_NETWORK 0x06
#define CISTPL_FUNCID_AIMS  0x07
#define CISTPL_FUNCID_SCSI  0x08

#define CISTPL_SYSINIT_POST 0x01
#define CISTPL_SYSINIT_ROM  0x02

typedef struct cistpl_funcid_t
{
  u_char func;
  u_char sysinit;
}


cistpl_funcid_t;

typedef struct cistpl_funce_t
{
  u_char type;
  u_char data[0];
}


cistpl_funce_t;

/*======================================================================

    Modem Function Extension Tuples

======================================================================*/

#define CISTPL_FUNCE_SERIAL_IF    0x00
#define CISTPL_FUNCE_SERIAL_CAP   0x01
#define CISTPL_FUNCE_SERIAL_SERV_DATA 0x02
#define CISTPL_FUNCE_SERIAL_SERV_FAX  0x03
#define CISTPL_FUNCE_SERIAL_SERV_VOICE  0x04
#define CISTPL_FUNCE_SERIAL_CAP_DATA  0x05
#define CISTPL_FUNCE_SERIAL_CAP_FAX 0x06
#define CISTPL_FUNCE_SERIAL_CAP_VOICE 0x07
#define CISTPL_FUNCE_SERIAL_IF_DATA 0x08
#define CISTPL_FUNCE_SERIAL_IF_FAX  0x09
#define CISTPL_FUNCE_SERIAL_IF_VOICE  0x0a

/* UART identification */
#define CISTPL_SERIAL_UART_8250   0x00
#define CISTPL_SERIAL_UART_16450  0x01
#define CISTPL_SERIAL_UART_16550  0x02
#define CISTPL_SERIAL_UART_8251   0x03
#define CISTPL_SERIAL_UART_8530   0x04
#define CISTPL_SERIAL_UART_85230  0x05

/* UART capabilities */
#define CISTPL_SERIAL_UART_SPACE  0x01
#define CISTPL_SERIAL_UART_MARK   0x02
#define CISTPL_SERIAL_UART_ODD    0x04
#define CISTPL_SERIAL_UART_EVEN   0x08
#define CISTPL_SERIAL_UART_5BIT   0x01
#define CISTPL_SERIAL_UART_6BIT   0x02
#define CISTPL_SERIAL_UART_7BIT   0x04
#define CISTPL_SERIAL_UART_8BIT   0x08
#define CISTPL_SERIAL_UART_1STOP  0x10
#define CISTPL_SERIAL_UART_MSTOP  0x20
#define CISTPL_SERIAL_UART_2STOP  0x40

typedef struct cistpl_serial_t
{
  u_char uart_type;
  u_char uart_cap_0;
  u_char uart_cap_1;
}


cistpl_serial_t;

typedef struct cistpl_modem_cap_t
{
  u_char flow;
  u_char cmd_buf;
  u_char rcv_buf_0, rcv_buf_1, rcv_buf_2;
  u_char xmit_buf_0, xmit_buf_1, xmit_buf_2;
}


cistpl_modem_cap_t;

#define CISTPL_SERIAL_MOD_103   0x01
#define CISTPL_SERIAL_MOD_V21   0x02
#define CISTPL_SERIAL_MOD_V23   0x04
#define CISTPL_SERIAL_MOD_V22   0x08
#define CISTPL_SERIAL_MOD_212A    0x10
#define CISTPL_SERIAL_MOD_V22BIS  0x20
#define CISTPL_SERIAL_MOD_V26   0x40
#define CISTPL_SERIAL_MOD_V26BIS  0x80
#define CISTPL_SERIAL_MOD_V27BIS  0x01
#define CISTPL_SERIAL_MOD_V29   0x02
#define CISTPL_SERIAL_MOD_V32   0x04
#define CISTPL_SERIAL_MOD_V32BIS  0x08
#define CISTPL_SERIAL_MOD_V34   0x10

#define CISTPL_SERIAL_ERR_MNP2_4  0x01
#define CISTPL_SERIAL_ERR_V42_LAPM  0x02

#define CISTPL_SERIAL_CMPR_V42BIS 0x01
#define CISTPL_SERIAL_CMPR_MNP5   0x02

#define CISTPL_SERIAL_CMD_AT1   0x01
#define CISTPL_SERIAL_CMD_AT2   0x02
#define CISTPL_SERIAL_CMD_AT3   0x04
#define CISTPL_SERIAL_CMD_MNP_AT  0x08
#define CISTPL_SERIAL_CMD_V25BIS  0x10
#define CISTPL_SERIAL_CMD_V25A    0x20
#define CISTPL_SERIAL_CMD_DMCL    0x40

typedef struct cistpl_data_serv_t
{
  u_char max_data_0;
  u_char max_data_1;
  u_char modulation_0;
  u_char modulation_1;
  u_char error_control;
  u_char compression;
  u_char cmd_protocol;
  u_char escape;
  u_char encrypt;
  u_char misc_features;
  u_char ccitt_code[0];
}


cistpl_data_serv_t;

typedef struct cistpl_fax_serv_t
{
  u_char max_data_0;
  u_char max_data_1;
  u_char modulation;
  u_char encrypt;
  u_char features_0;
  u_char features_1;
  u_char ccitt_code[0];
}


cistpl_fax_serv_t;

typedef struct cistpl_voice_serv_t
{
  u_char max_data_0;
  u_char max_data_1;
}


cistpl_voice_serv_t;

/*======================================================================

    LAN Function Extension Tuples

======================================================================*/

#define CISTPL_FUNCE_LAN_TECH   0x01
#define CISTPL_FUNCE_LAN_SPEED    0x02
#define CISTPL_FUNCE_LAN_MEDIA    0x03
#define CISTPL_FUNCE_LAN_NODE_ID  0x04
#define CISTPL_FUNCE_LAN_CONNECTOR  0x05

/* LAN technologies */
#define CISTPL_LAN_TECH_ARCNET    0x01
#define CISTPL_LAN_TECH_ETHERNET  0x02
#define CISTPL_LAN_TECH_TOKENRING 0x03
#define CISTPL_LAN_TECH_LOCALTALK 0x04
#define CISTPL_LAN_TECH_FDDI    0x05
#define CISTPL_LAN_TECH_ATM   0x06
#define CISTPL_LAN_TECH_WIRELESS  0x07

typedef struct cistpl_lan_tech_t
{
  u_char tech;
}


cistpl_lan_tech_t;

typedef struct cistpl_lan_speed_t
{
  u_int speed;
}


cistpl_lan_speed_t;

/* LAN media definitions */
#define CISTPL_LAN_MEDIA_UTP    0x01
#define CISTPL_LAN_MEDIA_STP    0x02
#define CISTPL_LAN_MEDIA_THIN_COAX  0x03
#define CISTPL_LAN_MEDIA_THICK_COAX 0x04
#define CISTPL_LAN_MEDIA_FIBER    0x05
#define CISTPL_LAN_MEDIA_900MHZ   0x06
#define CISTPL_LAN_MEDIA_2GHZ   0x07
#define CISTPL_LAN_MEDIA_5GHZ   0x08
#define CISTPL_LAN_MEDIA_DIFF_IR  0x09
#define CISTPL_LAN_MEDIA_PTP_IR   0x0a

typedef struct cistpl_lan_media_t
{
  u_char media;
}


cistpl_lan_media_t;

typedef struct cistpl_lan_node_id_t
{
  u_char nb;
  u_char id[16];
}


cistpl_lan_node_id_t;

typedef struct cistpl_lan_connector_t
{
  u_char code;
}


cistpl_lan_connector_t;

/*======================================================================

    IDE Function Extension Tuples

======================================================================*/

#define CISTPL_IDE_INTERFACE    0x01

typedef struct cistpl_ide_interface_t
{
  u_char interface;
}


cistpl_ide_interface_t;

/* First feature byte */
#define CISTPL_IDE_SILICON    0x04
#define CISTPL_IDE_UNIQUE   0x08
#define CISTPL_IDE_DUAL     0x10

/* Second feature byte */
#define CISTPL_IDE_HAS_SLEEP    0x01
#define CISTPL_IDE_HAS_STANDBY    0x02
#define CISTPL_IDE_HAS_IDLE   0x04
#define CISTPL_IDE_LOW_POWER    0x08
#define CISTPL_IDE_REG_INHIBIT    0x10
#define CISTPL_IDE_HAS_INDEX    0x20
#define CISTPL_IDE_IOIS16   0x40

typedef struct cistpl_ide_feature_t
{
  u_char feature1;
  u_char feature2;
}


cistpl_ide_feature_t;

#define CISTPL_FUNCE_IDE_IFACE    0x01
#define CISTPL_FUNCE_IDE_MASTER   0x02
#define CISTPL_FUNCE_IDE_SLAVE    0x03

/*======================================================================

    Configuration Table Entries

======================================================================*/

#define CISTPL_BAR_SPACE  0x07
#define CISTPL_BAR_SPACE_IO 0x10
#define CISTPL_BAR_PREFETCH 0x20
#define CISTPL_BAR_CACHEABLE  0x40
#define CISTPL_BAR_1MEG_MAP 0x80

typedef struct cistpl_bar_t
{
  u_char attr;
  u_int size;
}


cistpl_bar_t;

typedef struct cistpl_config_t
{
  u_char last_idx;
  u_int base;
  u_int rmask[4];
  u_char subtuples;
}


cistpl_config_t;

/* These are bits in the 'present' field, and indices in 'param' */
#define CISTPL_POWER_VNOM 0
#define CISTPL_POWER_VMIN 1
#define CISTPL_POWER_VMAX 2
#define CISTPL_POWER_ISTATIC  3
#define CISTPL_POWER_IAVG 4
#define CISTPL_POWER_IPEAK  5
#define CISTPL_POWER_IDOWN  6

#define CISTPL_POWER_HIGHZ_OK 0x01
#define CISTPL_POWER_HIGHZ_REQ  0x02

typedef struct cistpl_power_t
{
  u_char present;
  u_char flags;
  u_int param[7];
}


cistpl_power_t;

typedef struct cistpl_timing_t
{
  u_int wait, waitscale;
  u_int ready, rdyscale;
  u_int reserved, rsvscale;
}


cistpl_timing_t;

#define CISTPL_IO_LINES_MASK  0x1f
#define CISTPL_IO_8BIT    0x20
#define CISTPL_IO_16BIT   0x40
#define CISTPL_IO_RANGE   0x80

#define CISTPL_IO_MAX_WIN 16

typedef struct cistpl_io_t
{
  u_char flags;
  u_char nwin;
  struct
  {
    u_int base;
    u_int len;
  }
  win[CISTPL_IO_MAX_WIN];
}


cistpl_io_t;

typedef struct cistpl_irq_t
{
  u_int IRQInfo1;
  u_int IRQInfo2;
}


cistpl_irq_t;

#define CISTPL_MEM_MAX_WIN  8

typedef struct cistpl_mem_t
{
  u_char flags;
  u_char nwin;
  struct
  {
    u_int len;
    u_int card_addr;
    u_int host_addr;
  }
  win[CISTPL_MEM_MAX_WIN];
}


cistpl_mem_t;

#define CISTPL_CFTABLE_DEFAULT    0x0001
#define CISTPL_CFTABLE_BVDS   0x0002
#define CISTPL_CFTABLE_WP   0x0004
#define CISTPL_CFTABLE_RDYBSY   0x0008
#define CISTPL_CFTABLE_MWAIT    0x0010
#define CISTPL_CFTABLE_AUDIO    0x0800
#define CISTPL_CFTABLE_READONLY   0x1000
#define CISTPL_CFTABLE_PWRDOWN    0x2000

typedef struct cistpl_cftable_entry_t
{
  u_char index;
  u_short flags;
  u_char interface;
  cistpl_power_t vcc, vpp1, vpp2;
  cistpl_timing_t timing;
  cistpl_io_t io;
  cistpl_irq_t irq;
  cistpl_mem_t mem;
  u_char subtuples;
}


cistpl_cftable_entry_t;

#define CISTPL_CFTABLE_MASTER   0x000100
#define CISTPL_CFTABLE_INVALIDATE 0x000200
#define CISTPL_CFTABLE_VGA_PALETTE  0x000400
#define CISTPL_CFTABLE_PARITY   0x000800
#define CISTPL_CFTABLE_WAIT   0x001000
#define CISTPL_CFTABLE_SERR   0x002000
#define CISTPL_CFTABLE_FAST_BACK  0x004000
#define CISTPL_CFTABLE_BINARY_AUDIO 0x010000
#define CISTPL_CFTABLE_PWM_AUDIO  0x020000

typedef struct cistpl_cftable_entry_cb_t
{
  u_char index;
  u_int flags;
  cistpl_power_t vcc, vpp1, vpp2;
  u_char io;
  cistpl_irq_t irq;
  u_char mem;
  u_char subtuples;
}


cistpl_cftable_entry_cb_t;

typedef struct cistpl_device_geo_t
{
  u_char ngeo;
  struct
  {
    u_char buswidth;
    u_int erase_block;
    u_int read_block;
    u_int write_block;
    u_int partition;
    u_int interleave;
  }
  geo[CISTPL_MAX_DEVICES];
}


cistpl_device_geo_t;

typedef struct cistpl_vers_2_t
{
  u_char vers;
  u_char comply;
  u_short dindex;
  u_char vspec8, vspec9;
  u_char nhdr;
  u_char vendor, info;
  char str[244];
}


cistpl_vers_2_t;

typedef struct cistpl_org_t
{
  u_char data_org;
  char desc[30];
}


cistpl_org_t;

#define CISTPL_ORG_FS   0x00
#define CISTPL_ORG_APPSPEC  0x01
#define CISTPL_ORG_XIP    0x02

typedef struct cistpl_format_t
{
  u_char type;
  u_char edc;
  u_int offset;
  u_int length;
}


cistpl_format_t;

#define CISTPL_FORMAT_DISK  0x00
#define CISTPL_FORMAT_MEM 0x01

#define CISTPL_EDC_NONE   0x00
#define CISTPL_EDC_CKSUM  0x01
#define CISTPL_EDC_CRC    0x02
#define CISTPL_EDC_PCC    0x03

typedef union cisparse_t
{
  cistpl_device_t device;
  cistpl_checksum_t checksum;
  cistpl_longlink_t longlink;
  cistpl_longlink_mfc_t longlink_mfc;
  cistpl_vers_1_t version_1;
  cistpl_altstr_t altstr;
  cistpl_jedec_t jedec;
  cistpl_manfid_t manfid;
  cistpl_funcid_t funcid;
  cistpl_funce_t funce;
  cistpl_bar_t bar;
  cistpl_config_t config;
  cistpl_cftable_entry_t cftable_entry;
  cistpl_cftable_entry_cb_t cftable_entry_cb;
  cistpl_device_geo_t device_geo;
  cistpl_vers_2_t vers_2;
  cistpl_org_t org;
  cistpl_format_t format;
}


cisparse_t;

typedef struct tuple_t
{
  u_int Attributes;
  cisdata_t DesiredTuple;
  u_int Flags;                                    /* internal use */
  u_int LinkOffset;                               /* internal use */
  u_int CISOffset;                                /* internal use */
  cisdata_t TupleCode;
  cisdata_t TupleLink;
  cisdata_t TupleOffset;
  cisdata_t TupleDataMax;
  cisdata_t TupleDataLen;
  cisdata_t *TupleData;
}


tuple_t;

typedef struct tuple_parse_t
{
  tuple_t tuple;
  cisdata_t data[255];
  cisparse_t parse;
}


tuple_parse_t;

typedef union ds_ioctl_arg_t
{
//servinfo_t servinfo;
//adjust_t            adjust;
  config_info_t config;
  tuple_t tuple;
  tuple_parse_t tuple_parse;
//client_req_t        client_req;
//cs_status_t         status;
//conf_reg_t          conf_reg;
//cisinfo_t           cisinfo;
//region_info_t       region;
  bind_info_t bind_info;
//mtd_info_t          mtd_info;
//win_info_t          win_info;
//cisdump_t           cisdump;
}


ds_ioctl_arg_t;

#define TUPLE_RETURN_COMMON             0x02
#define DS_GET_CARD_SERVICES_INFO       _IOR ('d', 1, servinfo_t)
#define DS_GET_CONFIGURATION_INFO       _IOWR('d', 3, config_info_t)
#define DS_GET_FIRST_TUPLE              _IOWR('d', 4, tuple_t)
#define DS_GET_NEXT_TUPLE               _IOWR('d', 5, tuple_t)
#define DS_GET_TUPLE_DATA               _IOWR('d', 6, tuple_parse_t)
#define DS_PARSE_TUPLE                  _IOWR('d', 7, tuple_parse_t)
#define DS_GET_STATUS                   _IOWR('d', 9, cs_status_t)
#define DS_BIND_REQUEST                 _IOWR('d', 60, bind_info_t)
#define DS_GET_DEVICE_INFO              _IOWR('d', 61, bind_info_t)
#define DS_GET_NEXT_DEVICE              _IOWR('d', 62, bind_info_t)

#define MAX_SOCK 8

static int lookup_dev(const char *name)
{
  FILE *f;
  int n;
  char s[32], t[32];

  f = fopen("/proc/devices", "r");
  if (f == NULL)
    return -errno;
  while (fgets(s, 32, f) != NULL)
  {
    if (sscanf(s, "%d %s", &n, t) == 2)
      if (strcmp(name, t) == 0)
        break;
  }
  fclose(f);
  if (strcmp(name, t) == 0)
    return n;
  else
    return -ENODEV;
}                                                 /* lookup_dev */


static string pcmcia_handle(int socket)
{
  char buffer[20];

  snprintf(buffer, sizeof(buffer), "PCMCIA:%d", socket);

  return string(buffer);
}


static int get_tuple(int fd,
cisdata_t code,
ds_ioctl_arg_t & arg)
{
  arg.tuple.DesiredTuple = code;
  arg.tuple.Attributes = TUPLE_RETURN_COMMON;
  arg.tuple.TupleOffset = 0;
  if ((ioctl(fd, DS_GET_FIRST_TUPLE, &arg) == 0) &&
    (ioctl(fd, DS_GET_TUPLE_DATA, &arg) == 0) &&
    (ioctl(fd, DS_PARSE_TUPLE, &arg) == 0))
    return 0;
  else
    return -1;
}


static bool pcmcia_ident(int socket,
int fd,
hwNode * parent)
{
  ds_ioctl_arg_t arg;
  cistpl_vers_1_t *vers = &arg.tuple_parse.parse.version_1;
  cistpl_funcid_t *funcid = &arg.tuple_parse.parse.funcid;
  config_info_t config;
  vector < string > product_info;
  hwNode device("pccard",
    hw::generic);
  char buffer[20];
  int i;

  if (get_tuple(fd, CISTPL_VERS_1, arg) == 0)
    for (i = 0; i < vers->ns; i++)
  {
    product_info.push_back(string(vers->str + vers->ofs[i]));
  }
  else
    return false;

  if (get_tuple(fd, CISTPL_FUNCID, arg) == 0)
    switch (funcid->func)
    {
      case 0:                                     // multifunction
        break;
      case 1:                                     // memory
        device = hwNode("memory", hw::memory);
        device.claim();
        break;
      case 2:                                     // serial
        device = hwNode("serial", hw::communication);
        device.claim();
      break;
    case 3:                                       // parallel
      device = hwNode("parallel", hw::communication);
      device.claim();
      break;
    case 4:                                       // fixed disk
      device = hwNode("storage", hw::storage);
      device.claim();
      break;
    case 5:                                       // video
      device = hwNode("video", hw::display);
      device.claim();
      break;
    case 6:                                       // network
      device = hwNode("network", hw::network);
      device.claim();
      break;
    case 7:                                       // AIMS ?
      break;
    case 8:                                       // SCSI
      device = hwNode("scsi", hw::bus);
      device.claim();
      break;
    default:
      break;
  }

  if (product_info.size() >= 2)
  {
    if (product_info.size() >= 3)
    {
      device.setVendor(product_info[0]);
      if (product_info.size() >= 4)
      {
        device.setDescription(product_info[1]);
        device.setProduct(product_info[2]);
      }
      else
        device.setDescription(product_info[1]);
    }
    else
      device.setDescription(product_info[0]);
    device.setVersion(product_info[product_info.size() - 1]);
  }
  else
  {
    if (product_info.size() >= 1)
      device.setDescription(product_info[0]);
  }

  snprintf(buffer, sizeof(buffer), "Socket %d", socket);
  device.setSlot(buffer);

  for (int fct = 0; fct < 4; fct++)
  {
    memset(&config, 0, sizeof(config));
    config.Function = fct;
    if (ioctl(fd, DS_GET_CONFIGURATION_INFO, &config) == 0)
    {
      if (config.AssignedIRQ != 0)
        device.addResource(hw::resource::irq(config.AssignedIRQ));
    }
  }

//memset(&bind, 0, sizeof(bind));
//strcpy(bind.dev_info, "cb_enabler");
//bind.function = 0;
//ioctl(fd, DS_GET_NEXT_DEVICE, &bind);
//printf("%s : %s -> %d\n", bind.dev_info, bind.name, errno);

  device.setHandle(pcmcia_handle(socket));
  parent->addChild(device);

  return true;
}


static bool is_cardbus(const hwNode * n)
{
  return (n->getClass() == hw::bridge) && n->isCapable("pcmcia");
}


static hwNode *find_pcmciaparent(int slot,
hwNode & root)
{
  hwNode *result = NULL;
  unsigned int i;
  int currentslot = 0;

  if (slot < 0)
    return NULL;

  result = root.findChildByHandle(pcmcia_handle(slot));
  if (result)
    return result;

  for (i = 0; i < root.countChildren(); i++)
  {
    if (is_cardbus(root.getChild(i)))
    {
      if (currentslot == slot)
        return (hwNode *) root.getChild(i);
      currentslot++;
    }
  }

  for (i = 0; i < root.countChildren(); i++)
  {
    result = NULL;
    result =
      find_pcmciaparent(slot - currentslot, *(hwNode *) root.getChild(i));
    if (result)
      return result;
  }

  return NULL;
}


bool scan_pcmcialegacy(hwNode & n)
{
  int fd[MAX_SOCK];
  int major = lookup_dev("pcmcia");
  unsigned int sockets = 0;
  unsigned int i;
  vector < string > stab;

  if (major < 0)                                  // pcmcia support not loaded, there isn't much
    return false;                                 // we can do

  memset(fd, 0, sizeof(fd));
  for (i = 0; i < MAX_SOCK; i++)
  {
    fd[i] = open_dev((dev_t) ((major << 8) + i), S_IFCHR);

    if (fd[i] >= 0)
    {
      hwNode *parent = find_pcmciaparent(i, n);
      cs_status_t status;
      if (i >= sockets)
        sockets = i + 1;

// check that slot is populated
      memset(&status, 0, sizeof(status));
      status.Function = 0;

      ioctl(fd[i], DS_GET_STATUS, &status);
      if (status.CardState & CS_EVENT_CARD_DETECT)
      {
        if (parent)
          pcmcia_ident(i, fd[i], parent);
        else
          pcmcia_ident(i, fd[i], &n);
      }
    }
    else
      break;
  }

  for (unsigned int j = 0; j < sockets; j++)
  {
    close(fd[j]);
  }

  if (loadfile(VARLIBPCMCIASTAB, stab))
  {
    string socketname = "";
    string carddescription = "";

    for (i = 0; i < stab.size(); i++)
    {
      if (stab[i][0] == 'S')
      {
        string::size_type pos = stab[i].find(':');

        socketname = "";
        carddescription = "";

        if (pos != string::npos)
        {
          socketname = stab[i].substr(0, pos);
          carddescription = stab[i].substr(pos + 1);
        }
        else
        {
          carddescription = stab[i];
          socketname = "";
        }
      }
      else
      {
        int cnt = 0;
        int unused = 0;
        int socket = -1, devmajor = 0, devminor = 0;
        char devclass[20], driver[20], logicalname[20];

        memset(devclass, 0, sizeof(devclass));
        memset(driver, 0, sizeof(driver));
        memset(logicalname, 0, sizeof(logicalname));

        cnt = sscanf(stab[i].c_str(),
          "%d %s %s %d %s %d %d",
          &socket, devclass, driver, &unused, logicalname,
          &devmajor, &devminor);

        if ((cnt == 7) || (cnt == 5))             // we found a correct entry
        {
          string devclassstr = string(devclass);
          hwNode *parent = n.findChildByHandle(pcmcia_handle(socket));

          if (socket >= (int) sockets)
            sockets = socket + 1;

          hwNode device = hwNode(devclass, hw::generic);

          if (devclassstr == "serial")
            device = hwNode(devclass, hw::communication);
          else if (devclassstr == "ide")
            device = hwNode(devclass, hw::storage);
          else if (devclassstr == "memory")
            device = hwNode(devclass, hw::memory);
          else if (devclassstr == "network")
            device = hwNode(devclass, hw::network);

          device.setLogicalName(logicalname);
          device.setConfig("driver", driver);
          device.claim();

          if (!parent)
          {
            parent = find_pcmciaparent(socket, n);

            if (parent)
            {
              hwNode *realparent = parent->getChild(0);
              if (!realparent)
              {
                parent = parent->addChild(hwNode("pccard"));
                parent->setHandle(pcmcia_handle(socket));
              }
              else
                parent = realparent;
            }
          }

          if (parent)
          {
            parent->setSlot(socketname);
            if (parent->getDescription() == "")
              parent->setDescription(carddescription);
            parent->addChild(device);
          }
          else
            n.addChild(device);
        }
      }
    }
  }

  return true;
}
