#include <stdbool.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "support.h"
#include "config.h"
#include "stock.h"
#include "engine.h"

static char *id = "@(#) $Id$";

extern GtkWidget *mainwindow;

int
main (int argc, char *argv[])
{
#ifndef NONLS
  bindtextdomain (PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
#endif

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
  lshw_ui_init();

  if(!mainwindow)
    return(1);

  gtk_widget_show(mainwindow);
  gtk_main ();

  (void) &id;                                     // avoid warning "id defined but not used"

  return 0;
}
