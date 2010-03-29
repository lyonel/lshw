#ifndef _DUMP_H_

#include "db.h"
#include "hw.h"

bool dump(hwNode &, sqlite::database &, const std::string & path = "", bool recurse = true);

#endif
