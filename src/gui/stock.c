#include "stock.h"
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

static char *id = "@(#) $Id$";

#define UIFILE "gtk-lshw.ui"

GtkWidget *mainwindow = NULL;
GtkWidget *about = NULL;
GtkWidget *list1 = NULL;
GtkWidget *list2 = NULL;
GtkWidget *list3 = NULL;
GtkWidget *description = NULL;
GtkWidget *go_up_button = NULL;
GtkWidget *save_button = NULL;
GtkWidget *statusbar = NULL;

static struct StockIcon
{
  const char *name;
  const char *filename;

} const stock_icons[] =
{
  { LSHW_STOCK_AUDIO, "audio.svg" },
  { LSHW_STOCK_AMD, "amd.svg" },
  { LSHW_STOCK_BATTERY, "battery.svg" },
  { LSHW_STOCK_BLUETOOTH, "bluetooth.svg" },
  { LSHW_STOCK_BOARD, "board.svg" },
  { LSHW_STOCK_CHIP, "chip.svg" },
  { LSHW_STOCK_CPU, "cpu.svg" },
  { LSHW_STOCK_DESKTOPCOMPUTER, "desktopcomputer.svg" },
  { LSHW_STOCK_DISABLED, "disabled.svg" },
  { LSHW_STOCK_DISPLAY, "display.svg" },
  { LSHW_STOCK_CD, "cd.svg" },
  { LSHW_STOCK_DISC, "disc.svg" },
  { LSHW_STOCK_FIREWIRE, "firewire.svg" },
  { LSHW_STOCK_INTEL, "intel.svg" },
  { LSHW_STOCK_LAPTOP, "laptop.svg" },
  { LSHW_STOCK_LOGO, "logo.svg" },
  { LSHW_STOCK_MEMORY, "memory.svg" },
  { LSHW_STOCK_MINI, "mini.svg" },
  { LSHW_STOCK_MODEM, "modem.svg" },
  { LSHW_STOCK_MOTHERBOARD, "motherboard.svg" },
  { LSHW_STOCK_NETWORK, "network.svg" },
  { LSHW_STOCK_PARALLEL, "parallel.svg" },
  { LSHW_STOCK_POWERMAC, "powermac.svg" },
  { LSHW_STOCK_POWERMACG5, "powermacg5.svg" },
  { LSHW_STOCK_POWERPC, "powerpc.svg" },
  { LSHW_STOCK_PRINTER, "printer.svg" },
  { LSHW_STOCK_RADIO, "radio.svg" },
  { LSHW_STOCK_MD, "md.svg" },
  { LSHW_STOCK_SCSI, "scsi.svg" },
  { LSHW_STOCK_SERIAL, "serial.svg" },
  { LSHW_STOCK_TOWERCOMPUTER, "towercomputer.svg" },
  { LSHW_STOCK_USB, "usb.svg" },
  { LSHW_STOCK_WIFI, "wifi.svg" },
};

static gchar *
find_file(const char *base, const char *dir)
{
  char *filename;
  char *basedir;

  if (base == NULL)
    return NULL;

  if((basedir = getenv("BASEDIR")))
    filename = g_build_filename(basedir, dir, base, NULL);
  else
    filename = g_build_filename(DATADIR, "lshw", dir, base, NULL);

  if (!g_file_test(filename, G_FILE_TEST_EXISTS))
  {
    g_free(filename);
    return NULL;
  }
  else
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

      filename = find_file(stock_icons[i].filename, "artwork");

      if (filename == NULL)
        continue;

      pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
      g_free(filename);

      if(pixbuf)	/* we managed to load something */
      {
        iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
        g_object_unref(G_OBJECT(pixbuf));
        gtk_icon_factory_add(icon_factory, stock_icons[i].name, iconset);
        gtk_icon_set_unref(iconset);
      }
  }

  gtk_widget_destroy(win);

/* register logo icon size */
  gtk_icon_size_register(LSHW_ICON_SIZE_LOGO, LSHW_DEFAULT_ICON_SIZE, LSHW_DEFAULT_ICON_SIZE);

  g_object_unref(G_OBJECT(icon_factory));

  (void) &id;                                     /* avoid "id defined but not used" warning */
}

void lshw_ui_init(void)
{
  GError *error = NULL;
  GtkBuilder *builder = NULL;
  GdkPixbuf *icon = NULL;
  gchar *uiname = NULL;

  mainwindow = NULL;

  builder = gtk_builder_new();
  uiname = find_file(UIFILE, "ui");
  if(!uiname)
  {
    g_critical( "Could not find UI file: %s", UIFILE );
    return;
  }
  if(!gtk_builder_add_from_file(builder, uiname, &error))
  {
    g_critical( "Could not create UI: %s", error->message );
    g_free( error );
    g_object_unref( G_OBJECT( builder ) );
    return;
  }
  g_free(uiname);

  mainwindow = GTK_WIDGET( gtk_builder_get_object( builder, "mainwindow" ) );
  about = GTK_WIDGET( gtk_builder_get_object( builder, "aboutlshw" ) );
  list1 = GTK_WIDGET(gtk_builder_get_object( builder, "treeview1"));
  list2 = GTK_WIDGET(gtk_builder_get_object( builder, "treeview2"));
  list3 = GTK_WIDGET(gtk_builder_get_object( builder, "treeview3"));
  description = GTK_WIDGET(gtk_builder_get_object( builder, "description"));
  go_up_button = GTK_WIDGET(gtk_builder_get_object( builder, "upbutton"));
  save_button = GTK_WIDGET(gtk_builder_get_object( builder, "savebutton"));
  statusbar = GTK_WIDGET(gtk_builder_get_object( builder, "statusbar"));
  gtk_builder_connect_signals( builder, mainwindow );
  g_object_unref( G_OBJECT( builder ) );

  icon = gtk_widget_render_icon(GTK_WIDGET(mainwindow),
    "lshw-logo",
    GTK_ICON_SIZE_DIALOG,
    NULL);
  if(GDK_IS_PIXBUF(icon))
  {
    gtk_window_set_icon(GTK_WINDOW(mainwindow), icon);
    gtk_window_set_default_icon(icon);
  }
}
