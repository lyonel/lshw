#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

void refresh(GtkWidget *);

void change_selection(unsigned list, GtkTreeView *treeview);

void browse(unsigned list, GtkTreeView *treeview);


#ifdef __cplusplus
};
#endif

#endif
