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
static hwNode *root = NULL;
static GtkWidget *statusbar = NULL;

extern GtkWidget *mainwindow;

enum
  {
    COL_NAME = 0,
    COL_NODE,
    COL_WEIGHT,
    NUM_COLS
  };

static string curpath = "/";

static void populate_list(GtkWidget * list1, hwNode * root)
{
  GtkListStore *list_store = NULL;
  GtkTreeViewColumn   *col = NULL;
  GtkCellRenderer     *renderer = NULL;

  list_store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT);

  GtkTreeIter iter;
  string text;

  gtk_list_store_append(list_store, &iter);

  text = root->getDescription();
  if(text == "")
    text = root->getProduct();
  if(text == "")
    text = root->getId();

  gtk_list_store_set (list_store, &iter,
                    COL_NAME, text.c_str(),
                    COL_NODE, root,
                    COL_WEIGHT, (root->countChildren()>0)?PANGO_WEIGHT_BOLD:PANGO_WEIGHT_NORMAL,
                    -1);

  col = gtk_tree_view_column_new();
  gtk_tree_view_append_column(GTK_TREE_VIEW(list1), col);

  renderer = gtk_cell_renderer_text_new();

  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_NAME);
  gtk_tree_view_column_add_attribute(col, renderer, "weight", COL_WEIGHT);

#if 0
    col = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(list1), col);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    g_object_set(renderer, "text", ">", NULL);
#endif
  gtk_tree_view_set_model(GTK_TREE_VIEW(list1), GTK_TREE_MODEL(list_store));
  g_object_unref(list_store);

  YIELD();
}

static void populate_sublist(GtkWidget * list1, hwNode * root)
{
  GtkListStore *list_store = NULL;
  GtkTreeViewColumn   *col = NULL;
  GtkCellRenderer     *renderer = NULL;

  list_store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT);

  for(unsigned i = 0; i<root->countChildren(); i++)
  {
    GtkTreeIter iter;
    string text;

    gtk_list_store_append(list_store, &iter);

    text = root->getChild(i)->getDescription();
    if(text == "")
      text = root->getChild(i)->getProduct();
    if(text == "")
      text = root->getChild(i)->getId();

    gtk_list_store_set (list_store, &iter,
                      COL_NAME, text.c_str(),
                      COL_NODE, root->getChild(i),
                      COL_WEIGHT, (root->getChild(i)->countChildren()>0)?PANGO_WEIGHT_BOLD:PANGO_WEIGHT_NORMAL,
                      -1);
  }

  col = gtk_tree_view_column_new();
  gtk_tree_view_append_column(GTK_TREE_VIEW(list1), col);

  renderer = gtk_cell_renderer_text_new();

  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_NAME);
  gtk_tree_view_column_add_attribute(col, renderer, "weight", COL_WEIGHT);

  gtk_tree_view_set_model(GTK_TREE_VIEW(list1), GTK_TREE_MODEL(list_store));
  g_object_unref(list_store);

  YIELD();
}

static void display(GtkWidget * mainwindow)
{
  GtkWidget * description = lookup_widget(mainwindow, "description");

  if(!selected)
    gtk_label_set_text(GTK_LABEL(description), printmarkup(computer).c_str());
  else
    gtk_label_set_text(GTK_LABEL(description), printmarkup(*selected).c_str());
  gtk_label_set_use_markup (GTK_LABEL(description), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL(description), TRUE);

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
  GtkWidget * list1 = lookup_widget(mainwindow, "treeview1");
  GtkWidget * list2 = lookup_widget(mainwindow, "treeview2");
  GtkWidget * list3 = lookup_widget(mainwindow, "treeview3");
  statusbar = lookup_widget(mainwindow, "statusbar");

  computer = hwNode("computer", hw::system);
  status("Scanning...");
  scan_system(computer);
  status(NULL);


  //selected = computer.getChild("core");

  if(!root) root = &computer;

  populate_list(list1, root);
  populate_sublist(list2, root);
  populate_sublist(list3, root->getChild(0));
  display(mainwindow);
}

void activate(GtkTreeView *treeview,
              GtkTreePath     *path,
              GtkTreeViewColumn *column,
              gpointer         user_data)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  model = gtk_tree_view_get_model(treeview);

  selected = NULL;

  if(gtk_tree_model_get_iter(model, &iter, path))
    gtk_tree_model_get(model, &iter, COL_NODE, &selected, -1);

  display(mainwindow);
}

