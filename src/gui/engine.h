#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

void refresh(GtkWidget *);

void activate(GtkTreeView *treeview,
              GtkTreePath *path,
              GtkTreeViewColumn *column,
              gpointer         user_data);


#ifdef __cplusplus
};
#endif

#endif
