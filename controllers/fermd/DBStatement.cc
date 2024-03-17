
#include "DBStatement.hh"
#include "DB.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      Statement::Statement(sqlite3 *_db,
			   const std::string& _stmt,
			   bool _temporary): c_statement(0) {
	int flags = 0;
	if ( ! _temporary ) flags |= SQLITE_PREPARE_PERSISTENT;
	int rc = sqlite3_prepare_v3(_db, _stmt.c_str(), _stmt.size(),
				    flags,
				    &c_statement, 0);
	evalrc(rc, c_statement);
	if ( rc != SQLITE_OK )
	  throw Exception("Error while preparing statement(%i): %s",
			  rc, _stmt.c_str());
      }

      Statement& Statement::operator=(Statement&& other) {
	c_statement = std::move(other.c_statement);
	return *this;
      }

      Statement::~Statement() {
	if ( c_statement ) {
	  sqlite3_finalize(c_statement);
	}
      }

      Result Statement::execute() {
	return Result(c_statement);
      }

      void Statement::bind(const std::string& _field) {
	int idx = sqlite3_bind_parameter_index(c_statement, _field.c_str());
	if ( idx == 0 )
	  throw Exception("Parameter %s not found for query %s",
			  _field.c_str(), sqlite3_sql(c_statement));
	sqlite3_bind_null(c_statement, idx);
      }

      void Statement::bind(const std::string& _field, int _value) {
	int idx = sqlite3_bind_parameter_index(c_statement, _field.c_str());
	if ( idx == 0 )
	  throw Exception("Parameter %s not found for query %s",
			  _field.c_str(), sqlite3_sql(c_statement));
	sqlite3_bind_int(c_statement, idx, _value);
      }

      void Statement::bind(const std::string& _field, float _value) {
	int idx = sqlite3_bind_parameter_index(c_statement, _field.c_str());
	if ( idx == 0 )
	  throw Exception("Parameter %s not found for query %s",
			  _field.c_str(), sqlite3_sql(c_statement));
	sqlite3_bind_double(c_statement, idx, _value);
      }

      void Statement::bind(const std::string& _field,
			   const std::string& _value) {
	int idx = sqlite3_bind_parameter_index(c_statement, _field.c_str());
	if ( idx == 0 )
	  throw Exception("Parameter %s not found for query %s",
			  _field.c_str(), sqlite3_sql(c_statement));
	sqlite3_bind_text(c_statement, idx,
			  _value.c_str(), _value.size(),
			  SQLITE_STATIC);
      }
    } // ns DB
  } // ns fermd
} // ns aegir

