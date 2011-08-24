#include <stdbool.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "support.h"
#include "config.h"
#include "stock.h"
#include "engine.h"

static char *id = "@(#) $Id$";

GtkWidget *mainwindow;
GtkWidget *about;
GtkWidget *list1;
GtkWidget *list2;
GtkWidget *list3;
GtkWidget *description;
GtkWidget *go_up_button;
GtkWidget *save_button;
GtkWidget *statusbar;

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GtkBuilder *builder;
  GdkPixbuf *icon;
#ifndef NONLS
  bindtextdomain (PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
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

  builder = gtk_builder_new();
  if( ! gtk_builder_add_from_file( builder, "gtk-lshw.ui", &error ) )
  {
    g_warning( "%s", error->message );
    g_free( error );
    return( 1 );
  }

  mainwindow = GTK_WIDGET( gtk_builder_get_object( builder, "gtk-lshw" ) );
  about = GTK_WIDGET( gtk_builder_get_object( builder, "aboutlshw" ) );
  list1 = GTK_WIDGET(gtk_builder_get_object( builder, "treeview1"));
  list2 = GTK_WIDGET(gtk_builder_get_object( builder, "treeview2"));
  list3 = GTK_WIDGET(gtk_builder_get_object( builder, "treeview3"));
  description = GTK_WIDGET(gtk_builder_get_object( builder, "description"));
  go_up_button = GTK_WIDGET(gtk_builder_get_object( builder, "upbutton"));
  save_button = GTK_WIDGET(gtk_builder_get_object( builder, "savebutton"));
  statusbar = GTK_WIDGET(gtk_builder_get_object( builder, "statusbar"));
  gtk_builder_connect_signals( builder, NULL );
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

  gtk_widget_show (mainwindow);

  gtk_main ();

  (void) &id;                                     // avoid warning "id defined but not used"

  return 0;
}
