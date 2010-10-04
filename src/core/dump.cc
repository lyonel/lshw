#include "dump.h"
#include "version.h"
#include "osutils.h"

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef SQLITE

using namespace std;
using namespace sqlite;

static bool createtables(database & db)
{
  try {
    db.execute("CREATE TABLE IF NOT EXISTS META(key TEXT PRIMARY KEY COLLATE NOCASE, value BLOB)");
    statement stm(db, "INSERT OR IGNORE INTO META (key,value) VALUES(?,?)");

    stm.bind(1, "schema");
    stm.bind(2, 1.0);
    stm.execute();

    stm.reset();
    stm.bind(1, "application");
    stm.bind(2, "org.ezix.lshw");
    stm.execute();

    stm.reset();
    stm.bind(1, "creator");
    stm.bind(2, "lshw/" + string(getpackageversion()));
    stm.execute();

    stm.reset();
    stm.bind(1, "OS");
    stm.bind(2, operating_system());
    stm.execute();

    stm.reset();
    stm.bind(1, "platform");
    stm.bind(2, platform());
    stm.execute();

    db.execute("CREATE TABLE IF NOT EXISTS nodes(path TEXT PRIMARY KEY, id TEXT NOT NULL COLLATE NOCASE, parent TEXT COLLATE NOCASE, class TEXT NOT NULL COLLATE NOCASE, enabled BOOL, claimed BOOL, description TEXT, vendor TEXT, product TEXT, version TEXT, serial TEXT, businfo TEXT, physid TEXT, slot TEXT, size INTEGER, capacity INTEGER, clock INTEGER, width INTEGER, dev TEXT)");
    db.execute("CREATE TABLE IF NOT EXISTS logicalnames(logicalname TEXT NOT NULL, node TEXT NOT NULL COLLATE NOCASE)");
    db.execute("CREATE TABLE IF NOT EXISTS capabilities(capability TEXT NOT NULL COLLATE NOCASE, node TEXT NOT NULL COLLATE NOCASE, description TEXT, UNIQUE (capability,node))");
    db.execute("CREATE TABLE IF NOT EXISTS configuration(config TEXT NOT NULL COLLATE NOCASE, node TEXT NOT NULL COLLATE NOCASE, value TEXT, UNIQUE (config,node))");
    db.execute("CREATE TABLE IF NOT EXISTS hints(hint TEXT NOT NULL COLLATE NOCASE, node TEXT NOT NULL COLLATE NOCASE, value TEXT, UNIQUE (hint,node))");
    db.execute("CREATE TABLE IF NOT EXISTS resources(node TEXT NOT NULL COLLATE NOCASE, type TEXT NOT NULL COLLATE NOCASE, resource TEXT NOT NULL, UNIQUE(node,type,resource))");
    db.execute("CREATE VIEW IF NOT EXISTS unclaimed AS SELECT * FROM nodes WHERE NOT claimed");
    db.execute("CREATE VIEW IF NOT EXISTS disabled AS SELECT * FROM nodes WHERE NOT enabled");
  }
  catch(exception & e)
  {
    return false;
  }

  return true;
}

bool dump(hwNode & n, database & db, const string & path, bool recurse)
{
  if(!createtables(db))
    return false;

  try {
    unsigned i = 0;
    statement stm(db, "INSERT OR REPLACE INTO nodes (id,class,product,vendor,description,size,capacity,width,version,serial,enabled,claimed,slot,clock,businfo,physid,path,parent,dev) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
    string mypath = path+(path=="/"?"":"/")+n.getPhysId();

    stm.bind(1, n.getId());
    stm.bind(2, n.getClassName());
    if(n.getProduct() != "") stm.bind(3, n.getProduct());
    if(n.getVendor() != "") stm.bind(4, n.getVendor());
    if(n.getDescription() != "") stm.bind(5, n.getDescription());
    if(n.getSize()) stm.bind(6, (long long int)n.getSize());
    if(n.getCapacity()) stm.bind(7, (long long int)n.getCapacity());
    if(n.getWidth()) stm.bind(8, (long long int)n.getWidth());
    if(n.getVersion() != "") stm.bind(9, n.getVersion());
    if(n.getSerial() != "") stm.bind(10, n.getSerial());
    stm.bind(11, (long long int)n.enabled());
    stm.bind(12, (long long int)n.claimed());
    if(n.getSlot() != "") stm.bind(13, n.getSlot());
    if(n.getClock()) stm.bind(14, (long long int)n.getClock());
    if(n.getBusInfo() != "") stm.bind(15, n.getBusInfo());
    if(n.getPhysId() != "") stm.bind(16, n.getPhysId());
    stm.bind(17, mypath);
    if(path != "") stm.bind(18, path);
    if(n.getDev() != "") stm.bind(19, n.getDev());
    stm.execute();

    stm.prepare("INSERT OR REPLACE INTO logicalnames (node,logicalname) VALUES(?,?)");
    vector<string> keys = n.getLogicalNames();
    for(i=0; i<keys.size(); i++)
    {
      stm.reset();
      stm.bind(1, mypath);
      stm.bind(2, keys[i]);
      stm.execute();
    }

    stm.prepare("INSERT OR REPLACE INTO capabilities (capability,node,description) VALUES(?,?,?)");
    keys = n.getCapabilitiesList();
    for(i=0; i<keys.size(); i++)
    {
      stm.reset();
      stm.bind(1, keys[i]);
      stm.bind(2, mypath);
      stm.bind(3, n.getCapabilityDescription(keys[i]));
      stm.execute();
    }
    
    stm.prepare("INSERT OR REPLACE INTO configuration (config,node,value) VALUES(?,?,?)");
    keys = n.getConfigKeys();
    for(i=0; i<keys.size(); i++)
    {
      stm.reset();
      stm.bind(1, keys[i]);
      stm.bind(2, mypath);
      stm.bind(3, n.getConfig(keys[i]));
      stm.execute();
    }

    stm.prepare("INSERT OR IGNORE INTO resources (type,node,resource) VALUES(?,?,?)");
    keys = n.getResources(":");
    for(i=0; i<keys.size(); i++)
    {
      string type = keys[i].substr(0, keys[i].find_first_of(':'));
      string resource = keys[i].substr(keys[i].find_first_of(':')+1);
      stm.reset();
      stm.bind(1, type);
      stm.bind(2, mypath);
      stm.bind(3, resource);
      stm.execute();
    }

    stm.prepare("INSERT OR REPLACE INTO hints (hint,node,value) VALUES(?,?,?)");
    keys = n.getHints();
    for(i=0; i<keys.size(); i++)
    {
      stm.reset();
      stm.bind(1, keys[i]);
      stm.bind(2, mypath);
      stm.bind(3, n.getHint(keys[i]).asString());
      stm.execute();
    }

    stm.reset();
    stm.bind(1,"run.root");
    stm.bind(2,"");
    stm.bind(3,(long long int)(geteuid() == 0));
    stm.execute();
    stm.reset();
    stm.bind(1,"run.time");
    stm.bind(2,"");
    stm.bind(3,(long long int)time(NULL));
    stm.execute();
    stm.reset();
    stm.bind(1,"run.language");
    stm.bind(2,"");
    stm.bind(3,getenv("LANG"));
    stm.execute();

    if(recurse)
      for(i=0; i<n.countChildren(); i++)
        dump(*(n.getChild(i)), db, mypath, recurse);
  }
  catch(exception & e)
  {
    return false;
  }

  return true;
}

#endif
