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

#define PROCBUSUSB "/proc/bus/usb/"

#define	USB_DT_DEVICE_SIZE	0x12
#define USB_CTRL_TIMEOUT    100	/* milliseconds */
#define USB_CTRL_RETRIES     50

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

#define USBDEVICE_SUPER_MAGIC 0x9fa2

/* usbdevfs ioctl codes */

struct usbdevfs_ctrltransfer
{
  u_int8_t requesttype;
  u_int8_t request;
  u_int16_t value;
  u_int16_t index;
  u_int16_t length;
  u_int32_t timeout;		/* in milliseconds */
  void *data;
};

#define USBDEVFS_CONTROL	_IOWR('U', 0, struct usbdevfs_ctrltransfer)

static char *id = "@(#) $Id: usb.cc,v 1.2 2003/11/17 22:29:57 ezix Exp $";

static int usb_control_msg(int fd,
			   u_int8_t requesttype,
			   u_int8_t request,
			   u_int16_t value,
			   u_int16_t index,
			   unsigned int size,
			   void *data)
{
  struct usbdevfs_ctrltransfer ctrl;
  int result, tries;

  ctrl.requesttype = requesttype;
  ctrl.request = request;
  ctrl.value = value;
  ctrl.index = index;
  ctrl.length = size;
  ctrl.data = data;
  ctrl.timeout = USB_CTRL_TIMEOUT;
  tries = 0;
  do
  {
    result = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
    tries++;
  }
  while (tries < USB_CTRL_RETRIES && result == -1 && errno == ETIMEDOUT);
  return result;
}

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
	hwNode device("usb");
	usb_device_descriptor descriptor;
	int fd = -1;

	// device.setPhysId(devices[j]->d_name);
	device.setHandle(string("USB:") + string(devices[j]->d_name));

	memset(&descriptor, sizeof(descriptor), 0);
	fd = open(devices[j]->d_name, O_RDONLY);
	if (fd >= 0)
	{
	  if (read(fd, &descriptor, USB_DT_DEVICE_SIZE) == USB_DT_DEVICE_SIZE)
	  {
	    if (descriptor.bLength == USB_DT_DEVICE_SIZE)
	    {
	      char usbversion[20];

	      snprintf(usbversion, sizeof(usbversion), "usb-%x.%02x",
		       (descriptor.bcdUSB & 0xff00) >> 8,
		       descriptor.bcdUSB & 0x00ff);
	      device.addCapability(usbversion);

	      if ((descriptor.bcdUSB & 0x00ff) == 0)	// like USB 1.0
	      {
		snprintf(usbversion, sizeof(usbversion), "usb-%x.0",
			 (descriptor.bcdUSB & 0xff00) >> 8);
		device.addCapability(usbversion);	// usb-1
		snprintf(usbversion, sizeof(usbversion), "usb-%x",
			 (descriptor.bcdUSB & 0xff00) >> 8);
		device.addCapability(usbversion);	// usb-1.0
	      }

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

	      if (descriptor.idVendor == 0)
		device.addCapability("virtual");
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
