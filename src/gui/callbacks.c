#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "support.h"
#include "version.h"
#include "engine.h"
#include <string.h>

extern GtkWidget *mainwindow;

static char *id = "@(#) $Id$";

G_MODULE_EXPORT
void
on_go_up_activated                     (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app)
{
  go_back(mainwindow);
}

G_MODULE_EXPORT
void
on_refresh_activated                   (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app)
{
  refresh(mainwindow);
}

G_MODULE_EXPORT
void
on_save_activated                      (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app)
{
  save_as(mainwindow);
}

G_MODULE_EXPORT
void
on_quit_activated                      (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app)
{
  g_application_quit(G_APPLICATION(app));
}

G_MODULE_EXPORT
void
on_about_activated                     (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app)
{
  gtk_show_about_dialog(GTK_WINDOW(mainwindow),
                        "program-name", "GTK+ frontend for lshw",
                        "website", "https://www.ezix.org/",
                        "copyright", "© 2004-2011 Lyonel Vincent\n© 2020 Emmanuel Gil Peyrot",
                        "version", getpackageversion(),
                        "license-type", GTK_LICENSE_GPL_2_0,
                        NULL);

  const char *latest = checkupdates();

  if(latest)
  {
    if(strcmp(latest, getpackageversion()) != 0)
    {
      static GtkWidget *dialog = NULL;

      if(!GTK_IS_WIDGET(dialog))
      {
        dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(mainwindow),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  "The latest version is <tt>%s</tt>.\n\nYou can visit <span foreground=\"blue\"><u>http://www.ezix.org/</u></span> for more information.",
                                  latest);

        gtk_window_set_title(GTK_WINDOW(dialog), "Update available");
      }

      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
    }
  }
}

G_MODULE_EXPORT
void
on_treeview1_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(1, treeview);
}


G_MODULE_EXPORT
void
on_treeview2_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(2, treeview);
}


G_MODULE_EXPORT
void
on_treeview3_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(3, treeview);
}


G_MODULE_EXPORT
void
on_treeview1_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(1, treeview);
}


G_MODULE_EXPORT
void
on_treeview2_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(2, treeview);
}


G_MODULE_EXPORT
void
on_treeview3_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(3, treeview);

  (void) &id;                                     // avoid warning "id defined but not used"
}


G_MODULE_EXPORT
void
on_lshw_map                            (GtkWidget       *widget,
gpointer         user_data)
{
  refresh(mainwindow);
}
