#include "engine.h"
#include "hw.h"
#include "main.h"

extern "C" {
#include "support.h"
};

#define YIELD()  while(gtk_events_pending()) gtk_main_iteration()

static hwNode computer("computer", hw::system);
static GtkWidget *statusbar = NULL;

void status(const char *message)
{
  static guint context_id = 0;

  if(!GTK_IS_WIDGET(statusbar)) return;
  
  if(!context_id)
    context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "scanning");
  else
    gtk_statusbar_pop(GTK_STATUSBAR(statusbar), context_id);

  if(message) gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, message);

  YIELD();
}


void refresh(GtkWidget *mainwindow)
{
  statusbar = lookup_widget(mainwindow, "statusbar");

  if(!GTK_IS_WIDGET(statusbar)) return;


  computer = hwNode("computer", hw::system);
  status("Scanning...");
  scan_system(computer);
  status(NULL);
}
