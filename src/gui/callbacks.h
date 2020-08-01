#include <gtk/gtk.h>

void
on_go_up_activated                     (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app);

void
on_refresh_activated                   (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app);

void
on_save_activated                      (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app);

void
on_about_activated                     (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app);

void
on_quit_activated                      (GSimpleAction   *action,
                                        GVariant        *parameter,
                                        gpointer         app);

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

void on_lshw_map (GtkWidget * widget, gpointer user_data);
