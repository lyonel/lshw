#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "version.h"
#include "engine.h"

static GtkWidget *about = NULL;
extern GtkWidget *mainwindow;

static char *id = "@(#) $Id$";

void
refresh_display                        (GtkMenuItem     *menuitem,
gpointer         user_data)
{
  refresh(mainwindow);
}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
gpointer         user_data)
{
  if(!GTK_IS_WIDGET(about))
  {
    about = create_aboutlshw();
    gtk_widget_show(about);
  }
}


void
on_aboutclose_activate          (GtkButton       *button,
gpointer         user_data)
{
  if(GTK_IS_WIDGET(about))
  {
    gtk_widget_destroy(about);
  }
}


void
on_version_realize                     (GtkWidget       *widget,
gpointer         user_data)
{
  char *latest = checkupdates();
  char *msg = NULL;

  if(latest)
  {
    if(strcmp(latest, getpackageversion()) != 0)
      msg = g_strdup_printf("%s\nlatest version is %s", getpackageversion(), latest);
  }
  if(!msg)
    msg = g_strdup(getpackageversion());
  gtk_label_set_text(GTK_LABEL(widget), msg);
  g_free(msg);
}


void
on_treeview1_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(1, treeview);
}


void
on_treeview2_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(2, treeview);
}


void
on_treeview3_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(3, treeview);
}


void
on_treeview1_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(1, treeview);
}


void
on_treeview2_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(2, treeview);
}


void
on_treeview3_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(3, treeview);

  (void) &id;                                     // avoid warning "id defined but not used"
}


void
go_up                                  (GtkToolButton   *toolbutton,
gpointer         user_data)
{
  go_back(mainwindow);
}


void
on_lshw_map                            (GtkWidget       *widget,
gpointer         user_data)
{
  refresh(mainwindow);
}

void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  save_as(mainwindow);
}


void
on_savebutton_clicked                  (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
  on_save_activate(NULL, NULL);
}

