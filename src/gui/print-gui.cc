/*
 * print.cc
 *
 *
 */

#include "print-gui.h"
#include "options.h"
#include "config.h"
#include "version.h"
#include "osutils.h"
#include "stock.h"
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <stdio.h>

static const char *id = "@(#) $Id$";

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
      case hw::disk:
        printattr(name,kilobytes(value)+" ("+decimalkilos(value)+"B)", buffer, iter);
        break;
      case hw::display:
      case hw::memory:
      case hw::address:
      case hw::storage:
      case hw::volume:
        printattr(name,kilobytes(value), buffer, iter);
        break;

      case hw::processor:
      case hw::bus:
      case hw::system:
        printattr(name,decimalkilos(value)+"Hz", buffer, iter);
        break;

      case hw::network:
        printattr(name,decimalkilos(value)+"bit/s", buffer, iter);
        break;

      case hw::power:
        printattr(name,tostring(value)+"mWh", buffer, iter);
        break;

      default:
        printattr(name,tostring(value), buffer, iter);
    }
  }
}


static  void inserticon(const string & icon, const string & comment, GtkTextBuffer *buffer, GtkTextIter &iter, GtkTextView * textview)
{
  GdkPixbuf *pixbuf;
  GtkTextTag *tag;

  pixbuf = gtk_widget_render_icon(GTK_WIDGET(textview),
    icon.c_str(),
    gtk_icon_size_from_name(LSHW_ICON_SIZE_LOGO), /* size */
    NULL);
  if(!GDK_IS_PIXBUF(pixbuf))
    return;

  tag = gtk_text_buffer_create_tag (buffer, NULL, 
				    "rise", 2*LSHW_DEFAULT_ICON_SIZE*PANGO_SCALE/5,
				    NULL);
  gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf);
  gtk_text_buffer_insert_with_tags(buffer, &iter, comment.c_str(), -1, tag, NULL);
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
    out << _(" DISABLED");
  if(!node.claimed())
    out << _(" UNCLAIMED");
  if(!node.claimed() || node.disabled())
    out << "</span>";

  gtk_text_buffer_insert (buffer, &iter, "\n", -1);
  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, hwpath.c_str(), -1, "monospace", NULL);

  gtk_text_buffer_insert (buffer, &iter, "\n", -1);

  if(node.getHint("icon").defined())
    inserticon(string("lshw-") + node.getHint("icon").asString(), "", buffer, iter, textview);

  if(node.getHint("bus.icon").defined())
    inserticon(string("lshw-") + node.getHint("bus.icon").asString(), "", buffer, iter, textview);

  if(node.getHint("logo").defined())
    inserticon(string("lshw-") + node.getHint("logo").asString(), "", buffer, iter, textview);

  gtk_text_buffer_insert (buffer, &iter, "\n\n", -1);

//out << printattr("description", node.getDescription());
  printattr(_("product"), node.getProduct(), buffer, iter);
  printattr(_("vendor"), node.getVendor(), buffer, iter);
  printattr(_("bus info"), node.getBusInfo(), buffer, iter);
  if (node.getLogicalName() != "")
  {
    vector<string> logicalnames = node.getLogicalNames();

    for(unsigned int i = 0; i<logicalnames.size(); i++)
      printattr(_("logical name"), logicalnames[i], buffer, iter);
  }

  printattr(_("version"), node.getVersion(), buffer, iter);
  printattr(_("serial"), node.getSerial(), buffer, iter);
  printattr(_("slot"), node.getSlot(), buffer, iter);

  if(node.getSize() > 0)
    printsize(node.getSize(), node, _("size"), buffer, iter);
  if(node.getCapacity() > 0)
    printsize(node.getCapacity(), node, _("capacity"), buffer, iter);

  if(node.getWidth() > 0)
    printattr(_("width"),tostring(node.getWidth())+" bits", buffer, iter);

  if(node.getClock() > 0)
    printattr(_("clock"), decimalkilos(node.getClock())+string("Hz") + ((node.getClass() == hw::memory)?(string(" (")+tostring((long long)(1.0e9 / node.getClock())) + string("ns)")):string("")), buffer, iter);

  config.clear();
  splitlines(node.getCapabilities(), config, ' ');

  if (config.size() > 0)
  {
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("capabilities"), -1, "bold", NULL);
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
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("configuration"), -1, "bold", NULL);
    gtk_text_buffer_insert (buffer, &iter, ":", -1);
    for(unsigned j=0; j<config.size(); j++)
    {
      gtk_text_buffer_insert (buffer, &iter, "\n\t", -1);
      gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, config[j].c_str(), -1, "italic", NULL);
    }
  }

  gtk_text_buffer_insert (buffer, &iter, "\n", -1);

  config.clear();
  config = node.getResources(": ");

  if (config.size() > 0)
  {
    gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("resources"), -1, "bold", NULL);
    gtk_text_buffer_insert (buffer, &iter, ":", -1);
    for(unsigned j=0; j<config.size(); j++)
    {
      gtk_text_buffer_insert (buffer, &iter, "\n\t", -1);
      gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, config[j].c_str(), -1, "italic", NULL);
    }
  }

  gtk_text_buffer_insert (buffer, &iter, "\n", -1);

  if(!node.claimed())
    inserticon(LSHW_STOCK_DISABLED, _("this device hasn't been claimed\n"), buffer, iter, textview);

  if(!node.enabled())
    inserticon(LSHW_STOCK_DISABLED, _("this device has been disabled\n"), buffer, iter, textview);

  (void) &id;                                     // avoid "id defined but not used" warning
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


string gethwpath( hwNode & n, hwNode & base)
{
  ostringstream out;
  hwNode *parent = find_parent(&n, &base);

  if(parent && (parent != &base))
    out << gethwpath(*parent, base) << "/" << n.getPhysId();

  return out.str();
}
