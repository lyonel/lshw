/*
 * fb.cc
 *
 *
 */

#include "version.h"
#include "fb.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

__ID("@(#) $Id$");

#define FB_MODES_SHIFT  5                         /* 32 modes per framebuffer */
#define FB_NUM_MINORS 256                         /* 256 Minors               */
#define MAX_FB    (FB_NUM_MINORS / (1 << FB_MODES_SHIFT))

/* ioctls
   0x46 is 'F'                                                          */
#define FBIOGET_VSCREENINFO     0x4600
#define FBIOPUT_VSCREENINFO     0x4601
#define FBIOGET_FSCREENINFO     0x4602
#define FBIOGETCMAP             0x4604
#define FBIOPUTCMAP             0x4605
#define FBIOPAN_DISPLAY         0x4606
/* 0x4607-0x460B are defined below */
/* #define FBIOGET_MONITORSPEC  0x460C */
/* #define FBIOPUT_MONITORSPEC  0x460D */
/* #define FBIOSWITCH_MONIBIT   0x460E */
#define FBIOGET_CON2FBMAP       0x460F
#define FBIOPUT_CON2FBMAP       0x4610

#define FB_TYPE_PACKED_PIXELS           0         /* Packed Pixels        */
#define FB_TYPE_PLANES                  1         /* Non interleaved planes */
#define FB_TYPE_INTERLEAVED_PLANES      2         /* Interleaved planes   */
#define FB_TYPE_TEXT                    3         /* Text/attributes      */
#define FB_TYPE_VGA_PLANES              4         /* EGA/VGA planes       */

#define FB_AUX_TEXT_MDA         0                 /* Monochrome text */
#define FB_AUX_TEXT_CGA         1                 /* CGA/EGA/VGA Color text */
#define FB_AUX_TEXT_S3_MMIO     2                 /* S3 MMIO fasttext */
#define FB_AUX_TEXT_MGA_STEP16  3                 /* MGA Millenium I: text, attr, 14 reserved bytes */
#define FB_AUX_TEXT_MGA_STEP8   4                 /* other MGAs:      text, attr,  6 reserved bytes */

#define FB_AUX_VGA_PLANES_VGA4          0         /* 16 color planes (EGA/VGA) */
#define FB_AUX_VGA_PLANES_CFB4          1         /* CFB4 in planes (VGA) */
#define FB_AUX_VGA_PLANES_CFB8          2         /* CFB8 in planes (VGA) */

#define FB_VISUAL_MONO01                0         /* Monochr. 1=Black 0=White */
#define FB_VISUAL_MONO10                1         /* Monochr. 1=White 0=Black */
#define FB_VISUAL_TRUECOLOR             2         /* True color   */
#define FB_VISUAL_PSEUDOCOLOR           3         /* Pseudo color (like atari) */
#define FB_VISUAL_DIRECTCOLOR           4         /* Direct color */
#define FB_VISUAL_STATIC_PSEUDOCOLOR    5         /* Pseudo color readonly */

#define FB_ACCEL_NONE           0                 /* no hardware accelerator      */
#define FB_ACCEL_ATARIBLITT     1                 /* Atari Blitter                */
#define FB_ACCEL_AMIGABLITT     2                 /* Amiga Blitter                */
#define FB_ACCEL_S3_TRIO64      3                 /* Cybervision64 (S3 Trio64)    */
#define FB_ACCEL_NCR_77C32BLT   4                 /* RetinaZ3 (NCR 77C32BLT)      */
#define FB_ACCEL_S3_VIRGE       5                 /* Cybervision64/3D (S3 ViRGE)  */
#define FB_ACCEL_ATI_MACH64GX   6                 /* ATI Mach 64GX family         */
#define FB_ACCEL_DEC_TGA        7                 /* DEC 21030 TGA                */
#define FB_ACCEL_ATI_MACH64CT   8                 /* ATI Mach 64CT family         */
#define FB_ACCEL_ATI_MACH64VT   9                 /* ATI Mach 64CT family VT class */
#define FB_ACCEL_ATI_MACH64GT   10                /* ATI Mach 64CT family GT class */
#define FB_ACCEL_SUN_CREATOR    11                /* Sun Creator/Creator3D        */
#define FB_ACCEL_SUN_CGSIX      12                /* Sun cg6                      */
#define FB_ACCEL_SUN_LEO        13                /* Sun leo/zx                   */
#define FB_ACCEL_IMS_TWINTURBO  14                /* IMS Twin Turbo               */
#define FB_ACCEL_3DLABS_PERMEDIA2 15              /* 3Dlabs Permedia 2            */
#define FB_ACCEL_MATROX_MGA2064W 16               /* Matrox MGA2064W (Millenium)  */
#define FB_ACCEL_MATROX_MGA1064SG 17              /* Matrox MGA1064SG (Mystique)  */
#define FB_ACCEL_MATROX_MGA2164W 18               /* Matrox MGA2164W (Millenium II) */
#define FB_ACCEL_MATROX_MGA2164W_AGP 19           /* Matrox MGA2164W (Millenium II) */
#define FB_ACCEL_MATROX_MGAG100 20                /* Matrox G100 (Productiva G100) */
#define FB_ACCEL_MATROX_MGAG200 21                /* Matrox G200 (Myst, Mill, ...) */
#define FB_ACCEL_SUN_CG14       22                /* Sun cgfourteen                */
#define FB_ACCEL_SUN_BWTWO      23                /* Sun bwtwo                     */
#define FB_ACCEL_SUN_CGTHREE    24                /* Sun cgthree                   */
#define FB_ACCEL_SUN_TCX        25                /* Sun tcx                       */
#define FB_ACCEL_MATROX_MGAG400 26                /* Matrox G400                   */

#define FB_VMODE_NONINTERLACED  0                 /* non interlaced */
#define FB_VMODE_INTERLACED     1                 /* interlaced   */
#define FB_VMODE_DOUBLE         2                 /* double scan */
#define FB_VMODE_MASK           255

struct fb_fix_screeninfo
{
  char id[16];                                    /* identification string eg "TT Builtin" */
  char *smem_start;                               /* Start of frame buffer mem */
/*
 * (physical address)
 */
  u_int32_t smem_len;                             /* Length of frame buffer mem */
  u_int32_t type;                                 /* see FB_TYPE_*                */
  u_int32_t type_aux;                             /* Interleave for interleaved Planes */
  u_int32_t visual;                               /* see FB_VISUAL_*              */
  u_int16_t xpanstep;                             /* zero if no hardware panning  */
  u_int16_t ypanstep;                             /* zero if no hardware panning  */
  u_int16_t ywrapstep;                            /* zero if no hardware ywrap    */
  u_int32_t line_length;                          /* length of a line in bytes    */
  char *mmio_start;                               /* Start of Memory Mapped I/O   */
/*
 * (physical address)
 */
  u_int32_t mmio_len;                             /* Length of Memory Mapped I/O  */
  u_int32_t accel;                                /* Type of acceleration available */
  u_int16_t reserved[3];                          /* Reserved for future compatibility */
};

struct fb_bitfield
{
  u_int32_t offset;                               /* beginning of bitfield        */
  u_int32_t length;                               /* length of bitfield           */
  u_int32_t msb_right;                            /* != 0 : Most significant bit is */
/*
 * right
 */
};

struct fb_var_screeninfo
{
  u_int32_t xres;                                 /* visible resolution           */
  u_int32_t yres;
  u_int32_t xres_virtual;                         /* virtual resolution           */
  u_int32_t yres_virtual;
  u_int32_t xoffset;                              /* offset from virtual to visible */
  u_int32_t yoffset;                              /* resolution                   */

  u_int32_t bits_per_pixel;                       /* guess what                   */
  u_int32_t grayscale;                            /* != 0 Graylevels instead of colors */

  struct fb_bitfield red;                         /* bitfield in fb mem if true color, */
  struct fb_bitfield green;                       /* else only length is significant */
  struct fb_bitfield blue;
  struct fb_bitfield transp;                      /* transparency                 */

  u_int32_t nonstd;                               /* != 0 Non standard pixel format */

  u_int32_t activate;                             /* see FB_ACTIVATE_*            */

  u_int32_t height;                               /* height of picture in mm    */
  u_int32_t width;                                /* width of picture in mm     */

  u_int32_t accel_flags;                          /* acceleration flags (hints)   */

/*
 * Timing: All values in pixclocks, except pixclock (of course)
 */
  u_int32_t pixclock;                             /* pixel clock in ps (pico seconds) */
  u_int32_t left_margin;                          /* time from sync to picture    */
  u_int32_t right_margin;                         /* time from picture to sync    */
  u_int32_t upper_margin;                         /* time from sync to picture    */
  u_int32_t lower_margin;
  u_int32_t hsync_len;                            /* length of horizontal sync    */
  u_int32_t vsync_len;                            /* length of vertical sync      */
  u_int32_t sync;                                 /* see FB_SYNC_*                */
  u_int32_t vmode;                                /* see FB_VMODE_*               */
  u_int32_t reserved[6];                          /* Reserved for future compatibility */
};

static int lookup_dev(const char *name)
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
}


static int open_dev(dev_t dev)
{
  static const char *paths[] =
  {
    "/var/run", "/dev", "/tmp", NULL
  };
  char const **p;
  char fn[64];
  int fd;

  for (p = paths; *p; p++)
  {
    sprintf(fn, "%s/fb-%d", *p, getpid());
    if (mknod(fn, (S_IFCHR | S_IREAD), dev) == 0)
    {
      fd = open(fn, O_RDONLY);
      unlink(fn);
      if (fd >= 0)
        return fd;
    }
  }
  return -1;
}


bool scan_fb(hwNode & n)
{
  int fd[MAX_FB];
  unsigned int fbdevs = 0;
  unsigned int i;
  int major = lookup_dev("fb");

  if (major < 0)                                  // framebuffer support not loaded, there isn't
    return false;                                 // much we can do

  memset(fd, 0, sizeof(fd));
  for (i = 0; i < MAX_FB; i++)
  {
    fd[i] = open_dev((dev_t) ((major << 8) + i));

    if (fd[i] >= 0)
    {
      hwNode *fbdev = NULL;
      struct fb_fix_screeninfo fbi;

      if (ioctl(fd[i], FBIOGET_FSCREENINFO, &fbi) == 0)
      {
        fbdev =
          n.
          findChildByResource(hw::resource::
          iomem((unsigned long) fbi.smem_start,
          fbi.smem_len));

        if (fbdev)
        {
          char devname[20];
          struct fb_var_screeninfo fbconfig;

          snprintf(devname, sizeof(devname), "/dev/fb%d", i);
          fbdev->setLogicalName(devname);
          fbdev->claim();
          if (fbdev->getDescription() == "")
            fbdev->setDescription(hw::strip(fbi.id));
          fbdev->addCapability("fb");

          switch (fbi.visual)
          {
            case FB_VISUAL_MONO01:
              fbdev->setConfig("visual", "mono01");
              break;
            case FB_VISUAL_MONO10:
              fbdev->setConfig("visual", "mono10");
              break;
            case FB_VISUAL_TRUECOLOR:
              fbdev->setConfig("visual", "truecolor");
              break;
            case FB_VISUAL_PSEUDOCOLOR:
              fbdev->setConfig("visual", "pseudocolor");
              break;
            case FB_VISUAL_DIRECTCOLOR:
              fbdev->setConfig("visual", "directcolor");
              break;
            case FB_VISUAL_STATIC_PSEUDOCOLOR:
              fbdev->setConfig("visual", "static_pseudocolor");
              break;
          }

          if (fbi.accel != FB_ACCEL_NONE)
            fbdev->addCapability("accelerated");

          if (ioctl(fd[i], FBIOGET_VSCREENINFO, &fbconfig) == 0)
          {
            char vidmode[20];
            unsigned int htotal = 0;
            unsigned int vtotal = 0;

            snprintf(vidmode, sizeof(vidmode), "%dx%d", fbconfig.xres,
              fbconfig.yres);
            fbdev->setConfig("mode", vidmode);
            snprintf(vidmode, sizeof(vidmode), "%d", fbconfig.xres);
            fbdev->setConfig("xres", vidmode);
            snprintf(vidmode, sizeof(vidmode), "%d", fbconfig.yres);
            fbdev->setConfig("yres", vidmode);
            snprintf(vidmode, sizeof(vidmode), "%d", fbconfig.bits_per_pixel);
            fbdev->setConfig("depth", vidmode);

            vtotal =
              fbconfig.upper_margin + fbconfig.yres + fbconfig.lower_margin +
              fbconfig.vsync_len;
            htotal =
              fbconfig.left_margin + fbconfig.xres + fbconfig.right_margin +
              fbconfig.hsync_len;
            switch (fbconfig.vmode & FB_VMODE_MASK)
            {
              case FB_VMODE_INTERLACED:
                vtotal >>= 1;
                break;
              case FB_VMODE_DOUBLE:
                vtotal <<= 1;
                break;
            }

            if (fbconfig.pixclock)
            {
              char freq[20];

              double drate = 1E12 / fbconfig.pixclock;
              double hrate = drate / htotal;
              double vrate = hrate / vtotal;

              snprintf(freq, sizeof(freq), "%5.2fHz", vrate);
              fbdev->setConfig("frequency", freq);
            }

          }
        }
      }
    }
    else
      break;
  }

  for (unsigned int j = 0; j < fbdevs; j++)
  {
    close(fd[j]);
  }

  return false;
}
