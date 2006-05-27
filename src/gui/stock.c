#include "stock.h"
#include <string.h>
#include <gtk/gtk.h>

static char *id = "@(#) $Id$";

static struct StockIcon
{
  const char *name;
  const char *dir;
  const char *filename;

} const stock_icons[] =
{
  { LSHW_STOCK_AUDIO, ".",   "audio.svg" },
  { LSHW_STOCK_AMD, ".",   "amd.svg" },
  { LSHW_STOCK_BATTERY, ".",   "battery.svg" },
  { LSHW_STOCK_BLUETOOTH, ".",   "bluetooth.svg" },
  { LSHW_STOCK_BOARD, ".",   "board.svg" },
  { LSHW_STOCK_CHIP, ".",   "chip.svg" },
  { LSHW_STOCK_CPU, ".",   "cpu.svg" },
  { LSHW_STOCK_DESKTOPCOMPUTER, ".",   "desktopcomputer.svg" },
  { LSHW_STOCK_DISABLED, ".",   "disabled.svg" },
  { LSHW_STOCK_DISPLAY, ".",   "display.svg" },
  { LSHW_STOCK_CD, ".",   "cd.svg" },
  { LSHW_STOCK_DISC, ".",   "disc.svg" },
  { LSHW_STOCK_FIREWIRE, ".",   "firewire.svg" },
  { LSHW_STOCK_INTEL, ".",   "intel.svg" },
  { LSHW_STOCK_LAPTOP, ".",   "laptop.svg" },
  { LSHW_STOCK_LOGO, ".",   "logo.svg" },
  { LSHW_STOCK_MEMORY, ".",   "memory.svg" },
  { LSHW_STOCK_MINI, ".",   "mini.svg" },
  { LSHW_STOCK_MODEM, ".",   "modem.svg" },
  { LSHW_STOCK_MOTHERBOARD, ".",   "motherboard.svg" },
  { LSHW_STOCK_NETWORK, ".",   "network.svg" },
  { LSHW_STOCK_PARALLEL, ".",   "parallel.svg" },
  { LSHW_STOCK_POWERMAC, ".",   "powermac.svg" },
  { LSHW_STOCK_POWERMACG5, ".",   "powermacg5.svg" },
  { LSHW_STOCK_POWERPC, ".",   "powerpc.svg" },
  { LSHW_STOCK_PRINTER, ".",   "printer.svg" },
  { LSHW_STOCK_RADIO, ".",   "radio.svg" },
  { LSHW_STOCK_MD, ".",   "md.svg" },
  { LSHW_STOCK_SCSI, ".",   "scsi.svg" },
  { LSHW_STOCK_SERIAL, ".",   "serial.svg" },
  { LSHW_STOCK_TOWERCOMPUTER, ".",   "towercomputer.svg" },
  { LSHW_STOCK_USB, ".",   "usb.svg" },
  { LSHW_STOCK_WIFI, ".",   "wifi.svg" },
};

static gchar *
find_file(const char *dir, const char *base)
{
  char *filename;

  if (base == NULL)
    return NULL;

  if (!strcmp(dir, "lshw"))
    filename = g_build_filename(DATADIR, "lshw", "artwork", base, NULL);
  else
  {
    filename = g_build_filename(DATADIR, "lshw", "artwork", dir, base, NULL);
  }

  if (!g_file_test(filename, G_FILE_TEST_EXISTS))
  {
    g_critical("Unable to load stock pixmap %s\n", base);

    g_free(filename);

    return NULL;
  }

  return filename;
}


void
lshw_gtk_stock_init(void)
{
  static int stock_initted = 0;
  GtkIconFactory *icon_factory;
  int i;
  GtkWidget *win;

  if (stock_initted)
    return;

  stock_initted = 1;

/* Setup the icon factory. */
  icon_factory = gtk_icon_factory_new();

  gtk_icon_factory_add_default(icon_factory);

/* Er, yeah, a hack, but it works. :) */
  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize(win);

  for (i = 0; i < G_N_ELEMENTS(stock_icons); i++)
  {
    GdkPixbuf *pixbuf;
    GtkIconSet *iconset;
    gchar *filename;

    if (stock_icons[i].dir == NULL)
    {
/* GTK+ Stock icon */
      iconset = gtk_style_lookup_icon_set(gtk_widget_get_style(win),
        stock_icons[i].filename);
    }
    else
    {
      filename = find_file(stock_icons[i].dir, stock_icons[i].filename);

      if (filename == NULL)
        continue;

      pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

      g_free(filename);

      iconset = gtk_icon_set_new_from_pixbuf(pixbuf);

      g_object_unref(G_OBJECT(pixbuf));
    }

    gtk_icon_factory_add(icon_factory, stock_icons[i].name, iconset);

    gtk_icon_set_unref(iconset);
  }

  gtk_widget_destroy(win);

/* register logo icon size */
  gtk_icon_size_register(LSHW_ICON_SIZE_LOGO, 40, 40);

  g_object_unref(G_OBJECT(icon_factory));

  (void) &id;                                     /* avoid "id defined but not used" warning */
}
