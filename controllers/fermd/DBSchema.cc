
#include "DBSchema.hh"
#include "DBStatement.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      Schema::Schema(int _version, const std::string& _script)
	: c_version(_version) {
	std::string delim(";");
	std::string::size_type prev=0;
	while ( true ) {
	  auto at = _script.find(delim, prev);
	  auto size = (at==_script.npos?_script.size():at)-prev+1;
	  if ( at == _script.npos && size < 5 ) break;

	  auto cmd = _script.substr(prev, size);
#if 0
	  printf("Command(%zu -> %zu: %zu):\n%s\n---",
		 prev, at, size, cmd.c_str());
#endif
	  c_commands.emplace_back(cmd);

	  if ( at == _script.npos ) break;
	  prev = at + delim.length();
	}
      }

      Schema::~Schema() {
      }

      void Schema::apply(sqlite3* _db) {
	// apply the creation statements
	Statement(_db, "BEGIN EXCLUSIVE", true).execute();
	for (auto it: c_commands) {
	  Statement(_db, it, true).execute();
	}
	Statement(_db, "COMMIT", true).execute();
      }
    } // ns DB
  } // ns fermd
} // ns aegir

