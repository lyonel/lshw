/*
 * usb.cc
 *
 */

#include "usb.h"
#include "osutils.h"
#include "heuristics.h"
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

#define PROCBUSUSBDEVICES "/proc/bus/usb/devices"
#define USBID_PATH "/usr/share/lshw/usb.ids:/usr/local/share/usb.ids:/usr/share/usb.ids:/etc/usb.ids:/usr/share/hwdata/usb.ids:/usr/share/misc/usb.ids"

#define USB_CLASS_PER_INTERFACE         0	/* for DeviceClass */
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_PRINTER               7
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9
#define USB_CLASS_DATA                  10
#define USB_CLASS_VENDOR_SPEC           0xff

static map<u_int16_t,string> usbvendors;
static map<u_int32_t,string> usbproducts;

#define PRODID(x, y) ((x << 16) + y)

static char *id = "@(#) $Id$";

static string usbhost(unsigned bus)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "usb%u", bus);

  return string(buffer);
}

static string usbhandle(unsigned bus, unsigned level, unsigned dev)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "USB:%u:%u", bus, dev);

  return string(buffer);
}

static string usbbusinfo(unsigned bus, unsigned level, unsigned port)
{
  char buffer[10];

  if(level>0)
    snprintf(buffer, sizeof(buffer), "usb@%u:%u", bus, port);
  else
    snprintf(buffer, sizeof(buffer), "usb@%u", bus);

  return string(buffer);
}

static string usbspeed(float speed)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "%.1fMB/s", speed);

  return string(buffer);
}

static bool addUSBChild(hwNode & n, hwNode & device, unsigned bus, unsigned lev, unsigned prnt)
{
  hwNode * parent = NULL;

  if(prnt>0) parent = n.findChildByHandle(usbhandle(bus, lev-1, prnt));
  if(parent)
  {
    device.setBusInfo(parent->getBusInfo()+":"+device.getPhysId());
    parent->addChild(device);
    return true;
  }
  else
  {
    // USB host
    {
      string businfo = guessBusInfo(device.getSerial());
      parent = n.findChildByBusInfo(businfo);
      if(!parent)	// still no luck
      {
        unsigned long long ioport = strtoll(device.getSerial().c_str(), NULL, 16);
        parent = n.findChildByResource(hw::resource::ioport(ioport, ioport));
      }
      device.setSerial("");	// serial# has no meaning for USB hosts
    }
    if(parent)
    {
      parent->addChild(device);
      return true;
    }
    else
      n.addChild(device);
    return false;
  }
}

static bool setUSBClass(hwNode & device, unsigned cls)
{
  if(device.getClass()!=hw::generic) return false;
  switch(cls)
  {
    case USB_CLASS_AUDIO:
      device.setClass(hw::multimedia);
      device.setDescription("Audio device");
      break;
    case USB_CLASS_COMM:
      device.setClass(hw::communication);
      device.setDescription("Communication device");
      break;
    case USB_CLASS_HID:
      device.setClass(hw::input);
      device.setDescription("Human interface device");
      break;
    case USB_CLASS_PRINTER:
      device.setClass(hw::printer);
      device.setDescription("Printer");
      break;
    case USB_CLASS_MASS_STORAGE:
      device.setClass(hw::disk);	// hw::storage is for storage controllers
      device.setDescription("Mass storage device");
      break;
    case USB_CLASS_HUB:
      device.setClass(hw::bus);
      device.setDescription("USB hub");
      break;
    case USB_CLASS_DATA:
      device.setClass(hw::generic);
      break;
    default:
      return false;
  }

  return true;
}

static bool describeUSB(hwNode & device, unsigned vendor, unsigned prodid)
{
  if(usbvendors.find(vendor)==usbvendors.end()) return false;

  device.setVendor(usbvendors[vendor]);

  if(usbproducts.find(PRODID(vendor, prodid))!=usbproducts.end())
    device.setProduct(usbproducts[PRODID(vendor, prodid)]);

  return true;
}

static bool load_usbids(const string & name)
{
  FILE * usbids = NULL;
  u_int16_t vendorid = 0;

  usbids = fopen(name.c_str(), "r");
  if(!usbids) return false;

  while(!feof(usbids))
  {
    char * buffer = NULL;
    size_t linelen;
    unsigned t = 0;
    char description[100];

    if(getline(&buffer, &linelen, usbids)>0)
    {
      if(buffer[linelen-1]<' ')
        buffer[linelen-1] = '\0';	// chop \n
      string line = string(buffer);
      free(buffer);
      memset(description, 0, sizeof(description));
      
      if(line.length()>0)
      {
        if(line[0] == '\t')	// product id entry
        {
          if(sscanf(line.c_str(), "%x %[ -z]", &t, description)>0)
          {
            usbproducts[PRODID(vendorid,t)] = hw::strip(description);
          }
        }
        else			// vendor id entry
        {
          if(sscanf(line.c_str(), "%x %[ -z]", &t, description)>0)
          {
            vendorid = t;
            usbvendors[t] = hw::strip(description);
          }
        }
      }
    }
  }

  fclose(usbids);

  return true;
}

bool scan_usb(hwNode & n)
{
  hwNode device("device");
  FILE * usbdevices = NULL;
  bool defined = false;
  unsigned int bus,lev,prnt,port,cnt,devnum,mxch;
  float speed;
  char ver[10];
  unsigned int cls, sub, prot, mxps, numcfgs;
  unsigned int vendor, prodid;
  char rev[10];
  unsigned numifs, cfgnum, atr;
  char mxpwr[10];
  unsigned ifnum, alt, numeps;
  char driver[80];

  if (!exists(PROCBUSUSBDEVICES))
    return false;

  vector < string > filenames;
  splitlines(USBID_PATH, filenames, ':');
  for (int i = filenames.size() - 1; i >= 0; i--)
  {
    load_usbids(filenames[i]);
  }
  filenames.clear();

  usbdevices = fopen(PROCBUSUSBDEVICES, "r");

  while(!feof(usbdevices))
  {
    char * buffer = NULL;
    size_t linelen;
    char strname[80];
    char strval[80];

    if(getline(&buffer, &linelen, usbdevices)>0)
    {
      string line = hw::strip(string(buffer));
      free(buffer);

      if(line.length()<=0)
      {
        if(defined)
          addUSBChild(n, device, bus, lev, prnt);
        device = hwNode("device");
        defined = false;
      }
      else
      {
        if((line.length()>1) && (line[1] == ':'))
        switch(line[0])
        {
          case 'T':
            bus = lev = prnt = port = cnt = devnum = mxch = 0;
            speed = 0.0;
	    strcpy(ver, "");
	    strcpy(rev, "");
            cls = sub = prot = mxps = numcfgs = 0;
            vendor = prodid = 0;
            if(sscanf(line.c_str(), "T: Bus=%u Lev=%u Prnt=%u Port=%u Cnt=%u Dev#=%u Spd=%f MxCh=%u", &bus, &lev, &prnt, &port, &cnt, &devnum, &speed, &mxch)>0)
            {
              defined = true;
              if(lev==0)
              {
                device = hwNode("usbhost", hw::bus);
                device.claim();
                device.setLogicalName(usbhost(bus));
              }
              else
                device = hwNode("usb");
              device.setHandle(usbhandle(bus, lev, devnum));
              device.setBusInfo(usbbusinfo(bus, lev, port));
              device.setPhysId(port);
              device.setConfig("speed", usbspeed(speed));
              if(mxch>0)
              {
                snprintf(strval, sizeof(strval), "%u", mxch);
                device.setConfig("slots", strval);
              }
            }
            break;
          case 'D':
            strcpy(ver, "");
            cls = sub = prot = mxps = numcfgs = 0;
            if(sscanf(line.c_str(), "D: Ver=%s Cls=%x(%*5c) Sub=%x Prot=%x MxPS=%u #Cfgs=%u", ver, &cls, &sub, &prot, &mxps, &numcfgs)>0)
            {
              setUSBClass(device, cls);
              device.addCapability(string("usb-")+string(ver));
            }
            break;
          case 'P':
            vendor = prodid = 0;
            strcpy(rev, "");
            if(sscanf(line.c_str(), "P: Vendor=%x ProdID=%x Rev=%s", &vendor, &prodid, rev)>0)
            {
              describeUSB(device, vendor, prodid);
              device.setVersion(hw::strip(rev));
            }
            break;
          case 'S':
            memset(strname, 0, sizeof(strname));
            memset(strval, 0, sizeof(strval));
            if(sscanf(line.c_str(), "S: %[^=]=%[ -z]", strname, strval)>0)
            {
              if(strcasecmp(strname, "Manufacturer")==0)
                device.setVendor(hw::strip(strval));
              if(strcasecmp(strname, "Product")==0)
                device.setProduct(hw::strip(strval));
              if(strcasecmp(strname, "SerialNumber")==0)
                device.setSerial(hw::strip(strval));
            }
            break;
          case 'C':
            numifs = cfgnum = atr = 0;
            strcpy(mxpwr, "");
            if(sscanf(line.c_str(), "C:* #Ifs=%u Cfg#=%u Atr=%x MxPwr=%s", &numifs, &cfgnum, &atr, mxpwr)>0)
            {
              device.setConfig("maxpower", mxpwr);
            }
            break;
          case 'I':
            ifnum = alt = numeps = cls = sub = prot = 0;
            memset(driver, 0, sizeof(driver));
            if((sscanf(line.c_str(), "I: If#=%u Alt=%u #EPs=%u Cls=%x(%*5c) Sub=%x Prot=%x Driver=%[ -z]", &ifnum, &alt, &numeps, &cls, &sub, &prot, driver)>0) && (cfgnum>0))
            {
               setUSBClass(device, cls);
               if((strlen(driver)!=0) && (strcasecmp("(none)", driver)!=0))
               {
                 device.setConfig("driver", hw::strip(driver));
                 device.claim();
               }
            }
            break;
        }
      }
    }
  }
  if(defined)
    addUSBChild(n, device, bus, lev, prnt);

  if(usbdevices) fclose(usbdevices);

  (void) &id;			// avoid warning "id defined but not used"

  return true;
}
