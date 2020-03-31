#include "version.h"
#include "hw.h"
#include "sysfs.h"
#include "osutils.h"
#include "mmc.h"
#include "disk.h"
#include "heuristics.h"

#include <vector>
#include <iostream>

__ID("@(#) $Id$");

/*
 * Standard SDIO Function Interfaces
 */

#define SDIO_CLASS_NONE		0x00	/* Not a SDIO standard interface */
#define SDIO_CLASS_UART		0x01	/* standard UART interface */
#define SDIO_CLASS_BT_A		0x02	/* Type-A BlueTooth std interface */
#define SDIO_CLASS_BT_B		0x03	/* Type-B BlueTooth std interface */
#define SDIO_CLASS_GPS		0x04	/* GPS standard interface */
#define SDIO_CLASS_CAMERA	0x05	/* Camera standard interface */
#define SDIO_CLASS_PHS		0x06	/* PHS standard interface */
#define SDIO_CLASS_WLAN		0x07	/* WLAN interface */
#define SDIO_CLASS_ATA		0x08	/* Embedded SDIO-ATA std interface */
#define SDIO_CLASS_BT_AMP	0x09	/* Type-A Bluetooth AMP interface */

/*
 * Vendors and devices.  Sort key: vendor first, device next.
 */
#define SDIO_VENDOR_ID_BROADCOM			0x02d0
#define SDIO_DEVICE_ID_BROADCOM_43143		0xa887
#define SDIO_DEVICE_ID_BROADCOM_43241		0x4324
#define SDIO_DEVICE_ID_BROADCOM_4329		0x4329
#define SDIO_DEVICE_ID_BROADCOM_4330		0x4330
#define SDIO_DEVICE_ID_BROADCOM_4334		0x4334
#define SDIO_DEVICE_ID_BROADCOM_43340		0xa94c
#define SDIO_DEVICE_ID_BROADCOM_43341		0xa94d
#define SDIO_DEVICE_ID_BROADCOM_4335_4339	0x4335
#define SDIO_DEVICE_ID_BROADCOM_4339		0x4339
#define SDIO_DEVICE_ID_BROADCOM_43362		0xa962
#define SDIO_DEVICE_ID_BROADCOM_43364		0xa9a4
#define SDIO_DEVICE_ID_BROADCOM_43430		0xa9a6
#define SDIO_DEVICE_ID_BROADCOM_4345		0x4345
#define SDIO_DEVICE_ID_BROADCOM_43455		0xa9bf
#define SDIO_DEVICE_ID_BROADCOM_4354		0x4354
#define SDIO_DEVICE_ID_BROADCOM_4356		0x4356
#define SDIO_DEVICE_ID_BROADCOM_4359		0x4359
#define SDIO_DEVICE_ID_CYPRESS_4373		0x4373
#define SDIO_DEVICE_ID_CYPRESS_43012		43012
#define SDIO_DEVICE_ID_CYPRESS_89359		0x4355

#define SDIO_VENDOR_ID_INTEL			0x0089
#define SDIO_DEVICE_ID_INTEL_IWMC3200WIMAX	0x1402
#define SDIO_DEVICE_ID_INTEL_IWMC3200WIFI	0x1403
#define SDIO_DEVICE_ID_INTEL_IWMC3200TOP	0x1404
#define SDIO_DEVICE_ID_INTEL_IWMC3200GPS	0x1405
#define SDIO_DEVICE_ID_INTEL_IWMC3200BT		0x1406
#define SDIO_DEVICE_ID_INTEL_IWMC3200WIMAX_2G5	0x1407

#define SDIO_VENDOR_ID_MARVELL			0x02df
#define SDIO_DEVICE_ID_MARVELL_LIBERTAS		0x9103
#define SDIO_DEVICE_ID_MARVELL_8688WLAN		0x9104
#define SDIO_DEVICE_ID_MARVELL_8688BT		0x9105
#define SDIO_DEVICE_ID_MARVELL_8797_F0		0x9128
#define SDIO_DEVICE_ID_MARVELL_8887WLAN	0x9134

#define SDIO_VENDOR_ID_MEDIATEK			0x037a

#define SDIO_VENDOR_ID_SIANO			0x039a
#define SDIO_DEVICE_ID_SIANO_NOVA_B0		0x0201
#define SDIO_DEVICE_ID_SIANO_NICE		0x0202
#define SDIO_DEVICE_ID_SIANO_VEGA_A0		0x0300
#define SDIO_DEVICE_ID_SIANO_VENICE		0x0301
#define SDIO_DEVICE_ID_SIANO_NOVA_A0		0x1100
#define SDIO_DEVICE_ID_SIANO_STELLAR 		0x5347

#define SDIO_VENDOR_ID_TI			0x0097
#define SDIO_DEVICE_ID_TI_WL1271		0x4076
#define SDIO_VENDOR_ID_TI_WL1251		0x104c
#define SDIO_DEVICE_ID_TI_WL1251		0x9066

#define SDIO_VENDOR_ID_STE			0x0020
#define SDIO_DEVICE_ID_STE_CW1200		0x2280


using namespace std;

static string strip0x(string s)
{
  if(s.length()<2) return s;
  if(s.substr(0, 2) == "0x") return s.substr(2, string::npos);
  return s;
}

static string manufacturer(unsigned long n)
{
  switch(n)
  {
    case 0x0:
            return "";
    case 0x2:
	    return "Kingston";
    case 0x3:
	    return "SanDisk";
    default:
            return "Unknown ("+tostring(n)+")";
  }
}

static hwNode sdioNode(unsigned long long c, unsigned long long vendor = 0, unsigned long long device = 0)
{
  hwNode result("interface");

  switch(c)
  {
    case SDIO_CLASS_UART:
      result = hwNode("serial", hw::communication);
      result.setDescription("UART interface");
      break;
    case SDIO_CLASS_BT_A:
    case SDIO_CLASS_BT_B:
    case SDIO_CLASS_BT_AMP:
      result = hwNode("bt", hw::communication);
      result.setDescription("BlueTooth interface");
      result.addCapability("wireless");
      result.addCapability("bluetooth");
      result.setConfig("wireless", "BlueTooth");
      break;
    case SDIO_CLASS_GPS:
      result = hwNode("gps");
      result.setDescription("GPS interface");
      break;
    case SDIO_CLASS_CAMERA:
      result = hwNode("camera");
      result.setDescription("Camera interface");
      break;
    case SDIO_CLASS_PHS:
      result = hwNode("phs");
      result.setDescription("PHS interface");
      break;
    case SDIO_CLASS_WLAN:
      result = hwNode("wifi", hw::network);
      result.setDescription("Wireless interface");
      result.addCapability("wireless");
      result.setConfig("wireless", "IEEE 802.11");
      break;
    case SDIO_CLASS_ATA:
      result = hwNode("ata");
      result.setDescription("SDIO-ATA interface");
      break;
  }

  switch(vendor)
  {
    case SDIO_VENDOR_ID_BROADCOM:
      result.setVendor("Broadcom");
      switch(device)
      {
        case SDIO_DEVICE_ID_BROADCOM_43143:
          result.setProduct("43143");
          break;
        case SDIO_DEVICE_ID_BROADCOM_43241:
          result.setProduct("43241");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4329:
          result.setProduct("4329");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4330:
          result.setProduct("4330");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4334:
          result.setProduct("4334");
          break;
        case SDIO_DEVICE_ID_BROADCOM_43340:
          result.setProduct("43340");
          break;
        case SDIO_DEVICE_ID_BROADCOM_43341:
          result.setProduct("43341");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4335_4339:
          result.setProduct("4335/4339");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4339:
          result.setProduct("4339");
          break;
        case SDIO_DEVICE_ID_BROADCOM_43362:
          result.setProduct("43362");
          break;
        case SDIO_DEVICE_ID_BROADCOM_43364:
          result.setProduct("43364");
          break;
        case SDIO_DEVICE_ID_BROADCOM_43430:
          result.setProduct("43430");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4345:
          result.setProduct("4345");
          break;
        case SDIO_DEVICE_ID_BROADCOM_43455:
          result.setProduct("43455");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4354:
          result.setProduct("4354");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4356:
          result.setProduct("4356");
          break;
        case SDIO_DEVICE_ID_BROADCOM_4359:
          result.setProduct("4359");
          break;
        case SDIO_DEVICE_ID_CYPRESS_4373:
          result.setVendor("Cypress");
          result.setProduct("4373");
          break;
        case SDIO_DEVICE_ID_CYPRESS_43012:
          result.setVendor("Cypress");
          result.setProduct("43012");
          break;
        case SDIO_DEVICE_ID_CYPRESS_89359:
          result.setVendor("Cypress");
          result.setProduct("89359");
          break;
      }
      break;
    case SDIO_VENDOR_ID_INTEL:
      result.setVendor("Intel");
      switch(device)
      {
        case SDIO_DEVICE_ID_INTEL_IWMC3200WIMAX:
          result.setProduct("IWMC3200WIMAX");
          break;
        case SDIO_DEVICE_ID_INTEL_IWMC3200WIFI:
          result.setProduct("IWMC3200WIFI");
          break;
        case SDIO_DEVICE_ID_INTEL_IWMC3200TOP:
          result.setProduct("IWMC3200TOP");
          break;
        case SDIO_DEVICE_ID_INTEL_IWMC3200GPS:
          result.setProduct("IWMC3200GPS");
          break;
        case SDIO_DEVICE_ID_INTEL_IWMC3200BT:
          result.setProduct("IWMC3200BT");
          break;
        case SDIO_DEVICE_ID_INTEL_IWMC3200WIMAX_2G5:
          result.setProduct("IWMC3200WIMAX 2G5");
          break;
      }
      break;
    case SDIO_VENDOR_ID_MARVELL:
      result.setVendor("Marvell");
      break;
    case SDIO_VENDOR_ID_MEDIATEK:
      result.setVendor("MediaTek");
      break;
    case SDIO_VENDOR_ID_SIANO:
      result.setVendor("Siano");
      switch(device)
      {
        case SDIO_DEVICE_ID_SIANO_NOVA_B0:
          result.setProduct("NOVA B0");
          break;
        case SDIO_DEVICE_ID_SIANO_NICE:
          result.setProduct("NICE");
          break;
        case SDIO_DEVICE_ID_SIANO_VEGA_A0:
          result.setProduct("VEGA A0");
          break;
        case SDIO_DEVICE_ID_SIANO_VENICE:
          result.setProduct("VENICE");
          break;
        case SDIO_DEVICE_ID_SIANO_NOVA_A0:
          result.setProduct("NOVA A0");
          break;
        case SDIO_DEVICE_ID_SIANO_STELLAR:
          result.setProduct("STELLAR");
          break;
      }
      break;
    case SDIO_VENDOR_ID_TI:
      result.setVendor("Texas Instruments");
      switch(device)
      {
        case SDIO_DEVICE_ID_TI_WL1271:
          result.setProduct("WL1271");
          break;
        case SDIO_VENDOR_ID_TI_WL1251:
        case SDIO_DEVICE_ID_TI_WL1251:
          result.setProduct("WL1251");
          break;
      }
      break;
    case SDIO_VENDOR_ID_STE:
      result.setVendor("ST Ericsson");
      switch(device)
      {
        case SDIO_DEVICE_ID_STE_CW1200:
          result.setProduct("CW1200");
          break;
      }
      break;
  }

  return result;
}

bool scan_mmc(hwNode & n)
{
  vector < sysfs::entry > entries = sysfs::entries_by_class("mmc_host");

  if (entries.empty())
    return false;

  for (vector < sysfs::entry >::iterator it = entries.begin();
      it != entries.end(); ++it)
  {
    const sysfs::entry & e = *it;

    hwNode *device = n.findChildByBusInfo(e.leaf().businfo());
    if(!device)
      device = n.addChild(hwNode(e.name(), hw::bus));
    else
      device->setClass(hw::bus);

    device->claim();
    device->setLogicalName(e.name());
    device->setDescription("MMC Host");
    device->setModalias(e.modalias());

    vector < sysfs::entry > devices = e.devices();
    for(vector < sysfs::entry >::iterator i = devices.begin(); i != devices.end(); ++i)
    {
      const sysfs::entry & d = *i;

      hwNode card("device");
      card.claim();
      string prod = d.string_attr("name");
      if(prod != "00000") card.setProduct(prod);
      card.addCapability(d.string_attr("type"));
      card.setVendor(manufacturer(d.hex_attr("manfid")));
      card.setBusInfo(guessBusInfo(d.name()));
      card.setPhysId(strip0x(d.string_attr("rca")));
      card.setSerial(tostring(d.hex_attr("serial")));
      if(unsigned long hwrev = d.hex_attr("hwrev")) {
	card.setVersion(tostring(hwrev)+"."+tostring(d.hex_attr("fwrev")));
      }
      card.setDate(d.string_attr("date"));
      card.setDescription("SD/MMC Device");
      if(card.isCapable("sdio"))
        card.setDescription("SDIO Device");
      if(d.string_attr("scr")!="")
      {
	card.setDescription("SD Card");
        card.setClass(hw::disk);
      }

      vector < sysfs::entry > devices = d.devices();
      for(unsigned long i=0;i<devices.size();i++)
      {
        hwNode *subdev;
        if(devices.size()>1)
        {
          subdev = card.addChild(sdioNode(devices[i].hex_attr("class"), devices[i].hex_attr("vendor"), devices[i].hex_attr("device")));
          subdev->setPhysId(tostring(i+1));
        }
        else
          subdev = &card;

        subdev->setLogicalName(devices[i].name());
        subdev->setModalias(devices[i].modalias());
        subdev->setBusInfo(guessBusInfo(devices[i].name()));
        if(devices[i].hex_attr("removable"))
          subdev->addCapability("removable");
        scan_disk(*subdev);
      }

      device->addChild(card);
    }

  }

  return true;
}
