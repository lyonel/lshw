#ifndef _HW_H_
#define _HW_H_

#include <string>
#include <vector>

using namespace std;

namespace hw
{

  typedef enum
  {
    system,
    bridge,
    memory,
    processor,
    address,
    storage,
    disk,
    tape,
    bus,
    network,
    display,
    input,
    printer,
    multimedia,
    communication,
    power,
    volume,
    generic
  } hwClass;

  typedef enum { none, iomem, ioport, mem, irq, dma }
  hwResourceType;
  typedef enum { nil, boolean, integer, text }
  hwValueType;

  string strip(const string &);
  string asString(long);

  string reportSize(unsigned long long);

  class resource
  {
    public:

      resource();
      ~resource();
      resource(const resource &);
      resource & operator =(const resource &);

      static resource iomem(unsigned long long, unsigned long long);
      static resource ioport(unsigned long, unsigned long);
      static resource mem(unsigned long long, unsigned long long, bool prefetchable = false);
      static resource irq(unsigned int);
      static resource dma(unsigned int);

      bool operator ==(const resource &) const;

      string asString(const string & separator = ":") const;

    private:
      struct resource_i * This;

  };

  class value
  {
    public:

      value();
      ~value();
      value(const value &);
      value(long long);
      value(const string &);
      value & operator =(const value &);

      bool operator ==(const value &) const;

      string asString() const;
      bool defined() const;

    private:
      struct value_i * This;

  };

}                                                 // namespace hw


class hwNode
{
  public:
    hwNode(const string & id,
      hw::hwClass c = hw::generic,
      const string & vendor = "",
      const string & product = "",
      const string & version = "");
    hwNode(const hwNode & o);
    ~hwNode();
    hwNode & operator =(const hwNode & o);

    string getId() const;

    void setHandle(const string & handle);
    string getHandle() const;

    bool enabled() const;
    bool disabled() const;
    void enable();
    void disable();
    bool claimed() const;
    void claim(bool claimchildren=false);
    void unclaim();

    hw::hwClass getClass() const;
    const char * getClassName() const;
    void setClass(hw::hwClass c);

    string getDescription() const;
    void setDescription(const string & description);

    string getVendor() const;
    void setVendor(const string & vendor);

    string getProduct() const;
    void setProduct(const string & product);

    string getVersion() const;
    void setVersion(const string & version);

    string getDate() const;
    void setDate(const string &);

    string getSerial() const;
    void setSerial(const string & serial);

    unsigned long long getStart() const;
    void setStart(unsigned long long start);

    unsigned long long getSize() const;
    void setSize(unsigned long long size);

    unsigned long long getCapacity() const;
    void setCapacity(unsigned long long capacity);

    unsigned long long getClock() const;
    void setClock(unsigned long long clock);

    unsigned int getWidth() const;
    void setWidth(unsigned int width);

    string getSlot() const;
    void setSlot(const string & slot);

    unsigned int countChildren(hw::hwClass c = hw::generic) const;
    hwNode * getChild(unsigned int);
    hwNode * getChildByPhysId(long);
    hwNode * getChildByPhysId(const string &);
    hwNode * getChild(const string & id);
    hwNode * findChildByHandle(const string & handle);
    hwNode * findChildByLogicalName(const string & handle);
    hwNode * findChildByBusInfo(const string & businfo);
    hwNode * findChildByResource(const hw::resource &);
    hwNode * findChild(bool(*matchfunction)(const hwNode &));
    hwNode * addChild(const hwNode & node);
    bool isBus() const
    {
      return countChildren()>0;
    }

    bool isCapable(const string & feature) const;
    void addCapability(const string & feature, const string & description = "");
    void describeCapability(const string & feature, const string & description);
    string getCapabilities() const;
    vector<string> getCapabilitiesList() const;
    string getCapabilityDescription(const string & feature) const;

    void attractHandle(const string & handle);

    void setConfig(const string & key, const string & value);
    void setConfig(const string & key, unsigned long long value);
    string getConfig(const string & key) const;
    vector<string> getConfigKeys() const;
    vector<string> getConfigValues(const string & separator = "") const;

    vector<string> getLogicalNames() const;
    string getLogicalName() const;
    void setLogicalName(const string &);

    string getDev() const;
    void setDev(const string &);

    string getBusInfo() const;
    void setBusInfo(const string &);

    string getPhysId() const;
    void setPhysId(long);
    void setPhysId(unsigned, unsigned);
    void setPhysId(unsigned, unsigned, unsigned);
    void setPhysId(const string &);
    void assignPhysIds();

    void addResource(const hw::resource &);
    bool usesResource(const hw::resource &) const;
    vector<string> getResources(const string & separator = "") const;

    void addHint(const string &, const hw::value &);
    hw::value getHint(const string &) const;
    vector<string> getHints() const;

    void merge(const hwNode & node);

    void fixInconsistencies();

    string asXML(unsigned level = 0);
    string asJSON(unsigned level = 0);
    string asString();

    bool dump(const string & filename, bool recurse = true);
  private:

    void setId(const string & id);

    bool attractsHandle(const string & handle) const;
    bool attractsNode(const hwNode & node) const;

    struct hwNode_i * This;
};
#endif
