#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C"
{
#endif

  void refresh(GtkWidget *);

  void change_selection(unsigned list, GtkTreeView *treeview);

  void browse(unsigned list, GtkTreeView *treeview);

  void go_back(GtkWidget *);

  void save_as(GtkWidget *);

#ifdef __cplusplus
};
#endif
#endif
