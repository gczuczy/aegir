
#ifndef AEGIR_FERMD_DB_SCHEMA
#define AEGIR_FERMD_DB_SCHEMA

#include <string>
#include <list>

#include <sqlite3.h>

namespace aegir {
  namespace fermd {
    namespace DB {
      class Schema {
      public:
	Schema(int _version, const std::string& _script);
	~Schema();

      public:
	void apply(sqlite3* _db);
	inline int version() const {return c_version;};

      private:
	int c_version;
	std::list<std::string> c_commands;
      };
    } // ns DB
  } // ns fermd
} // ns aegir

#endif

