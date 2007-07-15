#include <gtk/gtk.h>

void refresh_display (GtkMenuItem * menuitem, gpointer user_data);

void on_about1_activate (GtkMenuItem * menuitem, gpointer user_data);

void on_aboutclose_activate (GtkButton * button, gpointer user_data);

void on_version_realize (GtkWidget * widget, gpointer user_data);

void
on_treeview1_row_activated (GtkTreeView * treeview,
			    GtkTreePath * path,
			    GtkTreeViewColumn * column, gpointer user_data);

void
on_treeview2_row_activated (GtkTreeView * treeview,
			    GtkTreePath * path,
			    GtkTreeViewColumn * column, gpointer user_data);

void
on_treeview3_row_activated (GtkTreeView * treeview,
			    GtkTreePath * path,
			    GtkTreeViewColumn * column, gpointer user_data);

void on_treeview3_cursor_changed (GtkTreeView * treeview, gpointer user_data);

void on_treeview1_cursor_changed (GtkTreeView * treeview, gpointer user_data);

void on_treeview2_cursor_changed (GtkTreeView * treeview, gpointer user_data);

void on_treeview1_cursor_changed (GtkTreeView * treeview, gpointer user_data);

void on_treeview2_cursor_changed (GtkTreeView * treeview, gpointer user_data);

void on_treeview3_cursor_changed (GtkTreeView * treeview, gpointer user_data);

void go_up (GtkToolButton * toolbutton, gpointer user_data);

void on_lshw_map (GtkWidget * widget, gpointer user_data);

void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_savebutton_clicked                  (GtkToolButton   *toolbutton,
                                        gpointer         user_data);
