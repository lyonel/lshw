#include "hw.h"
#include <vector>
#include <stdio.h>
#include <ctype.h>

using namespace hw;

struct hwNode_i
{
  hwClass deviceclass;
  string id, vendor, product, version, serial, slot;
  bool enabled;
  unsigned long long size;
  unsigned long long capacity;
  unsigned long long clock;
    vector < hwNode > children;
};

static string strip(const string & s)
{
  string result = s;

  while ((result.length() > 0) && (result[0] <= ' '))
    result.erase(0, 1);
  while ((result.length() > 0) && (result[result.length() - 1] <= ' '))
    result.erase(result.length() - 1, 1);
  for (int i = 0; i < result.length(); i++)
    if (result[0] <= ' ')
    {
      result.erase(i, 1);
      i--;
    }

  return result;
}

static string cleanupId(const string & id)
{
  string result = strip(id);

  for (int i = 0; i < result.length(); i++)
  {
    result[i] = tolower(result[i]);
    if (!strchr("0123456789abcdefghijklmnopqrstuvwxyz_.:-", result[i]))
      result[i] = '_';
  }

  return result;
}

hwNode::hwNode(const string & id,
	       hwClass c,
	       const string & vendor,
	       const string & product, const string & version)
{
  This = new hwNode_i;

  if (!This)
    return;

  This->deviceclass = c;
  This->id = cleanupId(id);
  This->vendor = strip(vendor);
  This->product = strip(product);
  This->version = strip(version);
  This->size = 0;
  This->capacity = 0;
  This->clock = 0;
  This->enabled = true;
}

hwNode::hwNode(const hwNode & o)
{
  This = new hwNode_i;

  if (o.This)
    (*This) = (*o.This);
}

hwNode::~hwNode()
{
  if (This)
    delete This;
}

hwNode & hwNode::operator = (const hwNode & o)
{
  This = new hwNode_i;

  if (o.This && This)
    (*This) = (*o.This);

  return *this;
}

hwClass hwNode::getClass() const
{
  if (This)
    return This->deviceclass;
  else
    return hw::generic;
}

void hwNode::setClass(hwClass c)
{
  if (!This)
    return;

  This->deviceclass = c;
}

bool hwNode::enabled() const
{
  if (!This)
    return false;

  return (This->enabled);
}

bool hwNode::disabled() const
{
  if (!This)
    return true;

  return !(This->enabled);
}

void hwNode::enable()
{
  if (!This)
    return;

  This->enabled = true;
}

void hwNode::disable()
{
  if (!This)
    return;

  This->enabled = false;
}

string hwNode::getId() const
{
  if (This)
    return This->id;
  else
    return "";
}

void hwNode::setId(const string & id)
{
  if (!This)
    return;

  This->id = cleanupId(id);
}

string hwNode::getVendor() const
{
  if (This)
    return This->vendor;
  else
    return "";
}

void hwNode::setVendor(const string & vendor)
{
  if (This)
    This->vendor = strip(vendor);
}

string hwNode::getProduct() const
{
  if (This)
    return This->product;
  else
    return "";
}

void hwNode::setProduct(const string & product)
{
  if (This)
    This->product = strip(product);
}

string hwNode::getVersion() const
{
  if (This)
    return This->version;
  else
    return "";
}

void hwNode::setVersion(const string & version)
{
  if (This)
    This->version = strip(version);
}

string hwNode::getSerial() const
{
  if (This)
    return This->serial;
  else
    return "";
}

void hwNode::setSerial(const string & serial)
{
  if (This)
    This->serial = strip(serial);
}

string hwNode::getSlot() const
{
  if (This)
    return This->slot;
  else
    return "";
}

void hwNode::setSlot(const string & slot)
{
  if (This)
    This->slot = strip(slot);
}

unsigned long long hwNode::getSize() const
{
  if (This)
    return This->size;
  else
    return 0;
}

void hwNode::setSize(unsigned long long size)
{
  if (This)
    This->size = size;
}

unsigned long long hwNode::getCapacity() const
{
  if (This)
    return This->capacity;
  else
    return 0;
}

void hwNode::setCapacity(unsigned long long capacity)
{
  if (This)
    This->capacity = capacity;
}

unsigned long long hwNode::getClock() const
{
  if (This)
    return This->clock;
  else
    return 0;
}

void hwNode::setClock(unsigned long long clock)
{
  if (This)
    This->clock = clock;
}

unsigned int hwNode::countChildren() const
{
  if (!This)
    return 0;
  else
    return This->children.size();
}

const hwNode *hwNode::getChild(unsigned int i) const
{
  if (!This)
    return NULL;

  if (i >= This->children.size())
    return NULL;
  else
    return &(This->children[i]);
}

hwNode *hwNode::getChild(const string & id)
{
  if (!This)
    return NULL;

  for (int i = 0; i < This->children.size(); i++)
    if (This->children[i].getId() == id)
      return &(This->children[i]);
  return NULL;
}

static string generateId(const string & radical,
			 int count)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "%d", count);

  return radical + ":" + string(buffer);
}

bool hwNode::addChild(const hwNode & node)
{
  hwNode *existing = NULL;
  string id = node.getId();
  int count = 0;

  if (!This)
    return false;

  existing = getChild(id);
  if (existing)			// first rename existing instance
  {
    while (getChild(generateId(id, count)))	// find a usable name
      count++;

    existing->setId(generateId(id, count));	// rename
  }

  while (getChild(generateId(id, count)))
    count++;

  This->children.push_back(node);
  if (existing || getChild(generateId(id, 0)))
    This->children.back().setId(generateId(id, count));

  return true;
}
