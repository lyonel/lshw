#include "engine.h"
#include "hw.h"
#include "main.h"
#include "print.h"

extern "C" {
#include "support.h"
};

#define YIELD()  while(gtk_events_pending()) gtk_main_iteration()

static hwNode computer("computer", hw::system);
static hwNode *selected = NULL;
static GtkWidget *statusbar = NULL;

static string curpath = "/";

static void display(GtkWidget * mainwindow)
{
  GtkWidget * description = lookup_widget(mainwindow, "description");

  if(!GTK_IS_WIDGET(description)) return;

  selected = computer.getChild("core/cpu");

  if(!selected)
    gtk_label_set_text(GTK_LABEL(description), printmarkup(computer).c_str());
  else
    gtk_label_set_text(GTK_LABEL(description), printmarkup(*selected).c_str());
  gtk_label_set_use_markup (GTK_LABEL(description), TRUE);

  YIELD();
}

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

  display(mainwindow);
}
