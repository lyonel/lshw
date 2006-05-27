#ifndef _PRINT_H_
#define _PRINT_H_

#include "hw.h"
#include <gtk/gtk.h>

void printmarkup(const hwNode & node, GtkTextView *textview, const string & hwpath);

string printhwpath(hwNode & node, hwNode & base);
#endif
