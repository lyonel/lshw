#include "pcmcia.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

/* parts of this code come from the excellent pcmcia-cs package written by
 * David A. Hinds <dahinds@users.sourceforge.net>.
 */

#define MAX_SOCK 8

static int lookup_dev(char *name)
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
}				/* lookup_dev */

static int open_dev(dev_t dev)
{
  static char *paths[] = {
    "/var/lib/pcmcia", "/var/run", "/dev", "/tmp", NULL
  };
  char **p, fn[64];
  int fd;

  for (p = paths; *p; p++)
  {
    sprintf(fn, "%s/ci-%d", *p, getpid());
    if (mknod(fn, (S_IFCHR | S_IREAD), dev) == 0)
    {
      fd = open(fn, O_RDONLY);
      unlink(fn);
      if (fd >= 0)
	return fd;
    }
  }
  return -1;
}				/* open_dev */

bool scan_pcmcia(hwNode & n)
{
  int major = lookup_dev("pcmcia");

  if (major < 0)
    return false;

  for (int i = 0; i < MAX_SOCK; i++)
  {
    int fd = open_dev((dev_t) (major << 8 + i));

    if (fd < 0)
      break;

    close(fd);
  }

  return true;
}

static char *id = "@(#) $Id: pcmcia.cc,v 1.1 2003/02/09 14:43:44 ezix Exp $";
