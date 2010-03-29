#ifndef _DB_H_
#define _DB_H_

#include <string>
#include <iostream>

namespace sqlite {

class database
{
  public:

	database(const std::string & filename);
	database(const database & db);
	~database();

	database operator =(const database &);

	void execute(const std::string & sql);

	friend class statement;

  private:
	database();

	struct database_i *implementation;
};

typedef enum {
	null,
	text,
	integer,
	real,
	blob} type;

class value
{
  public:

	value();
	value(const char *);
	value(const std::string &);
	value(long long);
	value(double);
	value(const value &);
	~value();

	value & operator =(const value &);

	type getType() const;
	std::string asText() const;
	long long asInteger() const;
	double asReal() const;

	std::ostream & print(std::ostream &) const;

  private:
	struct value_i *implementation;
};

class statement
{
  public:

	statement(database &, const std::string &);
	~statement();

	void prepare(const std::string &);

	void bind(int index, const value & v);
	bool step();
	void execute();
	void reset();

	int columns() const;
	value column(unsigned int) const;
	value column(const std::string &) const;

	value operator[](unsigned int) const;
	value operator[](const std::string &) const;
	
  private:

	statement();
	statement(const statement &);
	statement & operator =(const statement &);

	struct statement_i *implementation;
};

}; // namespace sqlite

#endif
