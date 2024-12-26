
#ifndef AEGIR_FERMD_DB_RESULT
#define AEGIR_FERMD_DB_RESULT

#include <string>
#include <map>

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

	inline bool hasField(const std::string& _name) {
	  return c_fields.find(_name) != c_fields.end();
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
	  const auto& it = c_fields.find(_name);
	  if ( it == c_fields.end() )
	    throw Exception("Field \"%s\" not found", _name.c_str());
	  return fetch<T>(it->second);
	};
      private:
	sqlite3_stmt *c_statement;
	std::map<std::string, int> c_fields;
      }; // class Result
    } // ns DB
  } // ns fermd
} // ns aegir

#endif

