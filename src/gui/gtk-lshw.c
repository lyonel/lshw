#include <stdbool.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "support.h"
#include "config.h"
#include "stock.h"
#include "engine.h"

static char *id = "@(#) $Id$";

extern GtkWidget *mainwindow;

static void
activate (GApplication *app,
          gpointer      user_data)
{
  if(geteuid() != 0)
  {
    bool proceed = false;
    GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_NONE,
				"Executing this program as a normal user will give incomplete and maybe erroneous information.");
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                                  "_Quit", GTK_RESPONSE_CANCEL,
                                  "_Execute", GTK_RESPONSE_ACCEPT,
                                  NULL);

    proceed = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT);
    gtk_widget_destroy (dialog);

    if(!proceed)
      return;
  }

  lshw_gtk_stock_init();
  lshw_ui_init();

  if(!mainwindow)
    return;

  gtk_widget_show(mainwindow);
  gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(mainwindow));
}

int
main (int argc, char *argv[])
{
#ifndef NONLS
  bindtextdomain (PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
#endif

  GtkApplication *app = gtk_application_new ("org.ezix.gtk-lshw", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  (void) &id;                                     // avoid warning "id defined but not used"

  return status;
}
