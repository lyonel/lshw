/*
 *  isapnp.cc
 *
 *  This scan, only available on (older) i386 systems, tries to detect PnP
 *  (Plug'n Play) ISA devices.
 *
 *  This detection is loosely based on the implementation used in the Linux
 *  kernel itself.
 */

#include "version.h"
#include "isapnp.h"
#include "pnp.h"

__ID("@(#) $Id$");

#ifdef __i386__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

/* config parameters */
#define PNP_CONFIG_NORMAL 0x0001
#define PNP_CONFIG_FORCE  0x0002                  /* disables validity checking */

/* capabilities */
#define PNP_READ    0x0001
#define PNP_WRITE   0x0002
#define PNP_DISABLE   0x0004
#define PNP_CONFIGURABLE  0x0008
#define PNP_REMOVABLE   0x0010

/* status */
#define PNP_READY   0x0000
#define PNP_ATTACHED    0x0001
#define PNP_BUSY    0x0002
#define PNP_FAULTY    0x0004

#define ISAPNP_CFG_ACTIVATE   0x30                /* byte */
#define ISAPNP_CFG_MEM      0x40                  /* 4 * dword */
#define ISAPNP_CFG_PORT     0x60                  /* 8 * word */
#define ISAPNP_CFG_IRQ      0x70                  /* 2 * word */
#define ISAPNP_CFG_DMA      0x74                  /* 2 * byte */

#define ISAPNP_VENDOR(a,b,c)  (((((a)-'A'+1)&0x3f)<<2)|\
((((b)-'A'+1)&0x18)>>3)|((((b)-'A'+1)&7)<<13)|\
((((c)-'A'+1)&0x1f)<<8))
#define ISAPNP_DEVICE(x)  ((((x)&0xf000)>>8)|\
(((x)&0x0f00)>>8)|\
(((x)&0x00f0)<<8)|\
(((x)&0x000f)<<8))
#define ISAPNP_FUNCTION(x)  ISAPNP_DEVICE(x)

#define DEVICE_COUNT_COMPATIBLE 4

#define ISAPNP_ANY_ID   0xffff
#define ISAPNP_CARD_DEVS  8

#define ISAPNP_CARD_ID(_va, _vb, _vc, _device) \
.card_vendor = ISAPNP_VENDOR(_va, _vb, _vc), .card_device = ISAPNP_DEVICE(_device)
#define ISAPNP_CARD_END \
.card_vendor = 0, .card_device = 0
#define ISAPNP_DEVICE_ID(_va, _vb, _vc, _function) \
{ .vendor = ISAPNP_VENDOR(_va, _vb, _vc), .function = ISAPNP_FUNCTION(_function) }

struct isapnp_card_id
{
  unsigned long driver_data;                      /* data private to the driver */
  unsigned short card_vendor, card_device;
  struct
  {
    unsigned short vendor, function;
  }
  devs[ISAPNP_CARD_DEVS];                         /* logical devices */
};

#define ISAPNP_DEVICE_SINGLE(_cva, _cvb, _cvc, _cdevice, _dva, _dvb, _dvc, _dfunction) \
.card_vendor = ISAPNP_VENDOR(_cva, _cvb, _cvc), .card_device =  ISAPNP_DEVICE(_cdevice), \
.vendor = ISAPNP_VENDOR(_dva, _dvb, _dvc), .function = ISAPNP_FUNCTION(_dfunction)
#define ISAPNP_DEVICE_SINGLE_END \
.card_vendor = 0, .card_device = 0

struct isapnp_device_id
{
  unsigned short card_vendor, card_device;
  unsigned short vendor, function;
  unsigned long driver_data;                      /* data private to the driver */
};

static int isapnp_rdp;                            /* Read Data Port */
static int isapnp_reset = 0;                      /* reset all PnP cards (deactivate) */
static bool isapnp_error = false;

#define _PIDXR    0x279
#define _PNPWRP   0xa79

/* short tags */
#define _STAG_PNPVERNO    0x01
#define _STAG_LOGDEVID    0x02
#define _STAG_COMPATDEVID 0x03
#define _STAG_IRQ   0x04
#define _STAG_DMA   0x05
#define _STAG_STARTDEP    0x06
#define _STAG_ENDDEP    0x07
#define _STAG_IOPORT    0x08
#define _STAG_FIXEDIO   0x09
#define _STAG_VENDOR    0x0e
#define _STAG_END   0x0f
/* long tags */
#define _LTAG_MEMRANGE    0x81
#define _LTAG_ANSISTR   0x82
#define _LTAG_UNICODESTR  0x83
#define _LTAG_VENDOR    0x84
#define _LTAG_MEM32RANGE  0x85
#define _LTAG_FIXEDMEM32RANGE 0x86

static unsigned char isapnp_checksum_value;
static int isapnp_detected;

/* some prototypes */

static void udelay(unsigned long l)
{
  struct timespec delay;

  delay.tv_sec = 0;
  delay.tv_nsec = l / 100;

  nanosleep(&delay, NULL);
}


static bool write_data(unsigned char x)
{
  bool result = true;
  int fd = open("/dev/port", O_WRONLY);

  if (fd >= 0)
  {
    lseek(fd, _PNPWRP, SEEK_SET);
    if(write(fd, &x, 1) != 1)
      result = false;
    close(fd);

    return result;
  }
  else
  {
    isapnp_error = true;
    return false;
  }
}


static bool write_address(unsigned char x)
{
  bool result = true;
  int fd = open("/dev/port", O_WRONLY);

  if (fd >= 0)
  {
    lseek(fd, _PIDXR, SEEK_SET);
    if(write(fd, &x, 1) != 1)
      result = false;
    close(fd);
    udelay(20);
    return result;
  }
  else
  {
    isapnp_error = true;
    return false;
  }
}


static unsigned char read_data(void)
{
  int fd = open("/dev/port", O_RDONLY);
  unsigned char val = 0;

  if (fd >= 0)
  {
    lseek(fd, isapnp_rdp, SEEK_SET);
    if(read(fd, &val, 1) != 1)
      val = 0;
    close(fd);
  }
  else
  {
    isapnp_error = true;
  }
  return val;
}


static unsigned char isapnp_read_byte(unsigned char idx)
{
  if (write_address(idx))
    return read_data();
  else
    return 0;
}


static bool isapnp_write_byte(unsigned char idx,
unsigned char val)
{
  return write_address(idx) && write_data(val);
}


static bool isapnp_key(void)
{
  unsigned char code = 0x6a, msb;
  int i;

  udelay(100);
  if (!write_address(0x00))
    return false;
  if (!write_address(0x00))
    return false;

  if (!write_address(code))
    return false;

  for (i = 1; i < 32; i++)
  {
    msb = ((code & 0x01) ^ ((code & 0x02) >> 1)) << 7;
    code = (code >> 1) | msb;
    if (!write_address(code))
      return false;
  }

  return true;
}


/* place all pnp cards in wait-for-key state */
static bool isapnp_wait(void)
{
  return isapnp_write_byte(0x02, 0x02);
}


static bool isapnp_wake(unsigned char csn)
{
  return isapnp_write_byte(0x03, csn);
}


static bool isapnp_peek(unsigned char *data,
int bytes)
{
  int i, j;
  unsigned char d = 0;

  isapnp_error = false;

  for (i = 1; i <= bytes; i++)
  {
    for (j = 0; j < 20; j++)
    {
      d = isapnp_read_byte(0x05);
      if (isapnp_error)
        return false;
      if (d & 1)
        break;
      udelay(10);
    }
    if (!(d & 1))
    {
      if (data != NULL)
        *data++ = 0xff;
      continue;
    }
    d = isapnp_read_byte(0x04);                   /* PRESDI */
    isapnp_checksum_value += d;
    if (data != NULL)
      *data++ = d;
  }
  return true;
}


#define RDP_STEP  32                              /* minimum is 4 */

static int isapnp_next_rdp(void)
{
  int rdp = isapnp_rdp;
  while (rdp <= 0x3ff)
  {
/*
 *      We cannot use NE2000 probe spaces for ISAPnP or we
 *      will lock up machines.
 */
    if ((rdp < 0x280 || rdp > 0x380) /*&& !check_region(rdp, 1) */ )
    {
      isapnp_rdp = rdp;
      return 0;
    }
    rdp += RDP_STEP;
  }
  return -1;
}


/* Set read port address */
static inline bool isapnp_set_rdp(void)
{
  if (isapnp_write_byte(0x00, isapnp_rdp >> 2))
  {
    udelay(100);
    return true;
  }
  else
    return false;
}


/*
 *	Perform an isolation. The port selection code now tries to avoid
 *	"dangerous to read" ports.
 */

static int isapnp_isolate_rdp_select(void)
{
  if (!isapnp_wait())
    return -1;
  if (!isapnp_key())
    return -1;

/*
 * Control: reset CSN and conditionally everything else too
 */
  if (!isapnp_write_byte(0x02, isapnp_reset ? 0x05 : 0x04))
    return -1;
  udelay(2);

  if (!isapnp_wait())
    return -1;
  if (!isapnp_key())
    return -1;
  if (!isapnp_wake(0x00))
    return -1;

  if (isapnp_next_rdp() < 0)
  {
    isapnp_wait();
    return -1;
  }

  isapnp_set_rdp();
  udelay(1);
  if (!write_address(0x01))
    return -1;
  udelay(1);
  return 0;
}


/*
 *  Isolate (assign uniqued CSN) to all ISA PnP devices.
 */

static int isapnp_isolate(void)
{
  unsigned char checksum = 0x6a;
  unsigned char chksum = 0x00;
  unsigned char bit = 0x00;
  int data;
  int csn = 0;
  int i;
  int iteration = 1;

  isapnp_rdp = 0x213;
  if (isapnp_isolate_rdp_select() < 0)
    return -1;

  isapnp_error = false;

  while (1)
  {
    for (i = 1; i <= 64; i++)
    {
      if (isapnp_error)
        return -1;

      data = read_data() << 8;
      udelay(250);
      data = data | read_data();
      udelay(250);
      if (data == 0x55aa)
        bit = 0x01;
      checksum =
        ((((checksum ^ (checksum >> 1)) & 0x01) ^ bit) << 7) | (checksum >>
        1);
      bit = 0x00;
    }
    for (i = 65; i <= 72; i++)
    {
      if (isapnp_error)
        return -1;

      data = read_data() << 8;
      udelay(250);
      data = data | read_data();
      udelay(250);
      if (data == 0x55aa)
        chksum |= (1 << (i - 65));
    }
    if (checksum != 0x00 && checksum == chksum)
    {
      csn++;

      if (isapnp_error)
        return -1;

      if (!isapnp_write_byte(0x06, csn))
        return -1;
      udelay(250);
      iteration++;
      if (!isapnp_wake(0x00))
        return -1;
      isapnp_set_rdp();
      udelay(1000);
      if (!write_address(0x01))
        return -1;
      udelay(1000);
      goto __next;
    }
    if (iteration == 1)
    {
      isapnp_rdp += RDP_STEP;
      if (isapnp_isolate_rdp_select() < 0)
        return -1;
    }
    else if (iteration > 1)
    {
      break;
    }
    __next:
    checksum = 0x6a;
    chksum = 0x00;
    bit = 0x00;
  }
  if (!isapnp_wait())
    return -1;
  return csn;
}


/*
 *  Read one tag from stream.
 */

static int isapnp_read_tag(unsigned char *type,
unsigned short *size)
{
  unsigned char tag, tmp[2];

  isapnp_peek(&tag, 1);
  if (tag == 0)                                   /* invalid tag */
    return -1;
  if (tag & 0x80)
  {                                               /* large item */
    *type = tag;
    isapnp_peek(tmp, 2);
    *size = (tmp[1] << 8) | tmp[0];
  }
  else
  {
    *type = (tag >> 3) & 0x0f;
    *size = tag & 0x07;
  }
#if 0
  printk(KERN_DEBUG "tag = 0x%x, type = 0x%x, size = %i\n", tag, *type,
    *size);
#endif
  if (type == 0)                                  /* wrong type */
    return -1;
  if (*type == 0xff && *size == 0xffff)           /* probably invalid data */
    return -1;
  return 0;
}


/*
 *  Skip specified number of bytes from stream.
 */

static void isapnp_skip_bytes(int count)
{
  isapnp_peek(NULL, count);
}


/*
 *  Parse EISA id.
 */

static const char *isapnp_parse_id(unsigned short vendor,
unsigned short device)
{
  static char id[8];
  snprintf(id, sizeof(id), "%c%c%c%x%x%x%x",
    'A' + ((vendor >> 2) & 0x3f) - 1,
    'A' + (((vendor & 3) << 3) | ((vendor >> 13) & 7)) - 1,
    'A' + ((vendor >> 8) & 0x1f) - 1,
    (device >> 4) & 0x0f,
    device & 0x0f, (device >> 12) & 0x0f, (device >> 8) & 0x0f);

  return id;
}


/*
 *  Parse logical device tag.
 */

static hwNode *isapnp_parse_device(hwNode & card,
int size,
int number)
{
  unsigned char tmp[6];

  isapnp_peek(tmp, size);
  return card.
    addChild(hwNode
    (isapnp_parse_id
    ((tmp[1] << 8) | tmp[0], (tmp[3] << 8) | tmp[2])));
}


/*
 *  Add IRQ resource to resources list.
 */

static void isapnp_parse_irq_resource(hwNode & n,
int size)
{
  unsigned char tmp[3];

  isapnp_peek(tmp, size);
/*
 * irq = isapnp_alloc(sizeof(struct pnp_irq));
 * if (!irq)
 * return;
 * irq->map = (tmp[1] << 8) | tmp[0];
 * if (size > 2)
 * irq->flags = tmp[2];
 * else
 * irq->flags = IORESOURCE_IRQ_HIGHEDGE;
 * return;
 */
}


/*
 *  Add DMA resource to resources list.
 */

static void isapnp_parse_dma_resource(int size)
{
  unsigned char tmp[2];

  isapnp_peek(tmp, size);
/*
 * dma = isapnp_alloc(sizeof(struct pnp_dma));
 * if (!dma)
 * return;
 * dma->map = tmp[0];
 * dma->flags = tmp[1];
 * return;
 */
}


/*
 *  Add port resource to resources list.
 */

static void isapnp_parse_port_resource(hwNode & n,
int size)
{
  unsigned char tmp[7];

  isapnp_peek(tmp, size);
/*
 * port = isapnp_alloc(sizeof(struct pnp_port));
 * if (!port)
 * return;
 * port->min = (tmp[2] << 8) | tmp[1];
 * port->max = (tmp[4] << 8) | tmp[3];
 * port->align = tmp[5];
 * port->size = tmp[6];
 * port->flags = tmp[0] ? PNP_PORT_FLAG_16BITADDR : 0;
 * return;
 */
}


/*
 *  Add fixed port resource to resources list.
 */

static void isapnp_parse_fixed_port_resource(int size)
{
  unsigned char tmp[3];

  isapnp_peek(tmp, size);
/*
 * port = isapnp_alloc(sizeof(struct pnp_port));
 * if (!port)
 * return;
 * port->min = port->max = (tmp[1] << 8) | tmp[0];
 * port->size = tmp[2];
 * port->align = 0;
 * port->flags = PNP_PORT_FLAG_FIXED;
 * return;
 */
}


/*
 *  Add memory resource to resources list.
 */

static void isapnp_parse_mem_resource(int size)
{
  unsigned char tmp[9];

  isapnp_peek(tmp, size);
/*
 * mem = isapnp_alloc(sizeof(struct pnp_mem));
 * if (!mem)
 * return;
 * mem->min = ((tmp[2] << 8) | tmp[1]) << 8;
 * mem->max = ((tmp[4] << 8) | tmp[3]) << 8;
 * mem->align = (tmp[6] << 8) | tmp[5];
 * mem->size = ((tmp[8] << 8) | tmp[7]) << 8;
 * mem->flags = tmp[0];
 * return;
 */
}


/*
 *  Add 32-bit memory resource to resources list.
 */

static void isapnp_parse_mem32_resource(int size)
{
  unsigned char tmp[17];

  isapnp_peek(tmp, size);
/*
 * mem = isapnp_alloc(sizeof(struct pnp_mem));
 * if (!mem)
 * return;
 * mem->min = (tmp[4] << 24) | (tmp[3] << 16) | (tmp[2] << 8) | tmp[1];
 * mem->max = (tmp[8] << 24) | (tmp[7] << 16) | (tmp[6] << 8) | tmp[5];
 * mem->align = (tmp[12] << 24) | (tmp[11] << 16) | (tmp[10] << 8) | tmp[9];
 * mem->size = (tmp[16] << 24) | (tmp[15] << 16) | (tmp[14] << 8) | tmp[13];
 * mem->flags = tmp[0];
 */
}


/*
 *  Add 32-bit fixed memory resource to resources list.
 */

static void isapnp_parse_fixed_mem32_resource(int size)
{
  unsigned char tmp[9];

  isapnp_peek(tmp, size);
/*
 * mem = isapnp_alloc(sizeof(struct pnp_mem));
 * if (!mem)
 * return;
 * mem->min = mem->max = (tmp[4] << 24) | (tmp[3] << 16) | (tmp[2] << 8) | tmp[1];
 * mem->size = (tmp[8] << 24) | (tmp[7] << 16) | (tmp[6] << 8) | tmp[5];
 * mem->align = 0;
 * mem->flags = tmp[0];
 */
}


/*
 *  Parse card name for ISA PnP device.
 */

static string isapnp_parse_name(unsigned short &size)
{
  char buffer[1024];
  unsigned short size1 = size >= sizeof(buffer) ? (sizeof(buffer) - 1) : size;
  isapnp_peek((unsigned char *) buffer, size1);
  buffer[size1] = '\0';
  size -= size1;

  return hw::strip(buffer);
}


/*
 *  Parse resource map for logical device.
 */

static int isapnp_create_device(hwNode & card,
unsigned short size)
{
  int number = 0, skip = 0, priority = 0, compat = 0;
  unsigned char type, tmp[17];
  hwNode *dev = NULL;

  if ((dev = isapnp_parse_device(card, size, number++)) == NULL)
    return 1;
/*
 * option = pnp_register_independent_option(dev);
 * if (!option)
 * return 1;
 */

  while (1)
  {
    if (isapnp_read_tag(&type, &size) < 0)
      return 1;
    if (skip && type != _STAG_LOGDEVID && type != _STAG_END)
      goto __skip;
    switch (type)
    {
      case _STAG_LOGDEVID:
        if (size >= 5 && size <= 6)
        {
          if ((dev = isapnp_parse_device(card, size, number++)) == NULL)
            return 1;
          size = 0;
          skip = 0;
/*
 * option = pnp_register_independent_option(dev);
 * if (!option)
 * return 1;
 * pnp_add_card_device(card,dev);
 */
        }
        else
        {
          skip = 1;
        }
        priority = 0;
        compat = 0;
        break;
      case _STAG_COMPATDEVID:
        if (size == 4 && compat < DEVICE_COUNT_COMPATIBLE)
        {
          isapnp_peek(tmp, 4);
          if (dev->getClass() == hw::generic)
            dev->
              setClass(pnp_class
              (isapnp_parse_id
              ((tmp[1] << 8) | tmp[0], (tmp[3] << 8) | tmp[2])));
          dev->
            addCapability(string(isapnp_parse_id
            ((tmp[1] << 8) | tmp[0],
            (tmp[3] << 8) | tmp[2])) +
            string("-compatible"));
          compat++;
          size = 0;
        }
        break;
      case _STAG_IRQ:
        if (size < 2 || size > 3)
          goto __skip;
        isapnp_parse_irq_resource(card, size);
        size = 0;
        break;
      case _STAG_DMA:
        if (size != 2)
          goto __skip;
        isapnp_parse_dma_resource(size);
        size = 0;
        break;
      case _STAG_STARTDEP:
        if (size > 1)
          goto __skip;
//priority = 0x100 | PNP_RES_PRIORITY_ACCEPTABLE;
        if (size > 0)
        {
          isapnp_peek(tmp, size);
          priority = 0x100 | tmp[0];
          size = 0;
        }
/*
 * option = pnp_register_dependent_option(dev,priority);
 * if (!option)
 * return 1;
 */
        break;
      case _STAG_ENDDEP:
        if (size != 0)
          goto __skip;
        priority = 0;
        break;
      case _STAG_IOPORT:
        if (size != 7)
          goto __skip;
        isapnp_parse_port_resource(*dev, size);
        size = 0;
        break;
      case _STAG_FIXEDIO:
        if (size != 3)
          goto __skip;
        isapnp_parse_fixed_port_resource(size);
        size = 0;
        break;
      case _STAG_VENDOR:
        break;
      case _LTAG_MEMRANGE:
        if (size != 9)
          goto __skip;
        isapnp_parse_mem_resource(size);
        size = 0;
        break;
      case _LTAG_ANSISTR:
        dev->setProduct(isapnp_parse_name(size));
        break;
      case _LTAG_UNICODESTR:
/*
 * silently ignore
 */
/*
 * who use unicode for hardware identification?
 */
        break;
      case _LTAG_VENDOR:
        break;
      case _LTAG_MEM32RANGE:
        if (size != 17)
          goto __skip;
        isapnp_parse_mem32_resource(size);
        size = 0;
        break;
      case _LTAG_FIXEDMEM32RANGE:
        if (size != 9)
          goto __skip;
        isapnp_parse_fixed_mem32_resource(size);
        size = 0;
        break;
      case _STAG_END:
        if (size > 0)
          isapnp_skip_bytes(size);
        return 1;
      default:
        break;
    }
    __skip:
    if (size > 0)
      isapnp_skip_bytes(size);
  }
  return 0;
}


static string bcd_version(unsigned char v,
const char *separator = "")
{
  char version[10];

  snprintf(version, sizeof(version), "%d%s%d", (v & 0xf0) >> 4, separator,
    v & 0x0f);

  return string(version);
}


/*
 *  Parse resource map for ISA PnP card.
 */

static bool isapnp_parse_resource_map(hwNode & card)
{
  unsigned char type, tmp[17];
  unsigned short size;

  while (1)
  {
    if (isapnp_read_tag(&type, &size) < 0)
      return false;
    switch (type)
    {
      case _STAG_PNPVERNO:
        if (size != 2)
          goto __skip;
        isapnp_peek(tmp, 2);
        card.addCapability("pnp-" + bcd_version(tmp[0], "."));
        card.setVersion(bcd_version(tmp[1]));
        size = 0;
        break;
      case _STAG_LOGDEVID:
        if (size >= 5 && size <= 6)
        {
          if (isapnp_create_device(card, size) == 1)
            return false;
          size = 0;
        }
        break;
      case _STAG_VENDOR:
        break;
      case _LTAG_ANSISTR:
        card.setProduct(isapnp_parse_name(size));
        break;
      case _LTAG_UNICODESTR:
/*
 * silently ignore
 */
/*
 * who uses unicode for hardware identification?
 */
        break;
      case _LTAG_VENDOR:
        break;
      case _STAG_END:
        if (size > 0)
          isapnp_skip_bytes(size);
        return true;
      default:
        break;
    }
    __skip:
    if (size > 0)
      isapnp_skip_bytes(size);
  }

  return true;
}


/*
 *  Compute ISA PnP checksum for first eight bytes.
 */

static unsigned char isapnp_checksum(unsigned char *data)
{
  int i, j;
  unsigned char checksum = 0x6a, bit, b;

  for (i = 0; i < 8; i++)
  {
    b = data[i];
    for (j = 0; j < 8; j++)
    {
      bit = 0;
      if (b & (1 << j))
        bit = 1;
      checksum =
        ((((checksum ^ (checksum >> 1)) & 0x01) ^ bit) << 7) | (checksum >>
        1);
    }
  }
  return checksum;
}


/*
 *  Parse EISA id for ISA PnP card.
 */

static const char *isapnp_parse_card_id(unsigned short vendor,
unsigned short device)
{
  static char id[8];

  snprintf(id, sizeof(id), "%c%c%c%x%x%x%x",
    'A' + ((vendor >> 2) & 0x3f) - 1,
    'A' + (((vendor & 3) << 3) | ((vendor >> 13) & 7)) - 1,
    'A' + ((vendor >> 8) & 0x1f) - 1,
    (device >> 4) & 0x0f,
    device & 0x0f, (device >> 12) & 0x0f, (device >> 8) & 0x0f);
  return id;
}


/*
 *  Build device list for all present ISA PnP devices.
 */

static int isapnp_build_device_list(hwNode & n)
{
  int csn;
  unsigned char header[9], checksum;

  isapnp_wait();
  isapnp_key();
  for (csn = 1; csn <= 10; csn++)
  {
    long serial = 0;

    isapnp_wake(csn);
    isapnp_peek(header, 9);

    hwNode card(isapnp_parse_card_id((header[1] << 8) | header[0],
      (header[3] << 8) | header[2]));

    checksum = isapnp_checksum(header);
/*
 * Don't be strict on the checksum, here !
 * e.g. 'SCM SwapBox Plug and Play' has header[8]==0 (should be: b7)
 */
    if (header[8] == 0)
      ;
                                                  /* not valid CSN */
    else if (checksum == 0x00 || checksum != header[8])
      continue;

    card.setPhysId(csn);
    card.
      setVendor(vendorname
      (isapnp_parse_card_id
      ((header[1] << 8) | header[0],
      (header[3] << 8) | header[2])));
    card.addCapability("pnp");
    card.addCapability("isapnp");
    card.setBusInfo("isapnp@" + card.getPhysId());
    serial =
      (header[7] << 24) | (header[6] << 16) | (header[5] << 8) | header[4];
    if (serial != -1)
    {
      char number[20];

      snprintf(number, sizeof(number), "%ld", serial);
      card.setSerial(number);
    }
    isapnp_checksum_value = 0x00;
    isapnp_parse_resource_map(card);
/*
 * if (isapnp_checksum_value != 0x00)
 * fprintf(stderr, "isapnp: checksum for device %i is not valid (0x%x)\n",
 * csn, isapnp_checksum_value);
 */

    n.addChild(card);
  }
  isapnp_wait();
  return 0;
}


static bool isabus(const hwNode & n)
{
  return (n.getClass() == hw::bridge) && n.isCapable("isa");
}
#endif

bool scan_isapnp(hwNode & n)
{
#ifdef __i386__
  int cards;

  isapnp_rdp = 0;
  isapnp_detected = 1;
  cards = isapnp_isolate();
  if (cards < 0 || (isapnp_rdp < 0x203 || isapnp_rdp > 0x3ff))
  {
    isapnp_detected = 0;
    return false;
  }

  hwNode *isaroot = n.findChild(isabus);

  if (isaroot)
    isapnp_build_device_list(*isaroot);
  else
    isapnp_build_device_list(n);
#endif
  return true;
}
