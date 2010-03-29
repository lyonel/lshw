#include <string.h>
#include <string>
#include <stdexcept>
#include <sqlite3.h>

#include "db.h"

using namespace sqlite;
using namespace std;

namespace sqlite {

class exception : public std::runtime_error
{
public:
	exception(sqlite3 *db):
		runtime_error("SQLite error: " + string(sqlite3_errmsg(db)))
	{
	}

	exception(const char * msg):
		runtime_error(msg)
	{
	}
};

}; // namespace sqlite

struct sqlite::database_i
{
  sqlite3* connection;

  database_i()
  {
    connection = NULL;
  }
};

database::database(const std::string & filename):
	implementation(NULL)
{
  implementation = new database_i;

  if(implementation)
    if(sqlite3_open(filename.c_str(), &implementation->connection) != SQLITE_OK)
      throw exception(implementation->connection);
}

database::database(const database & db)
{
  implementation = new database_i;

  if(implementation && db.implementation)
    *implementation = *(db.implementation);
}

database::~database()
{
  if(implementation)
  {
    sqlite3_close(implementation->connection);
    delete implementation;
  }
}

void database::execute(const string & sql)
{
  if(implementation)
  {
    statement s(*this, sql);
    s.execute();
  }
}

struct sqlite::value_i
{
  unsigned int ref;
  type value_type;
  union
  {
    long long integer_value;
    double real_value;
    string *text_value;
  };

  value_i()
  {
    value_type = null;
    ref = 1;
  }

  ~value_i()
  {
    if((value_type == text) && text_value)
      delete text_value;
  }
};

value::value(const std::string & s)
{
  implementation = new value_i;

  if(implementation)
  {
    implementation->value_type = text;
    implementation->text_value = new string(s);
  }
}

value::value(const char * s)
{
  implementation = new value_i;

  if(implementation)
  {
    implementation->value_type = text;
    implementation->text_value = new string(s);
  }
}

value::value(long long l)
{
  implementation = new value_i;

  if(implementation)
  {
    implementation->value_type = integer;
    implementation->integer_value = l;
  }
}

value::value(double d)
{
  implementation = new value_i;

  if(implementation)
  {
    implementation->value_type = real;
    implementation->real_value = d;
  }
}

value::value()
{
  implementation = new value_i;
}

value::value(const value & v)
{
  implementation = v.implementation;

  if(implementation) implementation->ref++;
}

value::~value()
{
  if(implementation)
  {
    implementation->ref--;
    if(implementation->ref<=0) delete implementation;
  }
}

value & value::operator =(const value & v)
{
  if(&v == this) return *this; // self-affectation

  if(implementation)
  {
    implementation->ref--;
    if(implementation->ref<=0) delete implementation;
  }

  implementation = v.implementation;

  if(implementation) implementation->ref++;

  return *this;
}

type value::getType() const
{
  if(implementation)
    return implementation->value_type;
  else
    return null;
}

string value::asText() const
{
  if(!implementation || (implementation->value_type == null))
    throw exception("Can't convert NULL value to text");

  switch(implementation->value_type)
  {
    case text:
    case blob:
	return *(implementation->text_value);
    default:
        throw exception("Can't convert value to text");
  }

  throw exception("Can't convert value to text");
}

long long value::asInteger() const
{
  if(!implementation || (implementation->value_type == null))
    throw exception("Can't convert NULL value to integer");

  switch(implementation->value_type)
  {
    case integer:
	return implementation->integer_value;
    default:
        throw exception("Can't convert value to integer");
  }
}

double value::asReal() const
{
  if(!implementation || (implementation->value_type == null))
    throw exception("Can't convert NULL value to real");

  switch(implementation->value_type)
  {
    case real:
	return implementation->real_value;
    default:
  	throw exception("Can't convert value to real");
  }

}

ostream & value::print(std::ostream & out) const
{
  if(!implementation)
    out << "NULL";
  else
  switch(implementation->value_type)
  {
    case integer:
	out << implementation->integer_value;
	break;
    case real:
	out << implementation->real_value;
	break;
    case text:
	out << *(implementation->text_value);
	break;
    case blob:
	out << "BLOB";
	break;
    case null:
	out << "NULL";
	break;

  }

  return out;
}

struct sqlite::statement_i
{
  database *db;
  sqlite3_stmt *stmt;
};

statement::statement(database & db, const std::string & s)
{
  implementation = new statement_i;

  if(!implementation)
    throw exception("memory exhausted");

  implementation->db = &db;
  if(!db.implementation)
    throw exception("invalid database");

  if(sqlite3_prepare(db.implementation->connection, s.c_str(), -1, &implementation->stmt, NULL) != SQLITE_OK)
    throw exception(db.implementation->connection);
}

statement::~statement()
{
  if(implementation)
  {
    if(implementation->stmt)
      sqlite3_finalize(implementation->stmt);

    delete implementation;
  }
}

void statement::prepare(const std::string & s)
{
  if(implementation)
  {
    if(implementation->stmt)
      sqlite3_finalize(implementation->stmt);
  }
  else
    implementation = new statement_i;

  if(!implementation)
    throw exception("memory exhausted");

  if(sqlite3_prepare(implementation->db->implementation->connection, s.c_str(), -1, &implementation->stmt, NULL) != SQLITE_OK)
    throw exception(implementation->db->implementation->connection);
}

void statement::bind(int index, const value & v)
{
  if(!implementation)
    throw exception("undefined statement");

  switch(v.getType())
  {
    case null:
        if(sqlite3_bind_null(implementation->stmt, index) != SQLITE_OK)
	  throw exception(implementation->db->implementation->connection);
	break;
    case text:
        if(sqlite3_bind_text(implementation->stmt, index, v.asText().c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
	  throw exception(implementation->db->implementation->connection);
	break;
    case blob:
	{
          string blob = v.asText();
          if(sqlite3_bind_text(implementation->stmt, index, blob.data(), blob.size(), SQLITE_TRANSIENT) != SQLITE_OK)
	    throw exception(implementation->db->implementation->connection);
	  break;
	}
    case integer:
        if(sqlite3_bind_int64(implementation->stmt, index, v.asInteger()) != SQLITE_OK)
	  throw exception(implementation->db->implementation->connection);
	break;
    case real:
        if(sqlite3_bind_double(implementation->stmt, index, v.asReal()) != SQLITE_OK)
	  throw exception(implementation->db->implementation->connection);
	break;
    default:
	throw exception("unknown type");
  }
}

bool statement::step()
{
  if(!implementation)
    throw exception("undefined statement");

  int result = sqlite3_step(implementation->stmt);

  if(result == SQLITE_DONE) return false;
  if(result == SQLITE_ROW) return true;

  throw exception(implementation->db->implementation->connection);
}

void statement::execute()
{
  while(step());
}

void statement::reset()
{
  if(!implementation)
    throw exception("undefined statement");

  sqlite3_reset(implementation->stmt);	// ignore return value
}

int statement::columns() const
{
  if(!implementation)
    throw exception("undefined statement");

  return sqlite3_column_count(implementation->stmt);
}

value statement::column(unsigned int i) const
{
  if(!implementation)
    throw exception("undefined statement");

  switch(sqlite3_column_type(implementation->stmt, i))
  {
    case SQLITE_INTEGER:
	return value(sqlite3_column_int64(implementation->stmt, i));
    case SQLITE_FLOAT:
	return value(sqlite3_column_double(implementation->stmt, i));
    case SQLITE_TEXT:
	return value((const char*)sqlite3_column_text(implementation->stmt, i));
    case SQLITE_BLOB:
    case SQLITE_NULL:
    default:
	return value();
  }
}

value statement::column(const string & name) const
{
  if(!implementation)
    throw exception("undefined statement");

  for(int i=0; i<columns(); i++)
    if(strcasecmp(sqlite3_column_name(implementation->stmt, i), name.c_str())==0)
      return column(i);

  return value();
}

value statement::operator[](unsigned int i) const
{
  return column(i);
}

value statement::operator[](const string & i) const
{
  return column(i);
}
