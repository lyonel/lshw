/*
 * print.cc
 *
 *
 */

#include "print.h"
#include "options.h"
#include "version.h"
#include "osutils.h"
#include "stock.h"
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <stdio.h>

static char *id = "@(#) $Id$";

static string itos(unsigned long long value)
{
  ostringstream out;

  out << value;

  return out.str();
}

static string decimalkilos(unsigned long long value)
{
  const char *prefixes = "KMGTPEZY";
  unsigned int i = 0;
  ostringstream out;

  while ((i <= strlen(prefixes)) && ((value > 10000) || (value % 1000 == 0)))
  {
    value = value / 1000;
    i++;
  }

  out << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    out << prefixes[i - 1];

  return out.str();
}

static string kilobytes(unsigned long long value)
{
  const char *prefixes = "KMGTPEZY";
  unsigned int i = 0;
  ostringstream out;

  while ((i <= strlen(prefixes)) && ((value > 10240) || (value % 1024 == 0)))
  {
    value = value >> 10;
    i++;
  }

  out << value;
  if ((i > 0) && (i <= strlen(prefixes)))
    out << prefixes[i - 1];
  out << "B";

  return out.str();
}

static void printattr(const string & name, const string & value, GtkTextBuffer *buffer, GtkTextIter &iter)
{
  if(value == "") return;

  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, name.c_str(), -1, "bold", NULL);
  gtk_text_buffer_insert (buffer, &iter, ": ", -1);
  gtk_text_buffer_insert (buffer, &iter, value.c_str(), -1);
  gtk_text_buffer_insert (buffer, &iter, "\n", -1);
}

static void printsize(long long value, const hwNode & node, const string & name, GtkTextBuffer *buffer, GtkTextIter &iter)
{
  if (value > 0)
    {
      switch (node.getClass())
      {
      case hw::display:
      case hw::memory:
      case hw::address:
      case hw::storage:
      case hw::disk:
	printattr(name,kilobytes(value), buffer, iter);
	break;

      case hw::processor:
      case hw::bus:
      case hw::system:
	printattr(name,decimalkilos(value)+"Hz", buffer, iter);
	break;

      case hw::network:
	printattr(name,decimalkilos(value)+"B/s", buffer, iter);
	break;

      case hw::power:
	printattr(name,itos(value)+"mWh", buffer, iter);
	break;

      default:
	printattr(name,itos(value), buffer, iter);
      }
    }
}

static  void inserticon(const string & icon, const string & comment, GtkTextBuffer *buffer, GtkTextIter &iter, GtkTextView * textview)
{
  GdkPixbuf *pixbuf;

  pixbuf = gtk_widget_render_icon(GTK_WIDGET(textview),
                                  icon.c_str(),
                                  gtk_icon_size_from_name(LSHW_ICON_SIZE_LOGO), /* size */
                                  NULL);
  gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf);
  gtk_text_buffer_insert(buffer, &iter, comment.c_str(), -1);
  //gtk_text_buffer_insert(buffer, &iter, "\n", -1);
}

void printmarkup(const hwNode & node, GtkTextView *textview, const string & hwpath)
{
  vector < string > config;
  vector < string > resources;
  ostringstream out;
  GtkTextIter iter;
  GtkTextBuffer *buffer;

  buffer = gtk_text_view_get_buffer(textview);

  gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  resources = node.getResources(":");

  if(node.getDescription()!="")
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, node.getDescription().c_str(), -1, "heading", NULL);
  else
  {
    if(node.getProduct()!="")
      gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, node.getProduct().c_str(), -1, "heading", NULL);
    else
      gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, node.getId().c_str(), -1, "heading", NULL);
  }
  if(!node.claimed() || node.disabled())
    out << "<span color=\"gray\">";
  if(node.disabled())
    out << " DISABLED";
  if(!node.claimed())
    out << " UNCLAIMED";
  if(!node.claimed() || node.disabled())
    out << "</span>";

  gtk_text_buffer_insert (buffer, &iter, "\n", -1);
  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, hwpath.c_str(), -1, "monospace", NULL);

  gtk_text_buffer_insert (buffer, &iter, "\n", -1);

  if(node.getHint("icon").defined())
    inserticon(string("lshw-") + node.getHint("icon").asString(), "", buffer, iter, textview);

  if(node.getHint("bus.icon").defined())
    inserticon(string("lshw-") + node.getHint("bus.icon").asString(), "", buffer, iter, textview);

  gtk_text_buffer_insert (buffer, &iter, "\n\n", -1);

  //out << printattr("description", node.getDescription());
  printattr("product", node.getProduct(), buffer, iter);
  printattr("vendor", node.getVendor(), buffer, iter);
  printattr("bus info", node.getBusInfo(), buffer, iter);
  if (node.getLogicalName() != "")
  {
    vector<string> logicalnames = node.getLogicalNames();

    for(unsigned int i = 0; i<logicalnames.size(); i++)
    	printattr("logical name", logicalnames[i], buffer, iter);
  }

  printattr("version", node.getVersion(), buffer, iter);
  printattr("serial", node.getSerial(), buffer, iter);
  printattr("slot", node.getSlot(), buffer, iter);

  if(node.getSize() > 0)
    printsize(node.getSize(), node, "size", buffer, iter);
  if(node.getCapacity() > 0)
    printsize(node.getCapacity(), node, "capacity", buffer, iter);

  if(node.getWidth() > 0)
    printattr("width",itos(node.getWidth())+" bits", buffer, iter);

  if(node.getClock() > 0)
    printattr("clock", decimalkilos(node.getClock())+string("Hz") + ((node.getClass() == hw::memory)?(string(" (")+itos((long long)(1.0e9 / node.getClock())) + string("ns)")):string("")), buffer, iter);

  config.clear();
  splitlines(node.getCapabilities(), config, ' ');

  if (config.size() > 0)
  {
     gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "capabilities", -1, "bold", NULL);
     gtk_text_buffer_insert (buffer, &iter, ":", -1);
     for(unsigned j=0; j<config.size(); j++)
     {
       gtk_text_buffer_insert (buffer, &iter, "\n\t", -1);
       if(node.getCapabilityDescription(config[j]) != "")
       {
         gtk_text_buffer_insert (buffer, &iter, node.getCapabilityDescription(config[j]).c_str(), -1);
       }
       else
         gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, config[j].c_str(), -1, "italic", NULL);
       if(j<config.size()-1)
         gtk_text_buffer_insert (buffer, &iter, ",", -1);
     }
  }

  gtk_text_buffer_insert (buffer, &iter, "\n", -1);

  config.clear();
  config = node.getConfigValues(": ");

  if (config.size() > 0)
  {
     gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "configuration", -1, "bold", NULL);
     gtk_text_buffer_insert (buffer, &iter, ":", -1);
     for(unsigned j=0; j<config.size(); j++)
     {
       gtk_text_buffer_insert (buffer, &iter, "\n\t", -1);
       gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, config[j].c_str(), -1, "italic", NULL);
     }
  }

  gtk_text_buffer_insert (buffer, &iter, "\n", -1);

  if(!node.claimed())
    inserticon(LSHW_STOCK_DISABLED, "this device hasn't been claimed\n", buffer, iter, textview);

  if(!node.enabled())
    inserticon(LSHW_STOCK_DISABLED, "this device has been disabled\n", buffer, iter, textview);


  (void) &id;			// avoid "id defined but not used" warning
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

string printhwpath( hwNode & n, hwNode & base)
{
  ostringstream out;
  hwNode *parent = find_parent(&n, &base);

  if(parent && (parent != &base))
    out << printhwpath(*parent, base) << "/" << n.getPhysId();

  return out.str();
}

