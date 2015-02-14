#define _XOPEN_SOURCE
#include "hw.h"
#include "osutils.h"
#include "version.h"
#include "config.h"
#include "options.h"
#include "heuristics.h"
#include "dump.h"
#include "db.h"
#include <cstring>
#include <vector>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <limits.h>
#include <sys/utsname.h>

using namespace hw;

__ID("@(#) $Id$");

struct hwNode_i
{
  hwClass deviceclass;
  string id, vendor, product, version, date, serial, slot, handle, description,
    businfo, physid, dev;
  bool enabled;
  bool claimed;
  unsigned long long start;
  unsigned long long size;
  unsigned long long capacity;
  unsigned long long clock;
  unsigned int width;
  vector < hwNode > children;
  vector < string > attracted;
  vector < string > features;
  vector < string > logicalnames;
  map < string, string > features_descriptions;
  vector < resource > resources;
  map < string, string > config;
  map < string, value > hints;
};

string hw::strip(const string & s)
{
  string result = s;
  size_t i = result.find('\0');

  if(i != string::npos)
    result = result.substr(0, i);

  while ((result.length() > 0) && ((uint8_t)result[0] <= ' '))
    result.erase(0, 1);
  while ((result.length() > 0) && ((uint8_t)result[result.length() - 1] <= ' '))
    result.erase(result.length() - 1);

  for (i = 0; i < result.length(); i++)
    if ((uint8_t)result[i] < ' ')
    {
      result.erase(i, 1);
      i--;
    }

  result = utf8_sanitize(result);

  return result;
}

string hw::asString(long n)
{
  char buffer[256];

  snprintf(buffer, sizeof(buffer), "%ld", n);
  return string(buffer);
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
  This->width = 0;
  This->enabled = true;
  This->claimed = false;
  This->handle = string("");
  This->description = string("");
  This->businfo = string("");
  This->physid = string("");
  This->dev = string("");
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
    return *this;                                 // self-affectation

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

      case power:
        return "power";

      case volume:
        return "volume";

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

string hwNode::getDate() const
{
  if (This)
    return This->date;
  else
    return "";
}


void hwNode::setDate(const string & s)
{
  if (This)
    This->date = strip(s);
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
  if(serial == "00000000-0000-0000-0000-000000000000")
    return;

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
    if (This->children[i].getId() == cleanupId(baseid))
  {
    if (path == "")
      return &(This->children[i]);
    else
      return This->children[i].getChild(path);
  }
  return NULL;
}


hwNode *hwNode::findChild(bool(*matchfunction) (const hwNode &))
{
  if (!This)
    return NULL;

  if (matchfunction(*this))
    return this;

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    hwNode *result = This->children[i].findChild(matchfunction);

    if (result)
      return result;
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
  unsigned int i = 0;

  if (!This)
    return NULL;

  for (i = 0; i < This->logicalnames.size(); i++)
    if (This->logicalnames[i] == name)
      return this;

  for (i = 0; i < This->children.size(); i++)
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

  if (lowercase(strip(This->businfo)) == lowercase(strip(businfo)))
    return this;

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    hwNode *result = This->children[i].findChildByBusInfo(businfo);

    if (result)
      return result;
  }

  return NULL;
}


hwNode *hwNode::findChildByResource(const hw::resource & r)
{
  if (!This)
    return NULL;

  if (this->usesResource(r))
    return this;

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    hwNode *result = This->children[i].findChildByResource(r);

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
  if (existing)                                   // first rename existing instance
  {
    while (getChild(generateId(id, count)))       // find a usable name
      count++;

    existing->setId(generateId(id, count));       // rename
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


void hwNode::addCapability(const string & feature,
const string & description)
{
  string features = feature;

  if (!This)
    return;

  if (description != "")
    This->features_descriptions[cleanupId(feature)] = strip(description);

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


void hwNode::describeCapability(const string & feature,
const string & description)
{
  if (!This)
    return;

  if (!isCapable(feature))
    return;

  This->features_descriptions[cleanupId(feature)] = strip(description);
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


vector < string > hwNode::getCapabilitiesList() const
{
  vector < string > result;

  if (!This)
    return result;

  for (unsigned int i = 0; i < This->features.size(); i++)
    result.push_back(This->features[i]);

  return result;
}


string hwNode::getCapabilityDescription(const string & feature) const
{
  string featureid = cleanupId(feature);

  if (!isCapable(feature))
    return "";

  if (!This)
    return "";

  if (This->features_descriptions.find(featureid) ==
    This->features_descriptions.end())
    return "";

  return This->features_descriptions[featureid];
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


void hwNode::setConfig(const string & key,
unsigned long long value)
{
  if (!This)
    return;

  This->config[key] = tostring(value);
}


string hwNode::getConfig(const string & key) const
{
  if (!This)
    return "";

  if (This->config.find(key) == This->config.end())
    return "";

  return This->config[key];
}


vector < string > hwNode::getConfigKeys() const
{
  vector < string > result;

  if (!This)
    return result;

  for (map < string, string >::iterator i = This->config.begin();
    i != This->config.end(); i++)
  result.push_back(i->first);

  return result;
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
  if (This && (This->logicalnames.size()>0))
    return This->logicalnames[0];
  else
    return "";
}


vector<string> hwNode::getLogicalNames() const
{
  if (This)
    return This->logicalnames;
  else
    return vector<string>();
}


void hwNode::setLogicalName(const string & name)
{
  string n = strip(name);

  if (This)
  {
    for (unsigned i = 0; i < This->logicalnames.size(); i++)
      if (This->logicalnames[i] == n || This->logicalnames[i] == "/dev/" + n)
        return;                                   // nothing to add, this logical name already exists
    if ((name[0] != '/') && exists("/dev/" + n))
    {
      This->logicalnames.push_back("/dev/" + n);
    }
    else
      This->logicalnames.push_back(n);

    if(This->dev == "")
      This->dev = get_devid(n);
  }
}


string hwNode::getDev() const
{
  if (This)
    return This->dev;
  else
    return "";
}


void hwNode::setDev(const string & s)
{
  if (This)
  {
    string devid = strip(s);
    unsigned int i = 0;

    while((i<devid.length()) && isdigit(devid[i])) i++;
    if(i>=devid.length()) return;
    if(devid[i] != ':') return;
    i++;
    while((i<devid.length()) && isdigit(devid[i])) i++;
    if(i!=devid.length()) return;

    This->dev = devid;
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
      This->businfo = guessBusInfo(strip(businfo));
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


void hwNode::setPhysId(unsigned physid1,
unsigned physid2,
unsigned physid3)
{
  if (This)
  {
    char buffer[40];

    snprintf(buffer, sizeof(buffer), "%x.%x.%x", physid1, physid2, physid3);
    This->physid = string(buffer);
  }
}


void hwNode::setPhysId(const string & physid)
{
  if (This)
  {
    This->physid = strip(physid);

    while ((This->physid.length() > 1) && (This->physid[0] == '0'))
      This->physid.erase(0, 1);
  }
}


void hwNode::assignPhysIds()
{
  if (!This)
    return;

  for (unsigned int i = 0; i < This->children.size(); i++)
  {
    long curid = 0;

    if (This->children[i].getClass() == hw::bridge)
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


void hwNode::fixInconsistencies()
{
  if (!This)
    return;

  if (getCapacity() < getSize())
    setCapacity(0);

  guessProduct(*this);
  guessVendor(*this);

  if(getLogicalName() != "")
    claim();

  for (unsigned int i = 0; i < This->children.size(); i++)
    This->children[i].fixInconsistencies();
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
  if (This->width == 0)
    This->width = node.getWidth();
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
  if (This->logicalnames.size() == 0)
    This->logicalnames = node.getLogicalNames();
  if (This->businfo == "")
    This->businfo = node.getBusInfo();
  if (This->physid == "")
    This->physid = node.getPhysId();

  for (unsigned int i = 0; i < node.This->features.size(); i++)
    addCapability(node.This->features[i]);
  for (map < string, string >::iterator i = node.This->features_descriptions.begin();
    i != node.This->features_descriptions.end(); i++)
  describeCapability(i->first, i->second);

  for (map < string, string >::iterator i = node.This->config.begin();
    i != node.This->config.end(); i++)
  setConfig(i->first, i->second);

  for (map < string, value >::iterator i = node.This->hints.begin();
    i != node.This->hints.end(); i++)
  addHint(i->first, i->second);
}


void hwNode::addResource(const resource & r)
{
  if (!This)
    return;

  This->resources.push_back(r);
}


bool hwNode::usesResource(const resource & r) const
{
  if (!This)
    return false;

  for (size_t i = 0; i < This->resources.size(); i++)
    if (r == This->resources[i])
      return true;

  return false;
}


vector < string > hwNode::getResources(const string & separator) const
{
  vector < string > result;

  if (!This)
    return result;

  for (vector < resource >::iterator i = This->resources.begin();
    i != This->resources.end(); i++)
  result.push_back(i->asString(separator));

  return result;
}


void hwNode::setWidth(unsigned int width)
{
  if(This)
    This->width = width;
}


unsigned int hwNode::getWidth() const
{
  if(This)
    return This->width;
  else
    return 0;
}


void hwNode::addHint(const string & id, const value & v)
{
  if(This)
    This->hints[id] = v;
}


value hwNode::getHint(const string & id) const
{
  if(!This)
    return value();

  return This->hints[id];
}

vector < string > hwNode::getHints() const
{
  vector < string > result;

  if (!This)
    return result;

  for (map < string, value >::iterator i = This->hints.begin();
    i != This->hints.end(); i++)
  result.push_back(i->first);

  return result;
}

string hwNode::asJSON(unsigned level)
{
  vector < string > config;
  vector < string > resources;
  ostringstream out;

  if(!This) return "";

  config = getConfigKeys();
  resources = getResources("\" value=\"");

  if(visible(getClassName()))
  {
    out << "{" << endl;
    out << spaces(2*level+2) << "\"id\" : \"" << getId() << "\"," << endl;
    out << spaces(2*level+2) << "\"class\" : \"" << getClassName() << "\"";

    if (disabled())
      out << "," << endl << spaces(2*level+2) << "\"disabled\" : true";
    if (claimed())
      out << "," << endl << spaces(2*level+2) << "\"claimed\" : true";

    if(getHandle() != "")
      out << "," << endl << spaces(2*level+2) << "\"handle\" : \"" << getHandle() << "\"";

    if (getDescription() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"description\" : \"";
      out << escapeJSON(getDescription());
      out << "\"";
    }

    if (getProduct() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"product\" : \"";
      out << escapeJSON(getProduct());
      out << "\"";
    }

    if (getVendor() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"vendor\" : \"";
      out << escapeJSON(getVendor());
      out << "\"";
    }

    if (getPhysId() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"physid\" : \"";
      out << getPhysId();
      out << "\"";
    }

    if (getBusInfo() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"businfo\" : \"";
      out << escapeJSON(getBusInfo());
      out << "\"";
    }

    if (getLogicalName() != "")
    {
      vector<string> logicalnames = getLogicalNames();

      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"logicalname\" : ";
      if(logicalnames.size() > 1)
      {
        out << "[";
        for(unsigned int i = 0; i<logicalnames.size(); i++)
        {
          if(i) out << ", ";
          out << "\"" << logicalnames[i] << "\"";
        }
        out << "]";
      }
      else
        out << "\"" << escapeJSON(getLogicalName()) << "\"";
    }

    if (getDev() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"dev\" : \"";
      out << escapeJSON(getDev());
      out << "\"";
    }

    if (getVersion() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"version\" : \"";
      out << escapeJSON(getVersion());
      out << "\"";
    }

    if (getDate() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"date\" : \"";
      out << escapeJSON(getDate());
      out << "\"";
    }

    if (getSerial() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"serial\" : \"";
      out << (::enabled("output:sanitize")?REMOVED:escapeJSON(getSerial()));
      out << "\"";
    }

    if (getSlot() != "")
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"slot\" : \"";
      out << escapeJSON(getSlot());
      out << "\"";
    }

    if ((getSize() > 0) || (getCapacity() > 0))
      switch (getClass())
      {
        case hw::memory:
        case hw::address:
        case hw::storage:
        case hw::disk:
        case hw::display:
          out << "," << endl << spaces(2*level+2) << "\"units\" : \"bytes\"";
          break;

        case hw::processor:
        case hw::bus:
        case hw::system:
          out << "," << endl << spaces(2*level+2) << "\"units\" : \"Hz\"";
          break;

        case hw::power:
          out << "," << endl << spaces(2*level+2) << "\"units\" : \"mWh\"";
          break;

        case hw::network:
          out << "," << endl << spaces(2*level+2) << "\"units\" : \"bit/s\"";
          break;

        default:
          break;
      }

    if (getSize() > 0)
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"size\" : ";
      out << getSize();
    }

    if (getCapacity() > 0)
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"capacity\" : ";
      out << getCapacity();
    }

    if (getWidth() > 0)
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"width\" : ";
      out << getWidth();
    }

    if (getClock() > 0)
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"clock\" : ";
      out << getClock();
    }

    if (config.size() > 0)
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"configuration\" : {" << endl;
      for (unsigned int j = 0; j < config.size(); j++)
      {
        if(j) out << "," << endl;
        out << spaces(2*level+4);
        out << "\"" << escapeJSON(config[j]) << "\" : \"" << escapeJSON(getConfig(config[j])) << "\"";
      }
      out << endl << spaces(2*level+2);
      out << "}";
    }
    config.clear();

    splitlines(getCapabilities(), config, ' ');
    if (config.size() > 0)
    {
      out << "," << endl;
      out << spaces(2*level+2);
      out << "\"capabilities\" : {" << endl;
      for (unsigned int j = 0; j < config.size(); j++)
      {
        if(j) out << "," << endl;
        out << spaces(2*level+4);
        out << "\"" << escapeJSON(config[j]) << "\" : ";
        if (getCapabilityDescription(config[j]) == "")
        {
          out << "true";
        }
        else
        {
          out << "\"";
          out << escapeJSON(getCapabilityDescription(config[j]));
          out << "\"";
        }
      }
      out << endl << spaces(2*level+2);
      out << "}";
    }
    config.clear();

    if (0 && resources.size() > 0)
    {
      out << spaces(2*level+1);
      out << "<resources>" << endl;
      for (unsigned int j = 0; j < resources.size(); j++)
      {
        out << spaces(2*level+2);
        out << "<resource type=\"" << escapeJSON(resources[j]) << "\" />";
        out << endl;
      }
      out << spaces(2*level+1);
      out << "</resources>" << endl;
    }
    resources.clear();
  }

  
  if(countChildren()>0)
  {
    if(visible(getClassName()))
      out << "," << endl << spaces(2*level+2) << "\"children\" : [" << endl;

    for (unsigned int i = 0; i < countChildren(); i++)
    {
      out << spaces(2*level+4) << getChild(i)->asJSON(visible(getClassName()) ? level + 2 : 1);
      if(visible(getChild(i)->getClassName()) && (i < countChildren()-1)) out << "," << endl;
    }

    if(visible(getClassName()))
      out << endl << spaces(2*level+2) << "]";
  }

  if(visible(getClassName()))
  {
    out << endl << spaces(2*level);
    out << "}";
  }

  return out.str();
}

string hwNode::asXML(unsigned level)
{
  vector < string > config;
  vector < string > resources;
  ostringstream out;

  if(!This) return "";

  config = getConfigKeys();
  resources = getResources("\" value=\"");

  if (level == 0)
  {
    struct utsname un;

    out << "<?xml version=\"1.0\" standalone=\"yes\" ?>" << endl;
    out << _("<!-- generated by lshw-") << getpackageversion() << " -->" <<
  #if defined(__GNUC__) && defined(__VERSION__)
      endl << "<!-- GCC " << escapecomment(__VERSION__) << " -->" <<
  #endif
      endl;

    if(uname(&un) == 0)
      out << "<!-- " << escapecomment(un.sysname) << " " << escapecomment(un.release) << " " << escapecomment(un.version) << " " << escapecomment(un.machine) << " -->" << endl;
  #if defined(__GLIBC__) && defined(_CS_GNU_LIBC_VERSION)
    char version[PATH_MAX];

      if(confstr(_CS_GNU_LIBC_VERSION, version, sizeof(version))>0)
        out << "<!-- GNU libc " << __GLIBC__ << " (" << escapecomment(version) << ") -->" << endl;
  #endif
    if (geteuid() != 0)
      out << _("<!-- WARNING: not running as root -->") << endl;

    if(::enabled("output:list"))
      out << "<list>" << endl;

  }

  if(visible(getClassName()))
  {
    out << spaces(2*level);
    out << "<node id=\"" << getId() << "\"";
    if (disabled())
      out << " disabled=\"true\"";
    if (claimed())
      out << " claimed=\"true\"";

    out << " class=\"" << getClassName() << "\"";
    out << " handle=\"" << getHandle() << "\"";
    out << ">" << endl;

    if (getDescription() != "")
    {
      out << spaces(2*level+1);
      out << "<description>";
      out << escape(getDescription());
      out << "</description>";
      out << endl;
    }

    if (getProduct() != "")
    {
      out << spaces(2*level+1);
      out << "<product>";
      out << escape(getProduct());
      out << "</product>";
      out << endl;
    }

    if (getVendor() != "")
    {
      out << spaces(2*level+1);
      out << "<vendor>";
      out << escape(getVendor());
      out << "</vendor>";
      out << endl;
    }

    if (getPhysId() != "")
    {
      out << spaces(2*level+1);
      out << "<physid>";
      out << getPhysId();
      out << "</physid>";
      out << endl;
    }

    if (getBusInfo() != "")
    {
      out << spaces(2*level+1);
      out << "<businfo>";
      out << escape(getBusInfo());
      out << "</businfo>";
      out << endl;
    }

    if (getLogicalName() != "")
    {
      vector<string> logicalnames = getLogicalNames();

      for(unsigned int i = 0; i<logicalnames.size(); i++)
      {
        out << spaces(2*level+1);
        out << "<logicalname>";
        out << logicalnames[i];
        out << "</logicalname>";
        out << endl;
      }
    }

    if (getDev() != "")
    {
      out << spaces(2*level+1);
      out << "<dev>";
      out << escape(getDev());
      out << "</dev>";
      out << endl;
    }

    if (getVersion() != "")
    {
      out << spaces(2*level+1);
      out << "<version>";
      out << escape(getVersion());
      out << "</version>";
      out << endl;
    }

    if (getDate() != "")
    {
      out << spaces(2*level+1);
      out << "<date>";
      out << escape(getDate());
      out << "</date>";
      out << endl;
    }

    if (getSerial() != "")
    {
      out << spaces(2*level+1);
      out << "<serial>";
      out << (::enabled("output:sanitize")?REMOVED:escape(getSerial()));
      out << "</serial>";
      out << endl;
    }

    if (getSlot() != "")
    {
      out << spaces(2*level+1);
      out << "<slot>";
      out << escape(getSlot());
      out << "</slot>";
      out << endl;
    }

    if (getSize() > 0)
    {
      out << spaces(2*level+1);
      out << "<size";
      switch (getClass())
      {
        case hw::memory:
        case hw::address:
        case hw::storage:
        case hw::disk:
        case hw::volume:
        case hw::display:
          out << " units=\"bytes\"";
          break;

        case hw::processor:
        case hw::bus:
        case hw::system:
          out << " units=\"Hz\"";
          break;

        case hw::network:
          out << " units=\"bit/s\"";
          break;

        case hw::power:
          out << " units=\"mWh\"";
          break;

        default:
          out << "";
      }
      out << ">";
      out << getSize();
      out << "</size>";
      out << endl;
    }

    if (getCapacity() > 0)
    {
      out << spaces(2*level+1);
      out << "<capacity";
      switch (getClass())
      {
        case hw::memory:
        case hw::address:
        case hw::storage:
        case hw::disk:
          out << " units=\"bytes\"";
          break;

        case hw::processor:
        case hw::bus:
        case hw::system:
          out << " units=\"Hz\"";
          break;

        case hw::power:
          out << " units=\"mWh\"";
          break;

        default:
          out << "";
      }
      out << ">";
      out << getCapacity();
      out << "</capacity>";
      out << endl;
    }

    if (getWidth() > 0)
    {
      out << spaces(2*level+1);
      out << "<width units=\"bits\">";
      out << getWidth();
      out << "</width>";
      out << endl;
    }

    if (getClock() > 0)
    {
      out << spaces(2*level+1);
      out << "<clock units=\"Hz\">";
      out << getClock();
      out << "</clock>";
      out << endl;
    }

    if (config.size() > 0)
    {
      out << spaces(2*level+1);
      out << "<configuration>" << endl;
      for (unsigned int j = 0; j < config.size(); j++)
      {
        out << spaces(2*level+2);
        out << "<setting id=\"" << escape(config[j]) << "\" value=\"" << escape(getConfig(config[j])) << "\" />";
        out << endl;
      }
      out << spaces(2*level+1);
      out << "</configuration>" << endl;
    }
    config.clear();

    splitlines(getCapabilities(), config, ' ');
    if (config.size() > 0)
    {
      out << spaces(2*level+1);
      out << "<capabilities>" << endl;
      for (unsigned int j = 0; j < config.size(); j++)
      {
        out << spaces(2*level+2);
        out << "<capability id=\"" << escape(config[j]);
        if (getCapabilityDescription(config[j]) == "")
        {
          out << "\" />";
        }
        else
        {
          out << "\" >";
          out << escape(getCapabilityDescription(config[j]));
          out << "</capability>";
        }
        out << endl;
      }
      out << spaces(2*level+1);
      out << "</capabilities>" << endl;
    }
    config.clear();

    if (resources.size() > 0)
    {
      out << spaces(2*level+1);
      out << "<resources>" << endl;
      for (unsigned int j = 0; j < resources.size(); j++)
      {
        out << spaces(2*level+2);
        out << "<resource type=\"" << resources[j] << "\" />";
        out << endl;
      }
      out << spaces(2*level+1);
      out << "</resources>" << endl;
    }
    resources.clear();
  }

  for (unsigned int i = 0; i < countChildren(); i++)
  {
    out << getChild(i)->asXML(visible(getClassName()) ? level + 1 : 1);
  }

  if(visible(getClassName()))
  {
    out << spaces(2*level);
    out << "</node>" << endl;
  }

  if((level==0) && ::enabled("output:list"))
    out << "</list>" << endl;


  return out.str();
}

string hwNode::asString()
{
  string summary = "";
  if(!This)
    return "";

  if(getClass() != hw::memory)
    summary = getProduct();        // memory devices tend to have obscure product names
  if (summary == "")
    summary = getDescription();

  if((getClass() == hw::memory) || (getClass() == hw::disk) || (getClass() == hw::storage) || (getClass() == hw::volume))
  {
    unsigned long long size = 0;
    if(getClass() != hw::memory)
    {
      if(getCapacity())
        size = getCapacity();
      else
      {
        if(getSize())
          size = getSize();
      }
    }
    else
    {
      if(getSize())
        size = getSize();
    }
    if(size)
      summary = (getClass()==hw::disk?(decimalkilos(size)+"B"):kilobytes(size)) +" "+ summary;
  }

  return summary;
}

struct hw::resource_i
{
  hw::hwResourceType type;

  unsigned int ui1;
  unsigned long ul1, ul2;
  unsigned long long ull1, ull2;
  bool b;

  int refcount;
};

resource::resource()
{
  This = new struct resource_i;

  if (This)
  {
    memset(This, 0, sizeof(*This));
    This->type = none;
    This->refcount = 1;
  }
}


resource::~resource()
{
  if (This)
  {
    This->refcount--;

    if (This->refcount <= 0)
    {
      delete This;
      This = NULL;
    }
  }
}


resource::resource(const resource & r)
{
  This = r.This;

  if (This)
    This->refcount++;
}


resource & resource::operator = (const resource & r)
{
  if (this == &r)
    return *this;                                 // ignore self-affectation

  if (This == r.This)
    return *this;                                 // both objects reference the same data

  if (This)
  {
    This->refcount--;

    if (This->refcount <= 0)
    {
      delete This;
      This = NULL;
    }
  }

  This = r.This;
  if (This)
    This->refcount++;

  return *this;
}


resource resource::iomem(unsigned long long start,
unsigned long long end)
{
  resource r;

  if (!r.This)
    return r;

  r.This->type = hw::iomem;
  r.This->ull1 = start;
  r.This->ull2 = end;

  return r;
}


resource resource::ioport(unsigned long start,
unsigned long end)
{
  resource r;

  if (!r.This)
    return r;

  r.This->type = hw::ioport;
  r.This->ul1 = start;
  r.This->ul2 = end;

  return r;
}


resource resource::mem(unsigned long long start,
unsigned long long end, bool prefetchable)
{
  resource r;

  if (!r.This)
    return r;

  r.This->type = hw::mem;
  r.This->ull1 = start;
  r.This->ull2 = end;
  r.This->b = prefetchable;

  return r;
}


resource resource::irq(unsigned int value)
{
  resource r;

  if (!r.This)
    return r;

  r.This->type = hw::irq;
  r.This->ui1 = value;

  return r;
}


resource resource::dma(unsigned int value)
{
  resource r;

  if (!r.This)
    return r;

  r.This->type = hw::dma;
  r.This->ui1 = value;

  return r;
}


string resource::asString(const string & separator) const
{
  char buffer[80];
  string result = "";

  if (!This)
    return result;

  strncpy(buffer, "", sizeof(buffer));

  switch (This->type)
  {
    case hw::none:
      result = _("(none)");
      break;
    case hw::dma:
      result = _("dma") + separator;
      snprintf(buffer, sizeof(buffer), "%d", This->ui1);
      break;
    case hw::irq:
      result = _("irq") + separator;
      snprintf(buffer, sizeof(buffer), "%d", This->ui1);
      break;
    case hw::iomem:
      result = _("iomemory") + separator;
      snprintf(buffer, sizeof(buffer), "%llx-%llx", This->ull1, This->ull2);
      break;
    case hw::mem:
      result = _("memory") + separator;
      snprintf(buffer, sizeof(buffer), "%llx-%llx", This->ull1, This->ull2);
      if(This->b) strcat(buffer, _("(prefetchable)"));
      break;
    case hw::ioport:
      result = _("ioport") + separator;
      if(This->ul1 == This->ul2)
        snprintf(buffer, sizeof(buffer), "%lx", This->ul1);
      else
        snprintf(buffer, sizeof(buffer), _("%lx(size=%ld)"), This->ul1, This->ul2 - This->ul1 + 1);
      break;
    default:
      result = _("(unknown)");
  }

  return result + string(buffer);
}


bool resource::operator == (const resource & r)
const
{
  if (This == r.This)
    return true;

  if (!This || !r.This)
    return false;

  if (This->type != r.This->type)
    return false;

  switch (This->type)
  {
    case hw::dma:
    case hw::irq:
      return This->ui1 == r.This->ui1;
      break;

    case hw::iomem:
    case hw::mem:
      return ((This->ull1 >= r.This->ull1)
      && (This->ull2 <= r.This->ull2)) || ((r.This->ull1 >= This->ull1)
      && (r.This->ull2 <= This->ull2));
    break;

    case hw::ioport:
      return ((This->ul1 >= r.This->ul1)
      && (This->ul1 <= r.This->ul2)) || ((r.This->ul1 >= This->ul1)
      && (r.This->ul1 <= This->ul2));
    break;

    default:return false;
  }
}


struct hw::value_i
{
  hw::hwValueType type;

  long long ll;
  string s;
  bool b;

  int refcount;
};

value::value()
{
  This = new hw::value_i;

  if(This)
  {
    This->type = nil;
    This->refcount = 1;
  }
}


value::~value()
{
  if(This)
  {
    This->refcount--;
    if(This->refcount<=0)
      delete This;
    This = NULL;
  }
}


value::value(long long ll)
{
  This = new hw::value_i;

  if(This)
  {
    This->type = integer;
    This->ll = ll;
    This->refcount = 1;
  }
}


value::value(const string & s)
{
  This = new hw::value_i;

  if(This)
  {
    This->type = text;
    This->s = s;
    This->refcount = 1;
  }
}


value::value(const value & v)
{
  This = v.This;

  if(This)
  {
    This->refcount++;
  }
}


value & value::operator=(const value & v)
{
  if(v.This == This) return *this;

  if(This)
  {
    This->refcount--;
    if(This->refcount<=0)
      delete This;
  }

  This = v.This;
  if(This)
    This->refcount++;

  return *this;
}


bool value::operator==(const value & v) const
{
  if(v.This == This) return true;

  if(!v.This || !This) return false;

  if(v.This->type != This->type) return false;

  switch(This->type)
  {
    case hw::integer:
      return This->ll == v.This->ll;
    case hw::text:
      return This->s == v.This->s;
    case hw::boolean:
      return This->b == v.This->b;
    case hw::nil:
      return true;
  };

  return false;
}


string value::asString() const
{
  if(!This) return "";

  switch(This->type)
  {
    case hw::integer:
      return "0x"+tohex(This->ll);
    case hw::text:
      return This->s;
    case hw::boolean:
      return This->b?_("true"):_("false");
    case hw::nil:
      return _("(nil)");
  };

  return "";
}


bool value::defined() const
{
  if(!This) return false;
  if(This->type==nil) return false;

  return true;
}

bool hwNode::dump(const string & filename, bool recurse)
{
#ifdef SQLITE
  try {
    sqlite::database db(filename);

    db.execute("BEGIN TRANSACTION");
    ::dump(*this, db, "", recurse);
    db.execute("COMMIT");
  }
  catch(exception & e)
  {
    return false;
  }
#else
  return false;
#endif

  return true;
}
