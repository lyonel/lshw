/*
 * print.cc
 *
 *
 */

#include "print.h"
#include "options.h"
#include "version.h"
#include "osutils.h"
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
  gtk_text_buffer_insert (buffer, &iter, escape(value).c_str(), -1);
  gtk_text_buffer_insert (buffer, &iter, "\n", -1);
}

void printmarkup(const hwNode & node, GtkTextBuffer *buffer)
{
  vector < string > config;
  vector < string > resources;
  ostringstream out;
  GtkTextIter iter;

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
  gtk_text_buffer_insert (buffer, &iter, "\n\n\n", -1);

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

  if (node.getSize() > 0)
    {
      switch (node.getClass())
      {
      case hw::display:
      case hw::memory:
      case hw::address:
      case hw::storage:
      case hw::disk:
	printattr("size",kilobytes(node.getSize()), buffer, iter);
	break;

      case hw::processor:
      case hw::bus:
      case hw::system:
	printattr("size",decimalkilos(node.getSize())+"Hz", buffer, iter);
	break;

      case hw::network:
	printattr("size",decimalkilos(node.getSize())+"/s", buffer, iter);
	break;

      case hw::power:
	printattr("size",itos(node.getSize())+"mWh", buffer, iter);
	break;

      default:
	printattr("size",itos(node.getSize()), buffer, iter);
      }
    }

  if (node.getCapacity() > 0)
    {
      switch (node.getClass())
      {
      case hw::display:
      case hw::memory:
      case hw::address:
      case hw::storage:
      case hw::disk:
	printattr("capacity",kilobytes(node.getCapacity()), buffer, iter);
	break;

      case hw::processor:
      case hw::bus:
      case hw::system:
	printattr("capacity",decimalkilos(node.getCapacity())+"Hz", buffer, iter);
	break;

      case hw::network:
	printattr("capacity",decimalkilos(node.getCapacity())+"/s", buffer, iter);
	break;

      case hw::power:
	printattr("capacity",itos(node.getCapacity())+"mWh", buffer, iter);
	break;

      default:
	printattr("capacity",itos(node.getCapacity()), buffer, iter);
      }
    }

    if (node.getClass() == hw::address)
    {
      if (node.getSize() == 0)
	out << "<b>address:</b> " << hex << setfill('0') << setw(8) << node.
	  getStart() << dec;
      else
	out << "<b>range:</b> " << hex << setfill('0') << setw(8) << node.
	  getStart() << " - " << hex << setfill('0') << setw(8) << node.
	  getStart() + node.getSize() - 1 << dec;
      out << endl;
    }

    if (node.getWidth() > 0)
      printattr("width",itos(node.getWidth())+" bits", buffer, iter);

    if (node.getClock() > 0)
    {
      printattr("clock", decimalkilos(node.getClock())+string("Hz") + ((node.getClass() == hw::memory)?(string(" (")+itos((long long)(1.0e9 / node.getClock())) + string("ns)")):string("")), buffer, iter);
    }

  config.clear();

  splitlines(node.getCapabilities(), config, ' ');
  if (config.size() > 0)
  {
     out << "<b>capabilities:</b>";
     for(unsigned j=0; j<config.size(); j++)
     {
       out << "\n\t";
       if(node.getCapabilityDescription(config[j]) != "")
       {
         out << escape(node.getCapabilityDescription(config[j]));
       }
       else
         out << "<i>" << config[j] << "</i>";
       if(j<config.size()-1) out << ",";
     }
     out << endl;
  }

  config.clear();
  config = node.getConfigValues(": <i>");

    if (config.size() > 0)
    {
      out << endl << "<b>configuration:</b>" << endl;
      for (unsigned int i = 0; i < config.size(); i++)
	out << "\t" << config[i] << "</i>" << endl;
      out << endl;
    }

#if 0
    if (resources.size() > 0)
    {
      tab(level + 1, false);
      if (html)
	cout << "<tr><td>";
      cout << "resources:";
      if (html)
	cout << "</td><td><table summary=\"resources of " << node.
	  getId() << "\">";
      for (unsigned int i = 0; i < resources.size(); i++)
      {
	if (html)
	  cout << "<tr><td>";
	cout << " " << resources[i];
	if (html)
	  cout << "</td></tr>";
      }
      if (html)
	cout << "</table></td></tr>";
      cout << endl;
    }

  }				// if visible

#endif
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

