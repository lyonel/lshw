#include "hw.h"
#include <vector>
#include <stdio.h>
#include <ctype.h>

using namespace hw;

struct hwNode_i
{
  hwClass deviceclass;
  string id, vendor, product, version, serial, slot, handle, description;
  bool enabled;
  bool claimed;
  unsigned long long start;
  unsigned long long size;
  unsigned long long capacity;
  unsigned long long clock;
    vector < hwNode > children;
    vector < string > attracted;
    vector < string > features;
};

string hw::strip(const string & s)
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
  This->start = 0;
  This->size = 0;
  This->capacity = 0;
  This->clock = 0;
  This->enabled = true;
  This->claimed = false;
  This->handle = "";
  This->description = "";
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

bool hwNode::claimed() const
{
  if (!This)
    return false;

  return (This->claimed);
}

void hwNode::claim()
{
  if (!This)
    return;

  This->claimed = true;
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

void hwNode::setHandle(const string & handle)
{
  if (!This)
    return;

  This->handle = handle;
}

string hwNode::getHandle() const
{
  if (This)
    return This->handle;
  else
    return "";
}

string hwNode::getDescription() const
{
  if (This)
    return This->description;
  else
    return "";
}

void hwNode::setDescription(const string & description)
{
  if (This)
    This->description = strip(description);
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

unsigned long long hwNode::getStart() const
{
  if (This)
    return This->start;
  else
    return 0;
}

void hwNode::setStart(unsigned long long start)
{
  if (This)
    This->start = start;
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

unsigned int hwNode::countChildren(hw::hwClass c) const
{
  unsigned int count = 0;

  if (!This)
    return 0;

  if (c == hw::generic)
    return This->children.size();

  for (int i = 0; i < This->children.size(); i++)
    if (This->children[i].getClass() == c)
      count++;

  return count;
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
  string baseid = id, path = "";
  size_t pos = 0;

  if (!This)
    return NULL;

  pos = id.find('/');
  if (pos != string::npos)
  {
    baseid = id.substr(0, pos);
    if (pos < id.length() - 1)
      path = id.substr(pos + 1);
  }

  for (int i = 0; i < This->children.size(); i++)
    if (This->children[i].getId() == baseid)
    {
      if (path == "")
	return &(This->children[i]);
      else
	return This->children[i].getChild(path);
    }
  return NULL;
}

hwNode *hwNode::findChildByHandle(const string & handle)
{
  if (!This)
    return NULL;

  if (This->handle == handle)
    return this;

  for (int i = 0; i < This->children.size(); i++)
  {
    hwNode *result = This->children[i].findChildByHandle(handle);

    if (result)
      return result;
  }

  return NULL;
}

static string generateId(const string & radical,
			 int count)
{
  char buffer[10];

  snprintf(buffer, sizeof(buffer), "%d", count);

  return radical + ":" + string(buffer);
}

hwNode *hwNode::addChild(const hwNode & node)
{
  hwNode *existing = NULL;
  string id = node.getId();
  int count = 0;

  if (!This)
    return NULL;

  // first see if the new node is attracted by one of our children
  for (int i = 0; i < This->children.size(); i++)
    if (This->children[i].attractsNode(node))
      return This->children[i].addChild(node);

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

  return getChild(This->children.back().getId());
}

void hwNode::attractHandle(const string & handle)
{
  if (!This)
    return;

  This->attracted.push_back(handle);
}

bool hwNode::attractsHandle(const string & handle) const
{
  int i = 0;
  if (handle == "" || !This)
    return false;

  for (i = 0; i < This->attracted.size(); i++)
    if (This->attracted[i] == handle)
      return true;

  for (i = 0; i < This->children.size(); i++)
    if (This->children[i].attractsHandle(handle))
      return true;

  return false;
}

bool hwNode::attractsNode(const hwNode & node) const
{
  if (!This || !node.This)
    return false;

  return attractsHandle(node.This->handle);
}

bool hwNode::isCapable(const string & feature) const
{
  string featureid = cleanupId(feature);

  if (!This)
    return false;

  for (int i = 0; i < This->features.size(); i++)
    if (This->features[i] == featureid)
      return true;

  return false;
}

void hwNode::addCapability(const string & feature)
{
  string features = feature;

  if (!This)
    return;

  while (features.length() > 0)
  {
    size_t pos = features.find('\0');

    if (pos == string::npos)
    {
      if (!isCapable(cleanupId(features)))
	This->features.push_back(cleanupId(features));
      features = "";
    }
    else
    {
      string featureid = cleanupId(features.substr(0, pos));
      if (!isCapable(featureid))
	This->features.push_back(featureid);
      features = features.substr(pos + 1);
    }
  }
}

string hwNode::getCapabilities() const
{
  string result = "";

  if (!This)
    return "";

  for (int i = 0; i < This->features.size(); i++)
    result += This->features[i] + " ";

  return strip(result);
}

static char *id = "@(#) $Id: hw.cc,v 1.26 2003/01/27 14:25:08 ezix Exp $";
