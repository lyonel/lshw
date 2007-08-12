#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdbool.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"
#include "stock.h"
#include "engine.h"

static char *id = "@(#) $Id$";

GtkWidget *mainwindow;

int
main (int argc, char *argv[])
{
  GdkPixbuf *icon;
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  if(geteuid() != 0)
  {
    bool proceed = false;
    GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_NONE,
				"Executing this program as a normal user will give incomplete and maybe erroneous information.");
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                                  GTK_STOCK_QUIT, GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_EXECUTE, GTK_RESPONSE_ACCEPT,
                                  NULL);

    proceed = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT);
    gtk_widget_destroy (dialog);

    if(!proceed)
      return -1;
  }

  lshw_gtk_stock_init();

/*
 * The following code was added by Glade to create one of each component
 * (except popup menus), just so that you see something after building
 * the project. Delete any components that you don't want shown initially.
 */
  mainwindow = create_lshw ();

  icon = gtk_widget_render_icon(GTK_WIDGET(mainwindow),
    "lshw-logo",
    GTK_ICON_SIZE_DIALOG,
    NULL);
  if(GDK_IS_PIXBUF(icon))
  {
    gtk_window_set_icon(GTK_WINDOW(mainwindow), icon);
    gtk_window_set_default_icon(icon);
  }

  gtk_widget_show (mainwindow);

  gtk_main ();

  (void) &id;                                     // avoid warning "id defined but not used"

  return 0;
}
