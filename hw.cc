#include "hw.h"
#include "osutils.h"
#include <vector>
#include <map>
#include <stdio.h>
#include <ctype.h>

using namespace hw;

static char *id = "@(#) $Id: hw.cc,v 1.48 2003/06/26 21:44:27 ezix Exp $";

struct hwNode_i
{
  hwClass deviceclass;
  string id, vendor, product, version, serial, slot, handle, description,
    logicalname, businfo, physid;
  bool enabled;
  bool claimed;
  unsigned long long start;
  unsigned long long size;
  unsigned long long capacity;
  unsigned long long clock;
    vector < hwNode > children;
    vector < string > attracted;
    vector < string > features;
    map < string,
    string > config;
};

string hw::strip(const string & s)
{
  string result = s;

  while ((result.length() > 0) && (result[0] <= ' '))
    result.erase(0, 1);
  while ((result.length() > 0) && (result[result.length() - 1] <= ' '))
    result.erase(result.length() - 1, 1);
  for (unsigned int i = 0; i < result.length(); i++)
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

  for (unsigned int i = 0; i < result.length(); i++)
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
  This = NULL;
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
  This->handle = string("");
  This->description = string("");
  This->logicalname = string("");
  This->businfo = string("");
  This->physid = string("");

  (void) &::id;			// avoid warning "id defined but not used"
}

hwNode::hwNode(const hwNode & o)
{
  This = NULL;
  This = new hwNode_i;

  if (!This)
    return;

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
  if (this == &o)
    return *this;		// self-affectation

  if (This)
    delete This;
  This = NULL;
  This = new hwNode_i;

  if (!This)
    return *this;

  if (o.This)
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

const char *hwNode::getClassName() const
{
  if (This)
  {
    switch (This->deviceclass)
    {
    case processor:
      return "processor";

    case memory:
      return "memory";

    case address:
      return "address";

    case storage:
      return "storage";

    case disk:
      return "disk";

    case tape:
      return "tape";

    case hw::system:
      return "system";

    case bridge:
      return "bridge";

    case bus:
      return "bus";

    case network:
      return "network";

    case display:
      return "display";

    case input:
      return "input";

    case printer:
      return "printer";

    case multimedia:
      return "multimedia";

    case communication:
      return "communication";

    default:
      return "generic";
    }
  }
  else
    return "generic";
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

  if (This->claimed)
    return true;

  for (unsigned int i = 0; i < This->children.size(); i++)
    if (This->children[i].claimed())
    {
      This->claimed = true;
      return true;
    }

  return false;
}

void hwNode::claim(bool claimchildren)
{
  if (!This)
    return;

  This->claimed = true;

  if (!claimchildren)
    return;

  for (unsigned int i = 0; i < This->children.size(); i++)
    This->children[i].claim(claimchildren);
}

void hwNode::unclaim()
{
  if (!This)
    return;

  This->claimed = false;
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

  for (unsigned int i = 0; i < This->children.size(); i++)
    if (This->children[i].getClass() == c)
      count++;

  return count;
}

hwNode *hwNode::getChild(unsigned int i)
{
  if (!This)
    return NULL;

  if (i >= This->children.size())
    return NULL;
  else
    return &(This->children[i]);
}

hwNode *hwNode::getChildByPhysId(const string & physid)
{
  if (physid == "" || !This)
    return NULL;

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    if (This->children[i].getPhysId() == physid)
      return &(This->children[i]);
  }

  return NULL;
}

hwNode *hwNode::getChildByPhysId(long physid)
{
  char buffer[20];
  if (!This)
    return NULL;

  snprintf(buffer, sizeof(buffer), "%lx", physid);

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    if (This->children[i].getPhysId() == string(buffer))
      return &(This->children[i]);
  }

  return NULL;
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

  for (unsigned int i = 0; i < This->children.size(); i++)
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

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    hwNode *result = This->children[i].findChildByHandle(handle);

    if (result)
      return result;
  }

  return NULL;
}

hwNode *hwNode::findChildByLogicalName(const string & name)
{
  if (!This)
    return NULL;

  if (This->logicalname == name)
    return this;

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    hwNode *result = This->children[i].findChildByLogicalName(name);

    if (result)
      return result;
  }

  return NULL;
}

hwNode *hwNode::findChildByBusInfo(const string & businfo)
{
  if (!This)
    return NULL;

  if (strip(businfo) == "")
    return NULL;

  if (strip(This->businfo) == strip(businfo))
    return this;

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    hwNode *result = This->children[i].findChildByBusInfo(businfo);

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
  hwNode *samephysid = NULL;
  string id = node.getId();
  int count = 0;

  if (!This)
    return NULL;

  // first see if the new node is attracted by one of our children
  for (unsigned int i = 0; i < This->children.size(); i++)
    if (This->children[i].attractsNode(node))
      return This->children[i].addChild(node);

  // find if another child already has the same physical id
  // in that case, we remove BOTH physical ids and let auto-allocation proceed
  if (node.getPhysId() != "")
    samephysid = getChildByPhysId(node.getPhysId());
  if (samephysid)
  {
    samephysid->setPhysId("");
  }

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

  if (samephysid)
    This->children.back().setPhysId("");

  return &(This->children.back());
  //return getChild(This->children.back().getId());
}

void hwNode::attractHandle(const string & handle)
{
  if (!This)
    return;

  This->attracted.push_back(handle);
}

bool hwNode::attractsHandle(const string & handle) const
{
  unsigned int i = 0;
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

  for (unsigned int i = 0; i < This->features.size(); i++)
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

  for (unsigned int i = 0; i < This->features.size(); i++)
    result += This->features[i] + " ";

  return strip(result);
}

void hwNode::setConfig(const string & key,
		       const string & value)
{
  if (!This)
    return;

  This->config[key] = strip(value);
  if (strip(value) == "")
    This->config.erase(This->config.find(key));
}

string hwNode::getConfig(const string & key) const
{
  if (!This)
    return "";

  if (This->config.find(key) == This->config.end())
    return "";

  return This->config[key];
}

vector < string > hwNode::getConfigValues(const string & separator) const
{
  vector < string > result;

  if (!This)
    return result;

  for (map < string, string >::iterator i = This->config.begin();
       i != This->config.end(); i++)
    result.push_back(i->first + separator + i->second);

  return result;
}

string hwNode::getLogicalName() const
{
  if (This)
    return This->logicalname;
  else
    return "";
}

void hwNode::setLogicalName(const string & name)
{
  if (This)
  {
    if (exists("/dev/" + strip(name)))
      This->logicalname = "/dev/" + strip(name);
    else
      This->logicalname = strip(name);
  }
}

string hwNode::getBusInfo() const
{
  if (This)
    return This->businfo;
  else
    return "";
}

void hwNode::setBusInfo(const string & businfo)
{
  if (This)
  {
    if (businfo.find('@') != string::npos)
      This->businfo = strip(businfo);
    else
    {
      string info = strip(businfo);

      if ((info.length() == 7) &&
	  isxdigit(info[0]) &&
	  isxdigit(info[1]) &&
	  (info[2] == ':') &&
	  isxdigit(info[3]) &&
	  isxdigit(info[4]) && (info[5] == '.') && isxdigit(info[6]))
	This->businfo = string("pci@") + info;
    }
  }
}

string hwNode::getPhysId() const
{
  if (This)
    return This->physid;
  else
    return "";
}

void hwNode::setPhysId(long physid)
{
  if (This)
  {
    char buffer[20];

    snprintf(buffer, sizeof(buffer), "%lx", physid);
    This->physid = string(buffer);
  }
}

void hwNode::setPhysId(unsigned physid1,
		       unsigned physid2)
{
  if (This)
  {
    char buffer[40];

    if (physid2 != 0)
      snprintf(buffer, sizeof(buffer), "%x.%x", physid1, physid2);
    else
      snprintf(buffer, sizeof(buffer), "%x", physid1);
    This->physid = string(buffer);
  }
}

void hwNode::setPhysId(const string & physid)
{
  if (This)
  {
    This->physid = strip(physid);
  }
}

void hwNode::assignPhysIds()
{
  if (!This)
    return;

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    long curid = 0;

    if (This->children[i].getClass() == hw::processor)
      curid = 0x100;

    if (This->children[i].getPhysId() == "")
    {
      while (getChildByPhysId(curid))
	curid++;

      This->children[i].setPhysId(curid);
    }

    This->children[i].assignPhysIds();
  }
}

void hwNode::merge(const hwNode & node)
{
  if (!This)
    return;
  if (!node.This)
    return;

  if (This->deviceclass == hw::generic)
    This->deviceclass = node.getClass();
  if (This->vendor == "")
    This->vendor = node.getVendor();
  if (This->product == "")
    This->product = node.getProduct();
  if (This->version == "")
    This->version = node.getVersion();
  if (This->serial == "")
    This->serial = node.getSerial();
  if (This->start == 0)
    This->start = node.getStart();
  if (This->size == 0)
    This->size = node.getSize();
  if (This->capacity == 0)
    This->capacity = node.getCapacity();
  if (This->clock == 0)
    This->clock = node.getClock();
  if (node.enabled())
    enable();
  else
    disable();
  if (node.claimed())
    claim();
  if (This->handle == "")
    This->handle = node.getHandle();
  if (This->description == "")
    This->description = node.getDescription();
  if (This->logicalname == "")
    This->logicalname = node.getLogicalName();
  if (This->businfo == "")
    This->businfo = node.getBusInfo();
  if (This->physid == "")
    This->physid = node.getPhysId();

  for (unsigned int i = 0; i < node.This->features.size(); i++)
    addCapability(node.This->features[i]);

  for (map < string, string >::iterator i = node.This->config.begin();
       i != node.This->config.end(); i++)
    setConfig(i->first, i->second);
}
