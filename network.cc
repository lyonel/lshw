#include "network.h"
#include "osutils.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/types.h>

using namespace std;

#ifndef SIOCETHTOOL
#define SIOCETHTOOL     0x8946
#endif

typedef unsigned long long u64;
typedef __uint32_t u32;
typedef __uint16_t u16;
typedef __uint8_t u8;

struct ethtool_cmd
{
  u32 cmd;
  u32 supported;		/* Features this interface supports */
  u32 advertising;		/* Features this interface advertises */
  u16 speed;			/* The forced speed, 10Mb, 100Mb, gigabit */
  u8 duplex;			/* Duplex, half or full */
  u8 port;			/* Which connector port */
  u8 phy_address;
  u8 transceiver;		/* Which tranceiver to use */
  u8 autoneg;			/* Enable or disable autonegotiation */
  u32 maxtxpkt;			/* Tx pkts before generating tx int */
  u32 maxrxpkt;			/* Rx pkts before generating rx int */
  u32 reserved[4];
};

#define ETHTOOL_BUSINFO_LEN     32
/* these strings are set to whatever the driver author decides... */
struct ethtool_drvinfo
{
  u32 cmd;
  char driver[32];		/* driver short name, "tulip", "eepro100" */
  char version[32];		/* driver version string */
  char fw_version[32];		/* firmware version string, if applicable */
  char bus_info[ETHTOOL_BUSINFO_LEN];	/* Bus info for this IF. */
  /*
   * For PCI devices, use pci_dev->slot_name. 
   */
  char reserved1[32];
  char reserved2[16];
  u32 n_stats;			/* number of u64's from ETHTOOL_GSTATS */
  u32 testinfo_len;
  u32 eedump_len;		/* Size of data from ETHTOOL_GEEPROM (bytes) */
  u32 regdump_len;		/* Size of data from ETHTOOL_GREGS (bytes) */
};

/* CMDs currently supported */
#define ETHTOOL_GSET            0x00000001	/* Get settings. */
#define ETHTOOL_GDRVINFO        0x00000003	/* Get driver info. */

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

static string getmac(const unsigned char *mac)
{
  char buff[5];
  string result = "";

  for (int i = 0; i < 6; i++)
  {
    snprintf(buff, sizeof(buff), "%02x", mac[i]);

    if (i == 0)
      result = string(buff);
    else
      result += ":" + string(buff);
  }

  return result;
}

static const char *hwname(int t)
{
  switch (t)
  {
  case ARPHRD_ETHER:
    return "ethernet";
  case ARPHRD_SLIP:
    return "slip";
  case ARPHRD_LOOPBACK:
    return "loopback";
  case ARPHRD_FDDI:
    return "fddi";
  case ARPHRD_IRDA:
    return "irda";
  case ARPHRD_PPP:
    return "ppp";
  case ARPHRD_X25:
    return "x25";
  case ARPHRD_TUNNEL:
    return "iptunnel";
  case ARPHRD_DLCI:
    return "framerelay.dlci";
  case ARPHRD_FRAD:
    return "framerelay.ad";
  default:
    return "";
  }
}

bool scan_network(hwNode & n)
{
  vector < string > interfaces;

  if (!load_interfaces(interfaces))
    return false;

  int fd = socket(PF_INET, SOCK_DGRAM, 0);

  if (fd > 0)
  {
    struct ifreq ifr;
    struct ethtool_drvinfo drvinfo;

    for (unsigned int i = 0; i < interfaces.size(); i++)
    {
      hwNode interface("network",
		       hw::network);

      interface.setLogicalName(interfaces[i]);
      interface.claim();

      memset(&ifr, 0, sizeof(ifr));
      strcpy(ifr.ifr_name, interfaces[i].c_str());

      // get MAC address
      if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
      {
	string hwaddr = getmac((unsigned char *) ifr.ifr_hwaddr.sa_data);
	interface.addCapability(hwname(ifr.ifr_hwaddr.sa_family));
	interface.setSerial(hwaddr);
      }

      drvinfo.cmd = ETHTOOL_GDRVINFO;
      ifr.ifr_data = (caddr_t) & drvinfo;
      if (ioctl(fd, SIOCETHTOOL, &ifr) == 0)
      {
	interface.setConfig("driver", drvinfo.driver);
	interface.setConfig("driverversion", drvinfo.version);
	interface.setConfig("firmware", drvinfo.fw_version);
	interface.setBusInfo(drvinfo.bus_info);
      }

      if (hwNode * existing = n.findChildByBusInfo(interface.getBusInfo()))
      {
	existing->merge(interface);
      }
      else
	n.addChild(interface);
    }

    close(fd);
    return true;
  }
  else
    return false;
}

static char *id = "@(#) $Id: network.cc,v 1.3 2003/06/13 07:15:15 ezix Exp $";
