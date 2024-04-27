
#ifndef AEGIR_FERMD_DB_STATEMENT
#define AEGIR_FERMD_DB_STATEMENT

#include <string>

#include <sqlite3.h>
#include <optional>

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

	Statement& bind(const std::string& _field);
	Statement& bind(const std::string& _field, int _value);
	Statement& bind(const std::string& _field, float _value);
	Statement& bind(const std::string& _field, const std::string& _value);
	template<typename T>
	Statement& bind(const std::string& _field, std::optional<T> _value) {
	  if ( !_value.has_value() ) {
	    return bind(_field);
	  }
	  return bind(_field, _value.value());
	}

      private:
	sqlite3_stmt *c_statement;
      }; // class Statement

      // The DB schemas
    } // ns DB
  } // ns fermd
} // ns aegir

#endif

