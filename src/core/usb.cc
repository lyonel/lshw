/*
 * usb.cc
 *
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "version.h"
#include "config.h"
#include "usb.h"
#include "osutils.h"
#include "heuristics.h"
#include "options.h"
#include <stdio.h>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#define PROCBUSUSBDEVICES "/proc/bus/usb/devices"
#define SYSKERNELDEBUGUSBDEVICES "/sys/kernel/debug/usb/devices"
#define USBID_PATH DATADIR"/usb.ids:/usr/share/lshw/usb.ids:/usr/local/share/usb.ids:/usr/share/usb.ids:/etc/usb.ids:/usr/share/hwdata/usb.ids:/usr/share/misc/usb.ids"

#define USB_CLASS_PER_INTERFACE         0         /* for DeviceClass */
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_IMAGING               6
#define USB_CLASS_PRINTER               7
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9
#define USB_CLASS_DATA                  0xa
#define USB_CLASS_SMARTCARD             0xb
#define USB_CLASS_VIDEO                 0xe
#define USB_CLASS_WIRELESS              0xe0
#define USB_CLASS_VENDOR_SPEC           0xff

#define USB_SC_AUDIOCONTROL   1
#define USB_SC_AUDIOSTREAMING   2
#define USB_SC_AUDIOMIDISTREAMING 3
#define USB_SC_COMMMODEM    2
#define USB_SC_COMMETHERNET   6
#define USB_SC_COMMOBEX     0xb
#define USB_SC_HIDNONE      0
#define USB_SC_HIDBOOT      1
#define USB_PROT_HIDKBD     1
#define USB_PROT_HIDMOUSE   2
#define USB_SC_PRINTER      1
#define USB_PROT_PRINTERUNIDIR    1
#define USB_PROT_PRINTERBIDIR   2
#define USB_PROT_PRINTER1284    3
#define USB_SC_STORAGERBC   1
#define USB_SC_STORAGEATAPI   2
#define USB_SC_STORAGEFLOPPY    4
#define USB_SC_STORAGESCSI    6
#define USB_SC_WIRELESSRADIO    1
#define USB_PROT_BLUETOOTH    1

static map<u_int16_t,string> usbvendors;
static map<u_int32_t,string> usbproducts;

#define PRODID(x, y) ((x << 16) + y)

__ID("@(#) $Id$");

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
  char buffer[15];

  snprintf(buffer, sizeof(buffer), "%.fMbit/s", speed);

  return string(buffer);
}


static bool addUSBChild(hwNode & n, hwNode & device, unsigned bus, unsigned lev, unsigned prnt)
{
  hwNode * parent = NULL;

  device.addHint("bus.icon", string("usb"));

  if(prnt>0) parent = n.findChildByHandle(usbhandle(bus, lev-1, prnt));
  if(parent)
  {
    if(parent->getBusInfo().find(":")==string::npos)
      device.setBusInfo(parent->getBusInfo()+":"+device.getPhysId());
    else
      device.setBusInfo(parent->getBusInfo()+"."+device.getPhysId());
    parent->addChild(device);
    return true;
  }
  else
  {
// USB host
    {
      string businfo = guessBusInfo(device.getSerial());
      parent = n.findChildByBusInfo(businfo);
      if(!parent)                                 // still no luck
      {
        unsigned long long ioport = strtoll(device.getSerial().c_str(), NULL, 16);
        parent = n.findChildByResource(hw::resource::ioport(ioport, ioport));
      }
      device.setSerial("");                       // serial# has no meaning for USB hosts
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


static bool setUSBClass(hwNode & device, unsigned cls, unsigned sub, unsigned prot)
{
  if(device.getClass()!=hw::generic) return false;
  switch(cls)
  {
    case USB_CLASS_AUDIO:
      device.setClass(hw::multimedia);
      device.setDescription(_("Audio device"));
      switch(sub)
      {
        case USB_SC_AUDIOCONTROL:
          device.addCapability("audio-control", _("Control device"));
          break;
        case USB_SC_AUDIOMIDISTREAMING:
          device.addCapability("midi", _("MIDI"));
        case USB_SC_AUDIOSTREAMING:
          device.addCapability("audio-streaming", _("Audio streaming"));
          break;
      }
      break;
    case USB_CLASS_COMM:
      device.setClass(hw::communication);
      device.setDescription(_("Communication device"));
      if(sub == USB_SC_COMMMODEM)
      {
        device.setDescription(_("Modem"));
        if((prot>=1) && (prot<=6)) device.addCapability("atcommands", _("AT (Hayes) compatible"));
      }
      if(sub==USB_SC_COMMETHERNET) device.addCapability("ethernet", _("Ethernet networking"));
      if(sub==USB_SC_COMMOBEX) device.addCapability("obex", _("OBEX networking"));
      break;
    case USB_CLASS_HID:
      device.setClass(hw::input);
      device.setDescription(_("Human interface device"));
      if((sub==USB_SC_HIDNONE)||(sub==USB_SC_HIDBOOT))
      {
        switch(prot)
        {
          case USB_PROT_HIDKBD:
            device.setDescription(_("Keyboard"));
            break;
          case USB_PROT_HIDMOUSE:
            device.setDescription(_("Mouse"));
            break;
        }
      }
      break;
    case USB_CLASS_PRINTER:
      device.setClass(hw::printer);
      device.setDescription(_("Printer"));
      device.addHint("icon", string("printer"));
      if(sub==USB_SC_PRINTER)
      {
        switch(prot)
        {
          case USB_PROT_PRINTERUNIDIR:
            device.addCapability("unidirectional", _("Unidirectional"));
            break;
          case USB_PROT_PRINTERBIDIR:
            device.addCapability("bidirectional", _("Bidirectional"));
            break;
          case USB_PROT_PRINTER1284:
            device.addCapability("ieee1284.4", _("IEEE 1284.4 compatible bidirectional"));
            break;
        }
      }
      break;
    case USB_CLASS_MASS_STORAGE:
      device.setClass(hw::storage);
      device.setDescription(_("Mass storage device"));
      switch(sub)
      {
        case USB_SC_STORAGERBC:
          device.addCapability("flash", _("RBC (typically Flash) mass storage"));
          break;
        case USB_SC_STORAGEATAPI:
          device.addCapability("atapi", _("SFF-8020i, MMC-2 (ATAPI)"));
          break;
        case USB_SC_STORAGEFLOPPY:
          device.addCapability("floppy", _("Floppy (UFI)"));
          break;
        case USB_SC_STORAGESCSI:
          device.addCapability("scsi", _("SCSI"));
          break;
      }
      break;
    case USB_CLASS_HUB:
      device.setClass(hw::bus);
      device.setDescription(_("USB hub"));
      break;
    case USB_CLASS_DATA:
      device.setClass(hw::generic);
      break;
    case USB_CLASS_SMARTCARD:
      device.setClass(hw::generic);
      device.setDescription(_("Smart card reader"));
      break;
    case USB_CLASS_VIDEO:
      device.setClass(hw::multimedia);
      device.setDescription(_("Video"));
      break;
    case USB_CLASS_WIRELESS:
      device.setClass(hw::communication);
      device.setDescription(_("Wireless interface"));
      if((sub==USB_SC_WIRELESSRADIO) && (prot==USB_PROT_BLUETOOTH))
      {
        device.setDescription(_("Bluetooth wireless interface"));
        device.addCapability("bluetooth", _("Bluetooth wireless radio"));
        device.addHint("icon", string("bluetooth"));
      }
      break;
    default:
      device.setDescription(_("Generic USB device"));
      return false;
  }

  return true;
}


static bool describeUSB(hwNode & device, unsigned vendor, unsigned prodid)
{
  if(usbvendors.find(vendor)==usbvendors.end()) return false;

  device.setVendor(usbvendors[vendor]+(enabled("output:numeric")?" ["+tohex(vendor)+"]":""));
  device.addHint("usb.idVendor", vendor);
  device.addHint("usb.idProduct", prodid);

  if(usbproducts.find(PRODID(vendor, prodid))!=usbproducts.end())
    device.setProduct(usbproducts[PRODID(vendor, prodid)]+(enabled("output:numeric")?" ["+tohex(vendor)+":"+tohex(prodid)+"]":""));

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
    char *description = NULL;

    if(getline(&buffer, &linelen, usbids)>0)
    {
      if(buffer[linelen-1]<' ')
        buffer[linelen-1] = '\0';                 // chop \n
      string line = string(buffer);
      free(buffer);

      description = NULL;
      t = 0;
      if(line.length()>0)
      {
        if(line[0] == '\t')                       // product id entry
        {
          line.erase(0, 1);
          if(matches(line, "^[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]"))
            t = strtol(line.c_str(), &description, 16);
          if(description && (description != line.c_str()))
          {
            usbproducts[PRODID(vendorid,t)] = hw::strip(description);
          }
        }
        else                                      // vendor id entry
        {
          if(matches(line, "^[[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]]"))
            t = strtol(line.c_str(), &description, 16);
          if(description && (description != line.c_str()))
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
  char ver[10+1];
  unsigned int cls, sub, prot, mxps, numcfgs;
  unsigned int vendor, prodid;
  char rev[10+1];
  unsigned numifs, cfgnum, atr;
  char mxpwr[10+1];
  unsigned ifnum, alt, numeps;
  char driver[80+1];

  if (!exists(SYSKERNELDEBUGUSBDEVICES) && !exists(PROCBUSUSBDEVICES))
    return false;

  vector < string > filenames;
  splitlines(USBID_PATH, filenames, ':');
  for (int i = filenames.size() - 1; i >= 0; i--)
  {
    load_usbids(filenames[i]);
  }
  filenames.clear();

  usbdevices = fopen(SYSKERNELDEBUGUSBDEVICES, "r");
  if(!usbdevices)
    usbdevices = fopen(PROCBUSUSBDEVICES, "r");

  while(!feof(usbdevices))
  {
    char * buffer = NULL;
    size_t linelen;
    char strname[80+1];
    char strval[80+1];

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
              device.setPhysId(port+1);
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
              setUSBClass(device, cls, sub, prot);
              device.addCapability(string("usb-")+string(ver));
              device.describeCapability("usb-1.00", "USB 1.0");
              device.describeCapability("usb-1.10", "USB 1.1");
              device.describeCapability("usb-2.00", "USB 2.0");
              device.addHint("usb.bDeviceClass", cls);
              device.addHint("usb.bDeviceSubClass", sub);
              device.addHint("usb.bDeviceProtocol", prot);
            }
            break;
          case 'P':
            vendor = prodid = 0;
            strcpy(rev, "");
            if(sscanf(line.c_str(), "P: Vendor=%x ProdID=%x Rev=%10s", &vendor, &prodid, rev)>0)
            {
              describeUSB(device, vendor, prodid);
              device.setVersion(hw::strip(rev));
            }
            break;
          case 'S':
            memset(strname, 0, sizeof(strname));
            memset(strval, 0, sizeof(strval));
            if(sscanf(line.c_str(), "S: %80[^=]=%80[ -z]", strname, strval)>0)
            {
              if(strcasecmp(strname, "Manufacturer")==0)
                device.setVendor(hw::strip(strval)+(enabled("output:numeric")?" ["+tohex(vendor)+"]":""));
              if(strcasecmp(strname, "Product")==0)
                device.setProduct(hw::strip(strval)+(enabled("output:numeric")?" ["+tohex(vendor)+":"+tohex(prodid)+"]":""));
              if(strcasecmp(strname, "SerialNumber")==0)
                device.setSerial(hw::strip(strval));
            }
            break;
          case 'C':
            numifs = cfgnum = atr = 0;
            strcpy(mxpwr, "");
            if(sscanf(line.c_str(), "C:* #Ifs=%u Cfg#=%u Atr=%x MxPwr=%s", &numifs, &cfgnum, &atr, mxpwr)>0)
            {
              if(strcmp("0mA", mxpwr)!=0)
                device.setConfig("maxpower", mxpwr);
            }
            break;
          case 'I':
            ifnum = alt = numeps = cls = sub = prot = 0;
            memset(driver, 0, sizeof(driver));
            if(((sscanf(line.c_str(), "I:* If#=%u Alt=%u #EPs=%u Cls=%x(%*5c) Sub=%x Prot=%x Driver=%80[ -z]", &ifnum, &alt, &numeps, &cls, &sub, &prot, driver)>0) && (cfgnum>0)) || ((sscanf(line.c_str(), "I: If#=%u Alt=%u #EPs=%u Cls=%x(%*5c) Sub=%x Prot=%x Driver=%80[ -z]", &ifnum, &alt, &numeps, &cls, &sub, &prot, driver)>0) && (cfgnum>0)))
            {
              setUSBClass(device, cls, sub, prot);
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

  return true;
}
