#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "version.h"

static GtkWidget *about = NULL;
static GtkWidget *saveas = NULL;

void
save_as                                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  /*if(!GTK_IS_WIDGET(saveas))
  {
    saveas = create_saveas();
    gtk_widget_show(saveas);
  }*/
}


void
on_copy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
refresh_display                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

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
  gtk_label_set_text(GTK_LABEL(widget), getpackageversion());
}


