#include <gtk/gtk.h>


void
save_as                                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_copy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
refresh_display                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_aboutclose_activate                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_version_realize                     (GtkWidget       *widget,
                                        gpointer         user_data);

