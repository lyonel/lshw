#include "version.h"
#include "pcmcia.h"
#include "osutils.h"
#include "sysfs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

__ID("@(#) $Id$");

#define SYS_CLASS_PCMCIASOCKET  "/sys/class/pcmcia_socket"
#define CLASS_PCMCIASOCKET  "pcmcia_socket"

bool scan_pcmcia(hwNode & n)
{
  bool result = false;
  hwNode *core = n.getChild("core");
  int count = 0;
  dirent **sockets = NULL;

  return result;

  if (!core)
  {
    n.addChild(hwNode("core", hw::bus));
    core = n.getChild("core");
  }

  if(!pushd(SYS_CLASS_PCMCIASOCKET))
    return false;

  count = scandir(".", &sockets, NULL, alphasort);
  if(count>=0)
  {
    for(int i=0; i<count; i++)
    {
      if(matches(sockets[i]->d_name, "^pcmcia_socket[[:digit:]]+$"))
      {
        sysfs::entry socket = sysfs::entry::byClass(CLASS_PCMCIASOCKET, sockets[i]->d_name);
        printf("found PCMCIA socket: %s\n", sockets[i]->d_name);
      }
      free(sockets[i]);
    }
    free(sockets);
  }

  popd();
  return result;
}
