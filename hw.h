#ifndef _HW_H_
#define _HW_H_

#include <string>

namespace hw {

typedef enum {cpu,
	memory,
	storage,
	system,
	bridge,
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
	hw::hwClass getClass() const;

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

	unsigned int countChildren() const;
	const hwNode * getChild(unsigned int) const;
	bool addChild(const hwNode & node);
	bool isBus() const
	{
	  return countChildren()>0;
	}

  private:

	struct hwNode_i * This;
};

#endif
