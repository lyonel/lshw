#include "network.h"
#include "osutils.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>

using namespace std;

bool load_interfaces(vector < string > &interfaces)
{
  vector < string > procnetdev;

  interfaces.clear();
  if (!loadfile("/proc/net/dev", procnetdev))
    return false;

  if (procnetdev.size() <= 2)
    return false;

  procnetdev.erase(procnetdev.begin());
  procnetdev.erase(procnetdev.begin());

  for (unsigned int i = 0; i < procnetdev.size(); i++)
  {
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

bool scan_network(hwNode & n)
{
  vector < string > interfaces;

  if (!load_interfaces(interfaces))
    return false;

  int fd = socket(PF_INET, SOCK_DGRAM, 0);

  if (fd > 0)
  {
    struct ifreq ifr;

    for (unsigned int i = 0; i < interfaces.size(); i++)
    {
      printf("%d ->%s<-\n", i, interfaces[i].c_str());
      memset(&ifr, 0, sizeof(ifr));
      strcpy(ifr.ifr_name, interfaces[i].c_str());

      if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
      {
	string hwaddr = getmac((unsigned char *) ifr.ifr_hwaddr.sa_data);
	printf("%s\n", hwaddr.c_str());
      }
      else
	perror("ioctl");
    }

    close(fd);
    return true;
  }
  else
    return false;
}

static char *id = "@(#) $Id: network.cc,v 1.1 2003/06/05 11:33:59 ezix Exp $";
