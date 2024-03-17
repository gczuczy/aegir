
#include "DBResult.hh"
#include "DB.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      Result::Result(sqlite3_stmt* _stmt): c_statement(_stmt) {
	int rc;
	rc = sqlite3_step(c_statement);
	evalrc(rc, c_statement);

	// cache column names
	for (int i=0; i<sqlite3_data_count(c_statement); ++i ) {
	  c_fields[sqlite3_column_name(c_statement, i)] = i;
	}
      }

      Result::Result(Result&& _other) {
	c_statement = std::move(_other.c_statement);
	c_fields = std::move(_other.c_fields);
	_other.c_statement = 0;
      }

      Result::~Result() {
	if ( c_statement ) {
	  sqlite3_clear_bindings(c_statement);
	  int rc = sqlite3_reset(c_statement);
	  evalrc(rc, c_statement);
	}
      }

      bool Result::isNull(const std::string& _name) {
	for (int i=0; i<=sqlite3_data_count(c_statement); ++i ) {
	  if ( _name == sqlite3_column_name(c_statement, i) )
	    return isNull(i);
	}
	throw Exception("Column \"%s\" not found", _name.c_str());
      }

      bool Result::isNull(int _idx) {
	return sqlite3_column_type(c_statement, _idx) == SQLITE_NULL;
      }

      template<>
      bool Result::fetch(int _idx) {
	return sqlite3_column_int(c_statement, _idx) != 0;
      }

      template<>
      int Result::fetch(int _idx) {
	return sqlite3_column_int(c_statement, _idx);
      }

      template<>
      std::string Result::fetch(int _idx) {
	const char* text = (const char*)sqlite3_column_text(c_statement, _idx);
	int len = sqlite3_column_bytes(c_statement, _idx);
	return std::string(text, len);
      }

      template<>
      float Result::fetch(int _idx) {
	return sqlite3_column_double(c_statement, _idx);
      }

      template<>
      uuid_t Result::fetch(int _idx) {
	const char* text = (const char*)sqlite3_column_text(c_statement, _idx);
	int len = sqlite3_column_bytes(c_statement, _idx);
	return g_uuidstrgen(std::string(text, len));
      }

    }
  }
}
