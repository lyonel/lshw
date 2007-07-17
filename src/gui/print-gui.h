#ifndef _PRINTGUI_H_
#define _PRINTGUI_H_

#include "hw.h"
#include <gtk/gtk.h>

void printmarkup(const hwNode & node, GtkTextView *textview, const string & hwpath);

string gethwpath(hwNode & node, hwNode & base);
#endif
