#include "hw.h"
#include <vector>

using namespace hw;

struct hwNode_i
{
  hwClass deviceclass;
  string id, vendor, product, version, serial;
  unsigned long long size;
    vector < hwNode > children;
};

hwNode::hwNode(const string & id,
	       hwClass c,
	       const string & vendor,
	       const string & product,
	       const string & version)
{
  This = new hwNode_i;

  if (!This)
    return;

  This->deviceclass = c;
  This->id = id;
  This->vendor = vendor;
  This->product = product;
  This->version = version;
  This->size = 0;
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
}

hwClass hwNode::getClass() const
{
  if (This)
    return This->deviceclass;
  else
    return hw::generic;
}

string hwNode::getId() const
{
  if (This)
    return This->id;
  else
    return "";
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
    This->vendor = vendor;
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
    This->product = product;
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
    This->version = version;
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
    This->serial = serial;
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

bool hwNode::addChild(const hwNode & node)
{
  if (!This)
    return false;

  This->children.push_back(node);
  return true;
}
