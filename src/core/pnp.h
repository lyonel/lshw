#ifndef _PNP_H_
#define _PNP_H_

#include "hw.h"
#include <string>

const char * vendorname(const char * id);

hw::hwClass pnp_class(const string & pnpid);
#endif
