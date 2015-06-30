#ifndef _PNP_H_
#define _PNP_H_

#include "hw.h"
#include <string>

string pnp_vendorname(const string & id);

hw::hwClass pnp_class(const string & pnpid);

bool scan_pnp(hwNode &);
#endif
