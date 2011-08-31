#include "engine.h"
#include "hw.h"
#include "main.h"
#include "print-gui.h"
#include "print.h"
#include "osutils.h"
#include "options.h"

#include <iostream>
#include <fstream>
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

static const char *id = "@(#) $Id$";

extern "C"
{
#include "support.h"
};

#define AUTOMATIC "automatic file format"
#define LSHW_XML "lshw XML format (.lshw, .xml)"
#define PLAIN_TEXT "plain text document (.text, .txt)"
#define JSON "JavaScript Object Notation document (.json)"
#define HTML "HTML document (.html, .htm)"

#define YIELD()  while(gtk_events_pending()) gtk_main_iteration()

static hwNode container("container", hw::generic);
static hwNode *displayed = NULL;
static hwNode *selected1 = NULL;
static hwNode *selected2 = NULL;
static hwNode *selected3 = NULL;

extern GtkWidget *mainwindow;
extern GtkWidget *list1, *list2, *list3;
extern GtkWidget *description;
extern GtkWidget *go_up_button;
extern GtkWidget *save_button;
extern GtkWidget *statusbar;

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
      COL_CONTINUATION, (root->getChild(i)->countChildren()>0)?"\342\226\270":"",
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


static void create_tags (GtkTextBuffer *buffer)
{
  static bool initialized = false;

  if(initialized) return;

  initialized = true;

  gtk_text_buffer_create_tag (buffer, "heading",
    "weight", PANGO_WEIGHT_BOLD,
    "size", 15 * PANGO_SCALE,
    NULL);

  gtk_text_buffer_create_tag (buffer, "italic",
    "style", PANGO_STYLE_ITALIC, NULL);

  gtk_text_buffer_create_tag (buffer, "bold",
    "weight", PANGO_WEIGHT_BOLD, NULL);

  gtk_text_buffer_create_tag (buffer, "big",
/* points times the PANGO_SCALE factor */
    "size", 20 * PANGO_SCALE, NULL);

  gtk_text_buffer_create_tag (buffer, "xx-small",
    "scale", PANGO_SCALE_XX_SMALL, NULL);

  gtk_text_buffer_create_tag (buffer, "x-large",
    "scale", PANGO_SCALE_X_LARGE, NULL);

  gtk_text_buffer_create_tag (buffer, "monospace",
    "family", "monospace", NULL);

  gtk_text_buffer_create_tag (buffer, "blue_foreground",
    "foreground", "blue", NULL);

  gtk_text_buffer_create_tag (buffer, "red_background",
    "background", "red", NULL);

  gtk_text_buffer_create_tag (buffer, "big_gap_before_line",
    "pixels_above_lines", 30, NULL);

  gtk_text_buffer_create_tag (buffer, "big_gap_after_line",
    "pixels_below_lines", 30, NULL);

  gtk_text_buffer_create_tag (buffer, "double_spaced_line",
    "pixels_inside_wrap", 10, NULL);

  gtk_text_buffer_create_tag (buffer, "not_editable",
    "editable", FALSE, NULL);

  gtk_text_buffer_create_tag (buffer, "word_wrap",
    "wrap_mode", GTK_WRAP_WORD, NULL);

  gtk_text_buffer_create_tag (buffer, "char_wrap",
    "wrap_mode", GTK_WRAP_CHAR, NULL);

  gtk_text_buffer_create_tag (buffer, "no_wrap",
    "wrap_mode", GTK_WRAP_NONE, NULL);

  gtk_text_buffer_create_tag (buffer, "center",
    "justification", GTK_JUSTIFY_CENTER, NULL);

  gtk_text_buffer_create_tag (buffer, "right_justify",
    "justification", GTK_JUSTIFY_RIGHT, NULL);

  gtk_text_buffer_create_tag (buffer, "wide_margins",
    "left_margin", 50, "right_margin", 50,
    NULL);

  gtk_text_buffer_create_tag (buffer, "strikethrough",
    "strikethrough", TRUE, NULL);

  gtk_text_buffer_create_tag (buffer, "underline",
    "underline", PANGO_UNDERLINE_SINGLE, NULL);

  gtk_text_buffer_create_tag (buffer, "double_underline",
    "underline", PANGO_UNDERLINE_DOUBLE, NULL);

  gtk_text_buffer_create_tag (buffer, "superscript",
    "rise", 10 * PANGO_SCALE,                     /* 10 pixels */
    "size", 8 * PANGO_SCALE,                      /* 8 points */
    NULL);

  gtk_text_buffer_create_tag (buffer, "subscript",
    "rise", -10 * PANGO_SCALE,                    /* 10 pixels */
    "size", 8 * PANGO_SCALE,                      /* 8 points */
    NULL);
}


static void display(GtkWidget * mainwindow)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (description));

  gtk_text_buffer_set_text(buffer, "", -1);

  if(!displayed)
    gtk_text_buffer_set_text(buffer, "Please select a node to display.", -1);
  else
  {
    create_tags(buffer);

    string hwpath = gethwpath(*displayed, container);
    printmarkup(*displayed, GTK_TEXT_VIEW(description), hwpath);
  }
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
  //GtkWidget * menu = lookup_widget(mainwindow, "menu");
  //GtkWidget * save_menuitem = lookup_widget(menu, "save");

  if(lock) return;

  lock = true;
  gtk_widget_set_sensitive(save_button, FALSE);
  //gtk_widget_set_sensitive(save_menuitem, FALSE);

  populate_sublist(list1, NULL);
  populate_sublist(list2, NULL);
  populate_sublist(list3, NULL);
  displayed = NULL;
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(description)), "scanning...", -1);

  disable("output:sanitize");
  container = hwNode("container", hw::generic);
  status("Scanning...");
  scan_system(computer);
  status(NULL);
  displayed = container.addChild(computer);

  gtk_widget_set_sensitive(go_up_button, FALSE);
  gtk_widget_set_sensitive(save_button, TRUE);
  //gtk_widget_set_sensitive(save_menuitem, TRUE);

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

  (void) &::id;                                   // avoid warning "id defined but not used"
}


void go_back(GtkWidget *mainwindow)
{
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

static const char *guess_format(char *s)
{
  char *dot = strrchr(s, '.');

  if(!dot)
    return LSHW_XML;

  if(!strcasecmp(".html", dot) || !strcasecmp(".htm", dot))
    return HTML;

  if(!strcasecmp(".text", dot) || !strcasecmp(".txt", dot))
    return PLAIN_TEXT;

  if(!strcasecmp(".json", dot))
    return JSON;

  return LSHW_XML;
}

static char *fix_filename(char *s, const char *extension)
{
  char *dot = strrchr(s, '.');

  if(dot)
    return s;

  s = (char*)realloc(s, strlen(s) + 1 + strlen(extension) + 1);
  strcat(s, ".");
  strcat(s, extension);

  return s;
}

static void redirect_cout(std::ofstream &out, bool enable = true)
{
  static std::streambuf* old_cout;
  
  if(enable)
  {
    old_cout = cout.rdbuf();
    cout.rdbuf(out.rdbuf());
  }
  else
    cout.rdbuf(old_cout);
}

void save_as(GtkWidget *mainwindow)
{
  struct utsname buf;
  GtkWidget *dialog = NULL;
  GtkWidget *sanitize = NULL;
  GtkFileFilter *filter = NULL;
  bool proceed = true;
  hwNode *computer = container.getChild(0);

  if(!computer)		// nothing to save
    return;

  dialog = gtk_file_chooser_dialog_new ("Save hardware configuration",
				      GTK_WINDOW(mainwindow),
				      GTK_FILE_CHOOSER_ACTION_SAVE,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				      NULL);
  //gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  sanitize = gtk_check_button_new_with_label("Anonymize output");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sanitize), enabled("output:sanitize")?TRUE:FALSE);
  gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), sanitize);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name(filter, AUTOMATIC);
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name(filter, LSHW_XML);
  gtk_file_filter_add_pattern(filter, "*.lshw");
  gtk_file_filter_add_pattern(filter, "*.xml");
  gtk_file_filter_add_mime_type(filter, "text/xml");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name(filter, HTML);
  gtk_file_filter_add_pattern(filter, "*.html");
  gtk_file_filter_add_pattern(filter, "*.htm");
  gtk_file_filter_add_mime_type(filter, "text/html");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name(filter, PLAIN_TEXT);
  gtk_file_filter_add_pattern(filter, "*.text");
  gtk_file_filter_add_pattern(filter, "*.txt");
  gtk_file_filter_add_mime_type(filter, "text/plain");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name(filter, JSON);
  gtk_file_filter_add_pattern(filter, "*.json");
  gtk_file_filter_add_mime_type(filter, "application/json");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  if(uname(&buf)==0)
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), buf.nodename);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sanitize)))
      enable("output:sanitize");
    else
      disable("output:sanitize");

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
    if(filename && filter)
    {
      const gchar *filtername = gtk_file_filter_get_name(filter);

      if(strcmp(filtername, AUTOMATIC)==0)
        filtername = guess_format(filename);

      if(!exists(filename))		// creating a new file
      {
        if(strcmp(filtername, LSHW_XML)==0)
          filename = fix_filename(filename, "lshw");
        else
        if(strcmp(filtername, HTML)==0)
          filename = fix_filename(filename, "html");
        else
        if(strcmp(filtername, PLAIN_TEXT)==0)
          filename = fix_filename(filename, "txt");
        else
        if(strcmp(filtername, JSON)==0)
          filename = fix_filename(filename, "json");
      }

      if(exists(filename))		// existing file
      {
        char * buffer1 = g_strdup(filename);
        char * buffer2 = g_strdup(filename);

        GtkWidget *dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(mainwindow),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_WARNING,
                                  GTK_BUTTONS_NONE,
                                  "A file named <i><tt>%s</tt></i> already exists in folder <tt>%s</tt>.\n\nDo you want to overwrite it?",
                                  basename(buffer1), dirname(buffer2));
        gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
				  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				  "Overwrite", GTK_RESPONSE_ACCEPT,
                                  NULL);
        proceed = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT);
        gtk_widget_destroy (dialog);
        g_free(buffer1);
        g_free(buffer2);
      }

      if(proceed)
      {
        if(strcmp(filtername, LSHW_XML)==0)
        {
          std::ofstream out(filename);
          redirect_cout(out);
          cout << computer->asXML();
          redirect_cout(out, false);
        }
        else
        if(strcmp(filtername, HTML)==0)
        {
          std::ofstream out(filename);
          redirect_cout(out);
          print(*computer, true);
          redirect_cout(out, false);
        }
        else
        if(strcmp(filtername, PLAIN_TEXT)==0)
        {
          std::ofstream out(filename);
          redirect_cout(out);
          print(*computer, false);
          redirect_cout(out, false);
        }
	else
        if(strcmp(filtername, JSON)==0)
        {
          std::ofstream out(filename);
          redirect_cout(out);
          cout << computer->asJSON() << endl;
          redirect_cout(out, false);
        }
      }
      g_free (filename);
    }
  }

  gtk_widget_destroy (dialog);
}
