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

static char *id = "@(#) $Id: ide.cc,v 1.2 2003/02/05 09:32:40 ezix Exp $";
