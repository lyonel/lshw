/*
 * network.cc
 *
 * This scan uses the same IOCTLs as ethtool, ifconfig or mii-diag to report
 * information about network interfaces like:
 * - medium type (ethernet, token ring, PPP, etc.)
 * - hardware address (MAC address)
 * - link status (link detected, in error, etc.)
 * - speed (10Mbits, 100Mbits, etc.)
 * - IP addressing
 *
 * As network interfaces can be plugged on PCI, PCMCIA, ISA, USB, etc. this
 * scan should be executed after all bus-related scans.
 *
 */

#include "version.h"
#include "config.h"
#include "network.h"
#include "osutils.h"
#include "sysfs.h"
#include "options.h"
#include "heuristics.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <sys/types.h>
using namespace std;

__ID("@(#) $Id$");

#ifndef ARPHRD_IEEE1394
#define ARPHRD_IEEE1394 24
#endif
#ifndef ARPHRD_SIT
#define ARPHRD_SIT  776
#endif

#ifndef SIOCETHTOOL
#define SIOCETHTOOL     0x8946
#endif
typedef unsigned long long u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int8_t s8;

struct ethtool_cmd
{
  u32 cmd;
  u32 supported;                                  /* Features this interface supports */
  u32 advertising;                                /* Features this interface advertises */
  u16 speed;                                      /* The forced speed, 10Mb, 100Mb, gigabit */
  u8 duplex;                                      /* Duplex, half or full */
  u8 port;                                        /* Which connector port */
  u8 phy_address;
  u8 transceiver;                                 /* Which tranceiver to use */
  u8 autoneg;                                     /* Enable or disable autonegotiation */
  u32 maxtxpkt;                                   /* Tx pkts before generating tx int */
  u32 maxrxpkt;                                   /* Rx pkts before generating rx int */
  u32 reserved[4];
};

#define MAX_LINK_MODE_MASK_SIZE 64
struct ethtool_link_settings
{
  u32 cmd;
  u32 speed;                                      /* The forced speed, 10Mb, 100Mb, gigabit */
  u8 duplex;                                      /* Duplex, half or full */
  u8 port;                                        /* Which connector port */
  u8 phy_address;
  u8 autoneg;                                     /* Enable or disable autonegotiation */
  u8 mdio_support;
  u8 eth_tp_mdix;
  u8 eth_tp_mdix_ctrl;
  s8 link_mode_masks_nwords;
  u8 transceiver;                                 /* Which tranceiver to use */
  u8 master_slave_cfg;
  u8 master_slave_state;
  u8 reserved1[1];
  u32 reserved[7];
  u32 link_mode_masks[3 * MAX_LINK_MODE_MASK_SIZE]; /* Link mode mask fields for modes:
                                                       supported, advertised, peer advertised. */
};

/* Recognized module eeprom standards. */
#define ETH_MODULE_SFF_8079     0x1
#define ETH_MODULE_SFF_8472     0x2
#define ETH_MODULE_SFF_8636     0x3
#define ETH_MODULE_SFF_8436     0x4

struct ethtool_modinfo {
  u32 cmd;
  u32 type;                                       /* SFF standard used in module */
  u32 eeprom_len;                                 /* Length of module eeprom */
  u32 reserved[8];
};

/* Known id types. */
#define SFF_8024_ID_SOLDERED                    0x2
#define SFF_8024_ID_SFP                         0x3
#define SFF_8024_EXT_ID_DEFINED_BY_2WIRE_ID     0x4

/* Common connector types. */
#define SFF_8024_CONNECTOR_SC                   0x1
#define SFF_8024_CONNECTOR_LC                   0x7
#define SFF_8024_CONNECTOR_OPTICAL_PIGTAIL      0xb
#define SFF_8024_CONNECTOR_COPPER_PIGTAIL       0x21
#define SFF_8024_CONNECTOR_RJ45                 0x22
#define SFF_8024_CONNECTOR_NON_SEPARABLE        0x23

#define MAX_EEPROM_SIZE 256
struct ethtool_eeprom {
  u32 cmd;
  u32 magic;                                      /* Only used for eeprom writes */
  u32 offset;                                     /* Read or write offset */
  u32 len;                                        /* Length of read/write */
  u8 data[MAX_EEPROM_SIZE];                       /* Buffer */
};

#ifndef IFNAMSIZ
#define IFNAMSIZ 32
#endif
#define SIOCGIWNAME     0x8B01                    /* get name == wireless protocol */
/* SIOCGIWNAME is used to verify the presence of Wireless Extensions.
 * Common values : "IEEE 802.11-DS", "IEEE 802.11-FH", "IEEE 802.11b"... */

#define ETHTOOL_BUSINFO_LEN     32
/* these strings are set to whatever the driver author decides... */
struct ethtool_drvinfo
{
  u32 cmd;
  char driver[32];                                /* driver short name, "tulip", "eepro100" */
  char version[32];                               /* driver version string */
  char fw_version[32];                            /* firmware version string, if applicable */
  char bus_info[ETHTOOL_BUSINFO_LEN];             /* Bus info for this IF. */
/*
 * For PCI devices, use pci_dev->slot_name.
 */
  char reserved1[32];
  char reserved2[16];
  u32 n_stats;                                    /* number of u64's from ETHTOOL_GSTATS */
  u32 testinfo_len;
  u32 eedump_len;                                 /* Size of data from ETHTOOL_GEEPROM (bytes) */
  u32 regdump_len;                                /* Size of data from ETHTOOL_GREGS (bytes) */
};

/* for passing single values */
struct ethtool_value
{
  u32 cmd;
  u32 data;
};

/* CMDs currently supported */
#define ETHTOOL_GSET            0x00000001        /* Get settings. */
#define ETHTOOL_GDRVINFO        0x00000003        /* Get driver info. */
#define ETHTOOL_GLINK           0x0000000a        /* Get link status (ethtool_value) */
#define ETHTOOL_GMODULEINFO     0x00000042        /* Get plug-in module information */
#define ETHTOOL_GMODULEEEPROM   0x00000043        /* Get plug-in module eeprom */
#define ETHTOOL_GLINKSETTINGS   0x0000004c        /* Get link mode settings. */

/* Indicates what features are supported by the interface. */
#define SUPPORTED_10baseT_Half          (1 << 0)
#define SUPPORTED_10baseT_Full          (1 << 1)
#define SUPPORTED_100baseT_Half         (1 << 2)
#define SUPPORTED_100baseT_Full         (1 << 3)
#define SUPPORTED_1000baseT_Half        (1 << 4)
#define SUPPORTED_1000baseT_Full        (1 << 5)
#define SUPPORTED_Autoneg               (1 << 6)
#define SUPPORTED_TP                    (1 << 7)
#define SUPPORTED_AUI                   (1 << 8)
#define SUPPORTED_MII                   (1 << 9)
#define SUPPORTED_FIBRE                 (1 << 10)
#define SUPPORTED_BNC                   (1 << 11)
#define SUPPORTED_10000baseT_Full       (1 << 12)
#define SUPPORTED_2500baseX_Full        (1 << 15)
#define SUPPORTED_BACKPLANE             (1 << 16)
#define SUPPORTED_1000baseKX_Full       (1 << 17)
#define SUPPORTED_10000baseKX4_Full     (1 << 18)
#define SUPPORTED_10000baseKR_Full      (1 << 19)
#define SUPPORTED_10000baseR_FEC        (1 << 20)
#define SUPPORTED_20000baseMLD2_Full    (1 << 21)
#define SUPPORTED_20000baseKR2_Full     (1 << 22)
#define SUPPORTED_40000baseKR4_Full     (1 << 23)
#define SUPPORTED_40000baseCR4_Full     (1 << 24)
#define SUPPORTED_40000baseSR4_Full     (1 << 25)
#define SUPPORTED_40000baseLR4_Full     (1 << 26)
#define SUPPORTED_56000baseKR4_Full     (1 << 27)
#define SUPPORTED_56000baseCR4_Full     (1 << 28)
#define SUPPORTED_56000baseSR4_Full     (1 << 29)
#define SUPPORTED_56000baseLR4_Full     (1 << 30)
#define SUPPORTED_25000baseCR_Full      (1 << 31)
#define SUPPORTED_25000baseKR_Full      (1 << 32)
#define SUPPORTED_25000baseSR_Full      (1 << 33)
#define SUPPORTED_50000baseCR2_Full     (1 << 34)
#define SUPPORTED_50000baseKR2_Full     (1 << 35)
#define SUPPORTED_100000baseKR4_Full    (1 << 36)
#define SUPPORTED_100000baseSR4_Full    (1 << 37)
#define SUPPORTED_100000baseCR4_Full    (1 << 38)
#define SUPPORTED_100000baseLR4_ER4_Full (1 << 39)
#define SUPPORTED_50000baseSR2_Full     (1 << 40)
#define SUPPORTED_1000baseX_Full        (1 << 41)
#define SUPPORTED_10000baseCR_Full      (1 << 42)
#define SUPPORTED_10000baseSR_Full      (1 << 43)
#define SUPPORTED_10000baseLR_Full      (1 << 44)
#define SUPPORTED_10000baseLRM_Full     (1 << 45)
#define SUPPORTED_10000baseER_Full      (1 << 46)
#define SUPPORTED_2500baseT_Full        (1 << 47)
#define SUPPORTED_5000baseT_Full        (1 << 48)

/* Indicates what features are supported by the interface,
 * in the second word of the extended bitmap. */
#define SUPPORTED2_25000baseKR_Full         (1 << 0)
#define SUPPORTED2_25000baseSR_Full         (1 << 1)
#define SUPPORTED2_100000baseKR4_Full       (1 << 4)
#define SUPPORTED2_100000baseSR4_Full       (1 << 5)
#define SUPPORTED2_100000baseCR4_Full       (1 << 6)
#define SUPPORTED2_100000baseLR4_ER4_Full   (1 << 7)
#define SUPPORTED2_1000baseX_Full           (1 << 9)
#define SUPPORTED2_10000baseCR_Full         (1 << 10)
#define SUPPORTED2_10000baseSR_Full         (1 << 11)
#define SUPPORTED2_10000baseLR_Full         (1 << 12)
#define SUPPORTED2_10000baseLRM_Full        (1 << 13)
#define SUPPORTED2_10000baseER_Full         (1 << 14)
#define SUPPORTED2_2500baseT_Full           (1 << 15)
#define SUPPORTED2_5000baseT_Full           (1 << 16)

/* The forced speed, 10Mb, 100Mb, gigabit, 2.5GbE, 5GbE, 10GbE and up. */
#define SPEED_10                10
#define SPEED_100               100
#define SPEED_1000              1000
#define SPEED_2500              2500
#define SPEED_5000              5000
#define SPEED_10000             10000
#define SPEED_25000             25000
#define SPEED_40000             40000
#define SPEED_100000            100000

/* Duplex, half or full. */
#define DUPLEX_HALF             0x00
#define DUPLEX_FULL             0x01

/* Which connector port. */
#define PORT_TP                 0x00
#define PORT_AUI                0x01
#define PORT_MII                0x02
#define PORT_FIBRE              0x03
#define PORT_BNC                0x04

/* Which tranceiver to use. */
#define XCVR_INTERNAL           0x00
#define XCVR_EXTERNAL           0x01
#define XCVR_DUMMY1             0x02
#define XCVR_DUMMY2             0x03
#define XCVR_DUMMY3             0x04

#define AUTONEG_DISABLE         0x00
#define AUTONEG_ENABLE          0x01

bool load_interfaces(vector < string > &interfaces)
{
  vector < string > procnetdev;

  interfaces.clear();
  if (!loadfile("/proc/net/dev", procnetdev))
    return false;

  if (procnetdev.size() <= 2)
    return false;

// get rid of header (2 lines)
  procnetdev.erase(procnetdev.begin());
  procnetdev.erase(procnetdev.begin());

  for (unsigned int i = 0; i < procnetdev.size(); i++)
  {
// extract interfaces names
    size_t pos = procnetdev[i].find(':');

    if (pos != string::npos)
      interfaces.push_back(hw::strip(procnetdev[i].substr(0, pos)));
  }

  return true;
}

static int maclen(unsigned family = ARPHRD_ETHER)
{
  switch(family)
  {
    case ARPHRD_INFINIBAND:
      return 20;
    case ARPHRD_ETHER:
      return 6;
    default:
      return 14;
  }
}

static string getmac(const unsigned char *mac, unsigned family = ARPHRD_ETHER)
{
  char buff[5];
  string result = "";
  bool valid = false;

  for (int i = 0; i < maclen(family); i++)
  {
    snprintf(buff, sizeof(buff), "%02x", mac[i]);

    valid |= (mac[i] != 0);

    if (i == 0)
      result = string(buff);
    else
      result += ":" + string(buff);
  }

  if (valid)
    return result;
  else
    return "";
}


static const char *hwname(int t)
{
  switch (t)
  {
    case ARPHRD_ETHER:
      return _("Ethernet");
    case ARPHRD_SLIP:
      return _("SLIP");
    case ARPHRD_LOOPBACK:
      return _("loopback");
    case ARPHRD_FDDI:
      return _("FDDI");
    case ARPHRD_IEEE1394:
      return _("IEEE1394");
    case ARPHRD_IRDA:
      return _("IRDA");
    case ARPHRD_PPP:
      return _("PPP");
    case ARPHRD_X25:
      return _("X25");
    case ARPHRD_TUNNEL:
      return _("IPtunnel");
    case ARPHRD_DLCI:
      return _("Framerelay.DLCI");
    case ARPHRD_FRAD:
      return _("Framerelay.AD");
    case ARPHRD_TUNNEL6:
      return _("IP6tunnel");
    case ARPHRD_SIT:
      return _("IP6inIP4");
    default:
      return "";
  }
}


static string print_ip(struct sockaddr_in *in)
{
  return string(inet_ntoa(in->sin_addr));
}


static void scan_ip(hwNode & interface)
{
  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd >= 0)
  {
    struct ifreq ifr;

// get IP address
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface.getLogicalName().c_str());
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
    {
// IP address is in ifr.ifr_addr
      interface.setConfig("ip", ::enabled("output:sanitize")?REMOVED:print_ip((sockaddr_in *) (&ifr.ifr_addr)));
      strcpy(ifr.ifr_name, interface.getLogicalName().c_str());
      if ((interface.getConfig("point-to-point") == "yes")
        && (ioctl(fd, SIOCGIFDSTADDR, &ifr) == 0))
      {
// remote PPP address is in ifr.ifr_dstaddr
        interface.setConfig("remoteip",
          print_ip((sockaddr_in *) & ifr.ifr_dstaddr));
      }
    }

    close(fd);
  }
}


static bool isVirtual(const string & MAC)
{
  if (MAC.length() < 8)
    return false;

  string manufacturer = uppercase(MAC.substr(0, 8));

  if ((manufacturer == "00:05:69") ||
    (manufacturer == "00:0C:29") || (manufacturer == "00:50:56"))
    return true;	// VMware
  if (manufacturer == "00:1C:42")
    return true;	// Parallels
  if (manufacturer == "0A:00:27")
    return true;	// VirtualBox

  return false;
}


// Get data for connected transceiver module.
static void scan_module(hwNode & interface, int fd)
{
  struct ifreq ifr;
  struct ethtool_modinfo emodinfo;
  struct ethtool_eeprom eeeprom;

  emodinfo.cmd = ETHTOOL_GMODULEINFO;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, interface.getLogicalName().c_str());
  ifr.ifr_data = (caddr_t) &emodinfo;
  // Skip interface if module info not supported.
  if (ioctl(fd, SIOCETHTOOL, &ifr) != 0)
    return;

  eeeprom.cmd = ETHTOOL_GMODULEEEPROM;
  eeeprom.offset = 0;
  eeeprom.len = emodinfo.eeprom_len;
  if (eeeprom.len > MAX_EEPROM_SIZE)
    eeeprom.len = MAX_EEPROM_SIZE;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, interface.getLogicalName().c_str());
  ifr.ifr_data = (caddr_t) &eeeprom;
  if (ioctl(fd, SIOCETHTOOL, &ifr) != 0)
    return;

  switch (emodinfo.type)
  {
    /* SFF 8472 eeprom layout starts with same data as SFF 8079. */
    case ETH_MODULE_SFF_8079:
    case ETH_MODULE_SFF_8472:
      if ((eeeprom.data[0] == SFF_8024_ID_SOLDERED || eeeprom.data[0] == SFF_8024_ID_SFP) &&
          eeeprom.data[1] == SFF_8024_EXT_ID_DEFINED_BY_2WIRE_ID)
      {
        char buffer[32];
        /* Get part number (padded with space). String is stripped inside setConfig. */
        interface.setConfig("module", string((const char*)&eeeprom.data[40], 16));
        int wavelength = eeeprom.data[60] << 8 | eeeprom.data[61];
        /* Skip wavelength for SFP+ cables, they use byte 60 for other data. */
        if ((eeeprom.data[8] & 0x0C) == 0 && wavelength > 0)
        {
          snprintf(buffer, sizeof(buffer), "%dnm", wavelength);
          interface.setConfig("wavelength", buffer);
        }
        int max_length = 0;
        int length;
        length = eeeprom.data[14] * 1000; /* SMF, km */
        if (length > max_length) max_length = length;
        length = eeeprom.data[15] * 100; /* SMF, meter */
        if (length > max_length) max_length = length;
        length = eeeprom.data[16] * 10; /* 50um (OM2), meter */
        if (length > max_length) max_length = length;
        length = eeeprom.data[17] * 10; /* 62.5um (OM1), meter */
        if (length > max_length) max_length = length;
        length = eeeprom.data[18]; /* Copper, meter */
        if (length > max_length) max_length = length;
        length = eeeprom.data[19] * 10; /* OM3, meter */
        if (length > max_length) max_length = length;
        if (max_length > 0)
        {
          snprintf(buffer, sizeof(buffer), "%dm", max_length);
          interface.setConfig("maxlength", buffer);
        }
        switch (eeeprom.data[2])
        {
          case SFF_8024_CONNECTOR_SC:
            interface.setConfig("connector", "SC");
            break;
          case SFF_8024_CONNECTOR_LC:
            interface.setConfig("connector", "LC");
            break;
          case SFF_8024_CONNECTOR_OPTICAL_PIGTAIL:
            interface.setConfig("connector", "optical pigtail");
            break;
          case SFF_8024_CONNECTOR_COPPER_PIGTAIL:
            interface.setConfig("connector", "copper pigtail");
            break;
          case SFF_8024_CONNECTOR_RJ45:
            interface.setConfig("connector", "RJ45");
            break;
          case SFF_8024_CONNECTOR_NON_SEPARABLE:
            interface.setConfig("connector", "non separable");
            break;
        }
      }
      break;
  }
}


static void updateCapabilities(hwNode & interface, u32 supported, u32 supported2, u32 speed, u8 duplex, u8 port, u8 autoneg)
{
  if(supported & SUPPORTED_TP)
    interface.addCapability("tp", _("twisted pair"));
  if(supported & SUPPORTED_AUI)
    interface.addCapability("aui", _("AUI"));
  if(supported & SUPPORTED_BNC)
    interface.addCapability("bnc", _("BNC"));
  if(supported & SUPPORTED_MII)
    interface.addCapability("mii", _("Media Independent Interface"));
  if(supported & SUPPORTED_FIBRE)
    interface.addCapability("fibre", _("optical fibre"));
  if(supported & SUPPORTED_BACKPLANE)
    interface.addCapability("backplane", _("backplane"));
  if(supported & SUPPORTED_10baseT_Half)
  {
    interface.addCapability("10bt", _("10Mbit/s"));
    interface.setCapacity(10000000ULL);
  }
  if(supported & SUPPORTED_10baseT_Full)
  {
    interface.addCapability("10bt-fd", _("10Mbit/s (full duplex)"));
    interface.setCapacity(10000000ULL);
  }
  if(supported & SUPPORTED_100baseT_Half)
  {
    interface.addCapability("100bt", _("100Mbit/s"));
    interface.setCapacity(100000000ULL);
  }
  if(supported & SUPPORTED_100baseT_Full)
  {
    interface.addCapability("100bt-fd", _("100Mbit/s (full duplex)"));
    interface.setCapacity(100000000ULL);
  }
  if(supported & SUPPORTED_1000baseT_Half)
  {
    interface.addCapability("1000bt", "1Gbit/s");
    interface.setCapacity(1000000000ULL);
  }
  if(supported & SUPPORTED_1000baseT_Full)
  {
    interface.addCapability("1000bt-fd", _("1Gbit/s (full duplex)"));
    interface.setCapacity(1000000000ULL);
  }
  if((supported & SUPPORTED_1000baseKX_Full) || (supported2 & SUPPORTED2_1000baseX_Full))
  {
    interface.addCapability("1000bx-fd", _("1Gbit/s (full duplex)"));
    interface.setCapacity(1000000000ULL);
  }
  if(supported & SUPPORTED_2500baseX_Full)
  {
    interface.addCapability("2500bx-fd", _("2.5Gbit/s (full duplex)"));
    interface.setCapacity(2500000000ULL);
  }
  if(supported & SUPPORTED_2500baseT_Full)
  {
    interface.addCapability("2500bt-fd", _("2500Mbit/s (full duplex)"));
    interface.setCapacity(2500000000ULL);
  }
  if(supported & SUPPORTED_5000baseT_Full)
  {
    interface.addCapability("5000bt-fd", _("5Gbit/s (full duplex)"));
    interface.setCapacity(5000000000ULL);
  }
  if(supported2 & SUPPORTED2_2500baseT_Full)
  {
    interface.addCapability("2500bt-fd", _("2.5Gbit/s (full duplex)"));
    interface.setCapacity(2500000000ULL);
  }
  if(supported2 & SUPPORTED2_5000baseT_Full)
  {
    interface.addCapability("5000bt-fd", _("5Gbit/s (full duplex)"));
    interface.setCapacity(5000000000ULL);
  }
  if(supported & SUPPORTED_10000baseT_Full)
  {
    interface.addCapability("10000bt-fd", _("10Gbit/s (full duplex)"));
    interface.setCapacity(10000000000ULL);
  }
  if((supported & (SUPPORTED_10000baseKX4_Full | SUPPORTED_10000baseKR_Full)) ||
     (supported2 & (SUPPORTED2_10000baseCR_Full | SUPPORTED2_10000baseSR_Full | SUPPORTED2_10000baseLR_Full |
                    SUPPORTED2_10000baseLRM_Full | SUPPORTED2_10000baseER_Full)))
  {
    interface.addCapability("10000bx-fd", _("10Gbit/s (full duplex)"));
    interface.setCapacity(10000000000ULL);
  }
  if(supported & SUPPORTED_10000baseR_FEC)
  {
    interface.addCapability("10000bt-fd", _("10Gbit/s (full duplex)"));
    interface.setCapacity(10000000000ULL);
  }
  if(supported & SUPPORTED_10000baseCR_Full)
  {
    interface.addCapability("10000bt-fd", _("10Gbit/s (full duplex)"));
    interface.setCapacity(10000000000ULL);
  }
  if(supported & SUPPORTED_10000baseSR_Full)
  {
    interface.addCapability("10000bt-fd", _("10Gbit/s (full duplex)"));
    interface.setCapacity(10000000000ULL);
  }
  if(supported & SUPPORTED_10000baseLR_Full)
  {
    interface.addCapability("10000bt-fd", _("10Gbit/s (full duplex)"));
    interface.setCapacity(10000000000ULL);
  }
  if(supported & SUPPORTED_10000baseLRM_Full)
  {
    interface.addCapability("10000bt-fd", _("10Gbit/s (full duplex)"));
    interface.setCapacity(10000000000ULL);
  }
  if(supported & SUPPORTED_10000baseER_Full)
  {
    interface.addCapability("10000bt-fd", _("10Gbit/s (full duplex)"));
    interface.setCapacity(10000000000ULL);
  }
  if(supported & SUPPORTED_20000baseMLD2_Full)
  {
    interface.addCapability("20000bt-fd", _("20Gbit/s (full duplex)"));
    interface.setCapacity(20000000000ULL);
  }
  if(supported & SUPPORTED_20000baseKR2_Full)
  {
    interface.addCapability("20000bt-fd", _("20Gbit/s (full duplex)"));
    interface.setCapacity(20000000000ULL);
  }
  if((supported & SUPPORTED_25000baseCR_Full) ||
     (supported2 & (SUPPORTED2_25000baseKR_Full | SUPPORTED2_25000baseSR_Full)))
  {
    interface.addCapability("25000bx-fd", _("25Gbit/s (full duplex)"));
    interface.setCapacity(25000000000ULL);
  }
  if(supported & SUPPORTED_25000baseKR_Full)
  {
    interface.addCapability("25000bt-fd", _("25Gbit/s (full duplex)"));
    interface.setCapacity(25000000000ULL);
  }
  if(supported & SUPPORTED_25000baseSR_Full)
  {
    interface.addCapability("25000bt-fd", _("25Gbit/s (full duplex)"));
    interface.setCapacity(25000000000ULL);
  }
  if(supported & (SUPPORTED_40000baseKR4_Full | SUPPORTED_40000baseCR4_Full |
                  SUPPORTED_40000baseSR4_Full | SUPPORTED_40000baseLR4_Full))
  {
    interface.addCapability("40000bx-fd", _("40Gbit/s (full duplex)"));
    interface.setCapacity(40000000000ULL);
  }
  if(supported & (SUPPORTED_50000baseCR2_Full | SUPPORTED_50000baseKR2_Full |
                  SUPPORTED_50000baseSR2_Full))
  {
    interface.addCapability("50000bt-fd", _("50Gbit/s (full duplex)"));
    interface.setCapacity(50000000000ULL);
  }
  if(supported & (SUPPORTED_56000baseCR4_Full | SUPPORTED_56000baseSR4_Full |
                  SUPPORTED_56000baseLR4_Full | SUPPORTED_56000baseKR4_Full))
  {
    interface.addCapability("56000bt-fd", _("56Gbit/s (full duplex)"));
    interface.setCapacity(56000000000ULL);
  }
  if(supported2 & (SUPPORTED2_100000baseKR4_Full | SUPPORTED2_100000baseSR4_Full |
                   SUPPORTED2_100000baseCR4_Full | SUPPORTED2_100000baseLR4_ER4_Full))
  {
    interface.addCapability("100000bx-fd", _("100Gbit/s (full duplex)"));
    interface.setCapacity(100000000000ULL);
  }
  if(supported & SUPPORTED_Autoneg)
    interface.addCapability("autonegotiation", _("Auto-negotiation"));

  switch(speed)
  {
    case SPEED_10:
      interface.setConfig("speed", "10Mbit/s");
      interface.setSize(10000000ULL);
      break;
    case SPEED_100:
      interface.setConfig("speed", "100Mbit/s");
      interface.setSize(100000000ULL);
      break;
    case SPEED_1000:
      interface.setConfig("speed", "1Gbit/s");
      interface.setSize(1000000000ULL);
      break;
    case SPEED_2500:
      interface.setConfig("speed", "2.5Gbit/s");
      interface.setSize(2500000000ULL);
      break;
    case SPEED_5000:
      interface.setConfig("speed", "5Gbit/s");
      interface.setSize(5000000000ULL);
      break;
    case SPEED_10000:
      interface.setConfig("speed", "10Gbit/s");
      interface.setSize(10000000000ULL);
      break;
    case SPEED_25000:
      interface.setConfig("speed", "25Gbit/s");
      interface.setSize(25000000000ULL);
      break;
    case SPEED_40000:
      interface.setConfig("speed", "40Gbit/s");
      interface.setSize(40000000000ULL);
      break;
    case SPEED_100000:
      interface.setConfig("speed", "100Gbit/s");
      interface.setSize(100000000000ULL);
      break;
  }
  switch(duplex)
  {
    case DUPLEX_HALF:
      interface.setConfig("duplex", "half");
      break;
    case DUPLEX_FULL:
      interface.setConfig("duplex", "full");
      break;
  }
  switch(port)
  {
    case PORT_TP:
      interface.setConfig("port", "twisted pair");
      break;
    case PORT_AUI:
      interface.setConfig("port", "AUI");
      break;
    case PORT_BNC:
      interface.setConfig("port", "BNC");
      break;
    case PORT_MII:
      interface.setConfig("port", "MII");
      break;
    case PORT_FIBRE:
      interface.setConfig("port", "fibre");
      break;
  }
  interface.setConfig("autonegotiation", (autoneg == AUTONEG_DISABLE) ?  "off" : "on");
}


static void scan_modes(hwNode & interface, int fd)
{
  struct ifreq ifr;
  struct ethtool_cmd ecmd;
  struct ethtool_link_settings elink;
  s8 mask_size;

  elink.cmd = ETHTOOL_GLINKSETTINGS;
  elink.link_mode_masks_nwords = 0;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, interface.getLogicalName().c_str());
  ifr.ifr_data = (caddr_t) &elink;
  // Probe link mode mask count.
  if (ioctl(fd, SIOCETHTOOL, &ifr) == 0)
  {
    mask_size = -elink.link_mode_masks_nwords;
    if (mask_size > 1 && mask_size <= MAX_LINK_MODE_MASK_SIZE)
    {
      elink.cmd = ETHTOOL_GLINKSETTINGS;
      elink.link_mode_masks_nwords = mask_size;
      memset(&ifr, 0, sizeof(ifr));
      strcpy(ifr.ifr_name, interface.getLogicalName().c_str());
      ifr.ifr_data = (caddr_t) &elink;
      // Read link mode settings.
      if (ioctl(fd, SIOCETHTOOL, &ifr) == 0)
      {
        updateCapabilities(interface, elink.link_mode_masks[0], elink.link_mode_masks[1],
          elink.speed, elink.duplex, elink.port, elink.autoneg);
        return;
      }
    }
  }

  ecmd.cmd = ETHTOOL_GSET;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, interface.getLogicalName().c_str());
  ifr.ifr_data = (caddr_t) &ecmd;
  if (ioctl(fd, SIOCETHTOOL, &ifr) == 0)
  {
    updateCapabilities(interface, ecmd.supported, 0, ecmd.speed, ecmd.duplex, ecmd.port, ecmd.autoneg);
  }
}


bool scan_network(hwNode & n)
{
  vector < string > interfaces;
  char buffer[2 * IFNAMSIZ + 1];

  if (!load_interfaces(interfaces))
    return false;

  int fd = socket(PF_INET, SOCK_DGRAM, 0);

  if (fd >= 0)
  {
    struct ifreq ifr;
    struct ethtool_drvinfo drvinfo;
    struct ethtool_value edata;

    for (unsigned int i = 0; i < interfaces.size(); i++)
    {
      hwNode *existing;
      hwNode interface("network",
        hw::network);

      interface.setLogicalName(interfaces[i]);
      interface.claim();
      interface.addHint("icon", string("network"));

      string businfo = sysfs::entry::byClass("net", interface.getLogicalName()).leaf().businfo();
      if (businfo!="")
        interface.setBusInfo(businfo);
      interface.setModalias(sysfs::entry::byClass("net", interface.getLogicalName()).leaf().modalias());

//scan_mii(fd, interface);
      scan_ip(interface);

      memset(&ifr, 0, sizeof(ifr));
      strcpy(ifr.ifr_name, interfaces[i].c_str());
      if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
      {
#ifdef IFF_PORTSEL
        if (ifr.ifr_flags & IFF_PORTSEL)
        {
          if (ifr.ifr_flags & IFF_AUTOMEDIA)
            interface.setConfig("automedia", "yes");
        }
#endif

        if (ifr.ifr_flags & IFF_UP)
          interface.enable();
        else
          interface.disable();
        if (ifr.ifr_flags & IFF_BROADCAST)
          interface.setConfig("broadcast", "yes");
        if (ifr.ifr_flags & IFF_DEBUG)
          interface.setConfig("debug", "yes");
        if (ifr.ifr_flags & IFF_LOOPBACK)
          interface.setConfig("loopback", "yes");
        if (ifr.ifr_flags & IFF_POINTOPOINT)
          interface.setConfig("point-to-point", "yes");
        if (ifr.ifr_flags & IFF_PROMISC)
          interface.setConfig("promiscuous", "yes");
        if (ifr.ifr_flags & IFF_SLAVE)
          interface.setConfig("slave", "yes");
        if (ifr.ifr_flags & IFF_MASTER)
          interface.setConfig("master", "yes");
        if (ifr.ifr_flags & IFF_MULTICAST)
          interface.setConfig("multicast", "yes");
      }

      memset(&ifr, 0, sizeof(ifr));
      strcpy(ifr.ifr_name, interfaces[i].c_str());
// get MAC address
      if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
      {
        string hwaddr = getmac((unsigned char *) ifr.ifr_hwaddr.sa_data, ifr.ifr_hwaddr.sa_family);
        interface.addCapability(hwname(ifr.ifr_hwaddr.sa_family));
        if (ifr.ifr_hwaddr.sa_family >= 256)
          interface.addCapability("logical", _("Logical interface"));
        else
          interface.addCapability("physical", _("Physical interface"));
        interface.setDescription(string(hwname(ifr.ifr_hwaddr.sa_family)) +
          " interface");
        interface.setSerial(hwaddr);

        if (isVirtual(interface.getSerial()))
          interface.addCapability("logical", _("Logical interface"));
      }

// check for wireless extensions
      memset(buffer, 0, sizeof(buffer));
      strncpy(buffer, interfaces[i].c_str(), sizeof(buffer));
      if (ioctl(fd, SIOCGIWNAME, &buffer) == 0)
      {
        interface.addCapability("wireless", _("Wireless-LAN"));
        interface.setConfig("wireless", hw::strip(buffer + IFNAMSIZ));
        interface.setDescription(_("Wireless interface"));
        interface.addHint("icon", string("wifi"));
        interface.addHint("bus.icon", string("radio"));
      }

      edata.cmd = ETHTOOL_GLINK;
      memset(&ifr, 0, sizeof(ifr));
      strcpy(ifr.ifr_name, interfaces[i].c_str());
      ifr.ifr_data = (caddr_t) &edata;
      if (ioctl(fd, SIOCETHTOOL, &ifr) == 0)
      {
        interface.setConfig("link", edata.data ? "yes":"no");
      }

      scan_modes(interface, fd);
      scan_module(interface, fd);

      drvinfo.cmd = ETHTOOL_GDRVINFO;
      memset(&ifr, 0, sizeof(ifr));
      strcpy(ifr.ifr_name, interfaces[i].c_str());
      ifr.ifr_data = (caddr_t) & drvinfo;
      if (ioctl(fd, SIOCETHTOOL, &ifr) == 0)
      {
        interface.setConfig("driver", drvinfo.driver);
        interface.setConfig("driverversion", drvinfo.version);
        interface.setConfig("firmware", drvinfo.fw_version);
        if (interface.getBusInfo() == "")
          interface.setBusInfo(guessBusInfo(drvinfo.bus_info));
      }

      if(sysfs::entry::byClass("net", interface.getLogicalName()).isvirtual())
        interface.addCapability("logical", _("Logical interface"));

      existing = n.findChildByBusInfo(interface.getBusInfo());
      // Multiple NICs can exist on one PCI function.

      if (existing && !existing->getBusInfo().empty() && (interface.getBusInfo() == existing->getBusInfo()) && interface.getProduct().empty())
        interface.setProduct(existing->getProduct());

      // Only merge if MACs also match.
      if (existing && (existing->getSerial() == "" || interface.getSerial() == existing->getSerial()))
      {
        existing->merge(interface);
        if(interface.getDescription()!="")
          existing->setDescription(interface.getDescription());
      }
      else
      {
        existing = n.findChildByLogicalName(interface.getLogicalName());
        if (existing)
        {
          existing->merge(interface);
        }
        else
        {
// we don't care about loopback and "logical" interfaces
          if (!interface.isCapable("loopback") &&
            !interface.isCapable("logical"))
            n.addChild(interface);
        }
      }
    }

    close(fd);
    return true;
  }
  else
    return false;
}
