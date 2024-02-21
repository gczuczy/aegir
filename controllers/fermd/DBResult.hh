
#ifndef AEGIR_FERMD_DB_RESULT
#define AEGIR_FERMD_DB_RESULT

#include <string>

#include <sqlite3.h>

#include "common/Exception.hh"
#include "uuid.hh"
#include "DBTypes.hh"
#include "DBResult.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      class Statement;
      class Result {
	friend Statement;
      protected:
	Result(sqlite3_stmt* _stmt);
      public:
	Result()=delete;
	Result(const Result&)=delete;
	Result(Result&&);
	~Result();

	inline Result& operator++() {
	  sqlite3_step(c_statement);
	  return *this;
	};
	inline operator bool() const {
	  return sqlite3_stmt_busy(c_statement) == 1;
	}

	bool isNull(const std::string& _name);
	bool isNull(int _idx);

	template<typename T>
	T fetch(int _idx) {
	  static_assert(sizeof(T)!=0, "Not implemented for type");
	};

	template<>
	bool fetch(int _idx);
	template<>
	int fetch(int _idx);
	template<>
	std::string fetch(int _idx);
	template<>
	float fetch(int _idx);
	template<>
	uuid_t fetch(int _idx);

	template<typename T>
	T fetch(const std::string& _name) {
	  for (int i=0; i<=sqlite3_data_count(c_statement); ++i ) {
	    if ( _name == sqlite3_column_name(c_statement, i) )
	      return fetch<T>(i);
	  }
	  throw Exception("Column \"%s\" not found", _name.c_str());
	};
      private:
	sqlite3_stmt *c_statement;
      }; // class Result
    } // ns DB
  } // ns fermd
} // ns aegir

#endif

