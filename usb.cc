/*
 * usb.cc
 *
 */

#include "usb.h"
#include "osutils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define PROCBUSUSB "/proc/bus/usb/"

#define	USB_DT_DEVICE_SIZE	0x12

#define USB_CLASS_PER_INTERFACE         0	/* for DeviceClass */
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_PRINTER               7
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9
#define USB_CLASS_DATA                  10
#define USB_CLASS_VENDOR_SPEC           0xff

/* All standard descriptors have these 2 fields in common */
struct usb_descriptor_header
{
  u_int8_t bLength;
  u_int8_t bDescriptorType;
}
__attribute__ ((packed));

/* Device descriptor */
struct usb_device_descriptor
{
  u_int8_t bLength;
  u_int8_t bDescriptorType;
  u_int16_t bcdUSB;
  u_int8_t bDeviceClass;
  u_int8_t bDeviceSubClass;
  u_int8_t bDeviceProtocol;
  u_int8_t bMaxPacketSize0;
  u_int16_t idVendor;
  u_int16_t idProduct;
  u_int16_t bcdDevice;
  u_int8_t iManufacturer;
  u_int8_t iProduct;
  u_int8_t iSerialNumber;
  u_int8_t bNumConfigurations;
}
__attribute__ ((packed));

static char *id = "@(#) $Id: usb.cc,v 1.1 2003/11/12 12:42:59 ezix Exp $";

static int selectnumber(const struct dirent *d)
{
  char *ptr = NULL;

  strtoul(d->d_name, &ptr, 10);
  return (ptr != NULL) && (strlen(ptr) == 0);
}

bool scan_usb(hwNode & n)
{
  hwNode *controller = NULL;
  struct dirent **hubs;
  int count;

  if (!exists(PROCBUSUSB))
    return false;

  if (!controller)
  {
    controller = n.addChild(hwNode("usb", hw::bus));

    if (controller)
      controller->setDescription("USB controller");
  }

  if (!controller)
    return false;

  controller->claim();

  pushd(PROCBUSUSB);
  count = scandir(".", &hubs, selectnumber, alphasort);

  for (int i = 0; i < count; i++)
  {
    hwNode hub("hub",
	       hw::bus);

    hub.claim();
    hub.setPhysId(hubs[i]->d_name);
    hub.setDescription("USB root hub");

    if (pushd(hubs[i]->d_name))
    {
      int countdevices;
      struct dirent **devices;
      countdevices = scandir(".", &devices, selectnumber, alphasort);

      for (int j = 0; j < countdevices; j++)
      {
	hwNode device("device");
	usb_device_descriptor descriptor;
	int fd = -1;

	device.setPhysId(devices[j]->d_name);

	memset(&descriptor, sizeof(descriptor), 0);
	fd = open(devices[j]->d_name, O_RDONLY);
	if (fd >= 0)
	{
	  if (read(fd, &descriptor, USB_DT_DEVICE_SIZE) == USB_DT_DEVICE_SIZE)
	  {
	    if (descriptor.bLength == USB_DT_DEVICE_SIZE)
	    {
	      switch (descriptor.bDeviceClass)
	      {
	      case USB_CLASS_AUDIO:
		device.setClass(hw::multimedia);
		device.setDescription("USB audio device");
		break;
	      case USB_CLASS_COMM:
		device.setClass(hw::communication);
		device.setDescription("USB communication device");
		break;
	      case USB_CLASS_HID:
		device.setClass(hw::input);
		device.setDescription("USB human interface device");
		break;
	      case USB_CLASS_PRINTER:
		device.setClass(hw::printer);
		device.setDescription("USB printer");
		break;
	      case USB_CLASS_MASS_STORAGE:
		device.setClass(hw::storage);
		device.setDescription("USB mass storage device");
		break;
	      case USB_CLASS_HUB:
		device.setClass(hw::bus);
		device.setDescription("USB hub");
		break;
	      }
	    }
	  }
	  close(fd);
	}

	hub.addChild(device);
      }

      popd();
    }

    controller->addChild(hub);
  }
  popd();

  (void) &id;			// avoid warning "id defined but not used"

  return false;
}
