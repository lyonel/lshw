/*
 * usb.cc
 *
 */

#include "usb.h"
#include "osutils.h"
#include "heuristics.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

#define PROCBUSUSBDEVICES "/proc/bus/usb/devices"

#define USB_CLASS_PER_INTERFACE         0	/* for DeviceClass */
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_PRINTER               7
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9
#define USB_CLASS_DATA                  10
#define USB_CLASS_VENDOR_SPEC           0xff

#define USB_DIR_OUT                     0
#define USB_DIR_IN                      0x80

/*
 * Descriptor types
 */
#define USB_DT_DEVICE                   0x01
#define USB_DT_CONFIG                   0x02
#define USB_DT_STRING                   0x03
#define USB_DT_INTERFACE                0x04
#define USB_DT_ENDPOINT                 0x05

#define USB_DT_HID                      (USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT                   (USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL                 (USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB                      (USB_TYPE_CLASS | 0x09)

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
    if(lev==0)		// USB host
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
              //device.setSpeed(speed*1.0E6, "B/s");
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
