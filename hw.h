#ifndef _HW_H_
#define _HW_H_

#include <string>

namespace hw {

typedef enum {processor,
	memory,
	storage,
	system,
	bridge,
	bus,
	network,
	display,
	input,
	printer,
	multimedia,
	communication,
	generic} hwClass;

} // namespace hw

using namespace std;

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

	bool enabled() const;
	bool disabled() const;
	void enable();
	void disable();

	hw::hwClass getClass() const;
	void setClass(hw::hwClass c);

	string getVendor() const;
	void setVendor(const string & vendor);

	string getProduct() const;
	void setProduct(const string & product);

	string getVersion() const;
	void setVersion(const string & version);

	string getSerial() const;
	void setSerial(const string & serial);

	unsigned long long getSize() const;
	void setSize(unsigned long long size);

	unsigned long long getCapacity() const;
	void setCapacity(unsigned long long capacity);

	unsigned long long getClock() const;
	void setClock(unsigned long long clock);

	string getSlot() const;
	void setSlot(const string & slot);

	unsigned int countChildren() const;
	const hwNode * getChild(unsigned int) const;
	hwNode * getChild(const string & id);
	bool addChild(const hwNode & node);
	bool isBus() const
	{
	  return countChildren()>0;
	}

  private:

	void setId(const string & id);

	struct hwNode_i * This;
};

#endif
