#include "engine.h"
#include "hw.h"
#include "main.h"
#include "print.h"

static char *id = "@(#) $Id$";

extern "C" {
#include "support.h"
};

#define YIELD()  while(gtk_events_pending()) gtk_main_iteration()

static hwNode container("container", hw::generic);
static hwNode *displayed = NULL;
static hwNode *selected1 = NULL;
static hwNode *selected2 = NULL;
static hwNode *selected3 = NULL;
static GtkWidget *statusbar = NULL;

extern GtkWidget *mainwindow;

enum
  {
    COL_NAME = 0,
    COL_NODE,
    COL_WEIGHT,
    COL_CONTINUATION,
    NUM_COLS
  };

static void clear_list(GtkWidget * list1)
{
  GtkTreeViewColumn *col;

  while((col = gtk_tree_view_get_column(GTK_TREE_VIEW(list1), 0)))
    gtk_tree_view_remove_column(GTK_TREE_VIEW(list1), col);
}

static void populate_sublist(GtkWidget * list1, hwNode * root, hwNode *current=NULL)
{
  GtkListStore *list_store = NULL;
  GtkTreeViewColumn   *col = NULL;
  GtkCellRenderer     *renderer = NULL;
  GtkTreeIter iter;
  GtkTreeIter current_iter;

  clear_list(list1);

  if(!root) return;

  list_store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT, G_TYPE_STRING);

  for(unsigned i = 0; i<root->countChildren(); i++)
  {
    string text;

    gtk_list_store_append(list_store, &iter);

    if(root->getChild(i) == current) current_iter = iter;

    text = root->getChild(i)->getDescription();
    if(text == "")
      text = root->getChild(i)->getProduct();
    if(text == "")
      text = root->getChild(i)->getId();

    gtk_list_store_set (list_store, &iter,
                      COL_NAME, text.c_str(),
                      COL_NODE, root->getChild(i),
                      COL_WEIGHT, (root->getChild(i)->countChildren()>0)?PANGO_WEIGHT_BOLD:PANGO_WEIGHT_NORMAL,
                      COL_CONTINUATION, (root->getChild(i)->countChildren()>0)?">":"",
                      -1);
  }

  col = gtk_tree_view_column_new();
  gtk_tree_view_append_column(GTK_TREE_VIEW(list1), col);

  renderer = gtk_cell_renderer_text_new();

  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_NAME);
  gtk_tree_view_column_add_attribute(col, renderer, "weight", COL_WEIGHT);

  col = gtk_tree_view_column_new();
  gtk_tree_view_append_column(GTK_TREE_VIEW(list1), col);
  renderer = gtk_cell_renderer_text_new();
  g_object_set(renderer, "xalign", (gfloat)1, NULL);
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_CONTINUATION);
  gtk_tree_view_column_add_attribute(col, renderer, "weight", COL_WEIGHT);

  gtk_tree_view_set_model(GTK_TREE_VIEW(list1), GTK_TREE_MODEL(list_store));
  g_object_unref(list_store);

  gtk_tree_selection_set_mode(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(list1))), GTK_SELECTION_BROWSE);
  if(current)
    gtk_tree_selection_select_iter(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(list1))), &current_iter);
}

static void display(GtkWidget * mainwindow)
{
  GtkWidget * description = lookup_widget(mainwindow, "description");

  if(!displayed)
    gtk_label_set_text(GTK_LABEL(description), "<i>Please select a node to display.</i>");
  else
  {
    string markup = printmarkup(*displayed);
    string hwpath = printhwpath(*displayed, container);

    if(hwpath!="") markup += "\n<b>path:</b> <tt>" + hwpath + "</tt>";

    gtk_label_set_text(GTK_LABEL(description), markup.c_str());
  }
  gtk_label_set_use_markup (GTK_LABEL(description), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL(description), TRUE);
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
  hwNode computer("computer", hw::system);
  static bool lock = false;
  GtkWidget * description = lookup_widget(mainwindow, "description");
  GtkWidget * go_up_button = lookup_widget(mainwindow, "upbutton");

  if(lock) return;

  lock = true;

  GtkWidget * list1 = lookup_widget(mainwindow, "treeview1");
  GtkWidget * list2 = lookup_widget(mainwindow, "treeview2");
  GtkWidget * list3 = lookup_widget(mainwindow, "treeview3");
  statusbar = lookup_widget(mainwindow, "statusbar");

  populate_sublist(list1, NULL);
  populate_sublist(list2, NULL);
  populate_sublist(list3, NULL);
  displayed = NULL;
  gtk_label_set_text(GTK_LABEL(description), "<i>scanning...</i>");
  gtk_label_set_use_markup (GTK_LABEL(description), TRUE);

  container = hwNode("container", hw::generic);
  status("Scanning...");
  scan_system(computer);
  status(NULL);
  container.addChild(computer);

  gtk_widget_set_sensitive(go_up_button, 0);

  selected1 = NULL;
  selected2 = NULL;
  selected3 = NULL;

  populate_sublist(list1, &container);
  populate_sublist(list2, NULL);
  populate_sublist(list3, NULL);
  display(mainwindow);

  lock = false;
}

void change_selection(unsigned list, GtkTreeView *treeview)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GtkWidget * list2 = lookup_widget(mainwindow, "treeview2");
  GtkWidget * list3 = lookup_widget(mainwindow, "treeview3");

  model = gtk_tree_view_get_model(treeview);

  displayed = NULL;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  if (gtk_tree_selection_get_selected(selection, &model, &iter))
    gtk_tree_model_get (model, &iter, COL_NODE, &displayed, -1);

  if(list<2) populate_sublist(list2, NULL);
  if(list<3) populate_sublist(list3, NULL);

  display(mainwindow);
}

static hwNode * find_parent(hwNode * n, hwNode *sub)
{
  if(!n) return NULL;

  if(n == sub) return n;

  for(unsigned i=0; i<sub->countChildren(); i++)
  {
    if(sub->getChild(i) == n) return sub;

    hwNode *r = find_parent(n, sub->getChild(i));
    if(r) return r;
  }

  return NULL;
}

void browse(unsigned list, GtkTreeView *treeview)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  hwNode *n = NULL;
  GtkWidget * list1 = lookup_widget(mainwindow, "treeview1");
  GtkWidget * list2 = lookup_widget(mainwindow, "treeview2");
  GtkWidget * list3 = lookup_widget(mainwindow, "treeview3");
  GtkWidget * go_up_button = lookup_widget(mainwindow, "upbutton");

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  if (gtk_tree_selection_get_selected(selection, &model, &iter))
    gtk_tree_model_get (model, &iter, COL_NODE, &n, -1);

  if(n)
  switch(list)
  {
    case 1:
      //if(n!=selected1)
      {
        selected1 = n;
        selected2 = NULL;
        selected3 = NULL;
        populate_sublist(list2, selected1, selected2);
        populate_sublist(list3, selected2, selected3);
      }
      break;
    case 2:
      //if(n!=selected2)
      {
        selected2 = n;
        selected3 = NULL;
        populate_sublist(list3, selected2, selected3);
      }
      break;
    case 3:
      //if(n!=selected3)
      {
        selected3 = n;
        if(n->countChildren()>0)
        {
          hwNode *oldselected1 = selected1;
          selected1 = selected2;
          selected2 = n;
          selected3 = NULL;
          populate_sublist(list1, oldselected1, selected1);
          populate_sublist(list2, selected1, selected2);
          populate_sublist(list3, selected2, selected3);
        }
      }
      break;
  }

  if(selected1 && (find_parent(selected1, &container)!= &container))
    gtk_widget_set_sensitive(go_up_button, 1);
  else
    gtk_widget_set_sensitive(go_up_button, 0);

  (void) &::id;                 // avoid warning "id defined but not used"
}

void go_back(GtkWidget *mainwindow)
{
  GtkWidget * list1 = lookup_widget(mainwindow, "treeview1");
  GtkWidget * list2 = lookup_widget(mainwindow, "treeview2");
  GtkWidget * list3 = lookup_widget(mainwindow, "treeview3");
  GtkWidget * go_up_button = lookup_widget(mainwindow, "upbutton");

  if(selected1 && (find_parent(selected1, &container)!= &container))
  {
    selected3 = selected2;
    selected2 = selected1;
    selected1 = find_parent(selected1, &container);
    if(selected1 == &container) selected1 = container.getChild(0);
    populate_sublist(list1, find_parent(selected1, &container), selected1);
    populate_sublist(list2, selected1, selected2);
    populate_sublist(list3, selected2, selected3);

    if(find_parent(displayed, &container)!= &container)
      displayed = find_parent(displayed, &container);
  }

  if(selected1 && (find_parent(selected1, &container)!= &container))
    gtk_widget_set_sensitive(go_up_button, 1);
  else
    gtk_widget_set_sensitive(go_up_button, 0);

  display(mainwindow);
}
