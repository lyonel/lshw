/*
 * usb.cc
 *
 */

#include "usb.h"
#include "osutils.h"
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

static string usbhandle(unsigned bus, unsigned level, unsigned dev)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "USB:%u:%u", bus, dev);

  return string(buffer);
}

static string usbbusinfo(unsigned bus, unsigned port)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "usb@%u:%u", bus, port);

  return string(buffer);
}

bool scan_usb(hwNode & n)
{
  hwNode device("device");
  FILE * usbdevices = NULL;
  bool defined = false;

  if (!exists(PROCBUSUSBDEVICES))
    return false;

  usbdevices = fopen(PROCBUSUSBDEVICES, "r");

  while(!feof(usbdevices))
  {
    char * buffer = NULL;
    unsigned int bus,lev,prnt,port,cnt,devnum,mxch;
    size_t linelen;
    float speed;
    char ver[10];
    unsigned int cls, sub, prot, mxps, numcfgs;
    unsigned int vendor, prodid;
    char rev[10];
    char strname[80];
    char strval[80];

    if(getline(&buffer, &linelen, usbdevices)>0)
    {
      string line = hw::strip(string(buffer));
      free(buffer);
      
      if(line.length()<=0)
      {
        if(defined)
        {
          n.addChild(device);
        }
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
                device = hwNode("usbhost", hw::bus);
              else
                device = hwNode("usb");
              device.setHandle(usbhandle(bus, lev, devnum));
              device.setBusInfo(usbbusinfo(bus, port));
              device.setPhysId(port);
            }
            break;
          case 'D':
            strcpy(ver, "");
            cls = sub = prot = mxps = numcfgs = 0;
            if(sscanf(line.c_str(), "D: Ver=%s Cls=%x(%*5c) Sub=%x Prot=%x MxPS=%u #Cfgs=%u", ver, &cls, &sub, &prot, &mxps, &numcfgs)>0)
            {
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
              }
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
        }
      }
    }
  }
  if(defined)
  {
    n.addChild(device);
  }

  if(usbdevices) fclose(usbdevices);

  (void) &id;			// avoid warning "id defined but not used"

  return true;
}
