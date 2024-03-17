
#ifndef AEGIR_FERMD_DB_STATEMENT
#define AEGIR_FERMD_DB_STATEMENT

#include <string>

#include <sqlite3.h>

#include "DBResult.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      class Statement {
      public:
	Statement()=delete;
	Statement(sqlite3 *_db,
		  const std::string& _stmt, bool _temporary=false);
	Statement(const Statement&)=delete;
	Statement(Statement&&)=delete;
	Statement& operator=(Statement&&);
	~Statement();

	Result execute();

	void bind(const std::string& _field);
	void bind(const std::string& _field, int _value);
	void bind(const std::string& _field, float _value);
	void bind(const std::string& _field, const std::string& _value);

      private:
	sqlite3_stmt *c_statement;
      }; // class Statement

      // The DB schemas
    } // ns DB
  } // ns fermd
} // ns aegir

#endif

