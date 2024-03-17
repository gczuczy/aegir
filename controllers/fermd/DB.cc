#include "DB.hh"

#include <utility>

#include "generated.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      // evaluates SQLite3 return codes
      // return true indicates it should be retried
      bool evalrc(int rc, sqlite3* db) {

	if ( rc == SQLITE_ABORT )
	  throw Exception("SQLITE_CORRUPT: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_AUTH )
	  throw Exception("SQLITE_CORRUPT: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_BUSY )
	  return true;
	else if ( rc == SQLITE_CANTOPEN )
	  throw Exception("SQLITE_CORRUPT: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_CONSTRAINT )
	  throw Exception("SQLITE_CONSTRAINT: %s",
			  sqlite3_errmsg(db));
	else if ( rc == SQLITE_CORRUPT )
	  throw Exception("SQLITE_CORRUPT: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_ERROR )
	  throw Exception("SQLITE_CORRUPT: %s: %s",
			  sqlite3_errstr(rc),
			  sqlite3_errmsg(db));
	else if ( rc == SQLITE_FULL )
	  throw Exception("SQLITE_FULL: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_INTERNAL )
	  throw Exception("SQLITE_INTERNAL: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_INTERRUPT )
	  return true;
	else if ( rc == SQLITE_IOERR )
	  throw Exception("SQLITE_IOERR: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_LOCKED )
	  return true;
	else if ( rc == SQLITE_MISMATCH )
	  throw Exception("SQLITE_MISMATCH: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_MISUSE )
	  throw Exception("SQLITE_MISUSE: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_NOLFS )
	  throw Exception("SQLITE_NOLFS: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_NOMEM )
	  throw Exception("SQLITE_NOMEM: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_NOTADB )
	  throw Exception("SQLITE_NOTADB: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_NOTFOUND )
	  throw Exception("SQLITE_NOTFOUND: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_PERM )
	  throw Exception("SQLITE_PERM: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_PROTOCOL )
	  throw Exception("SQLITE_PROTOCOL: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_RANGE )
	  throw Exception("SQLITE_RANGE: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_READONLY )
	  throw Exception("SQLITE_READONLY: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_SCHEMA )
	  throw Exception("SQLITE_SCHEMA: %s", sqlite3_errstr(rc));
	else if ( rc == SQLITE_TOOBIG )
	  throw Exception("SQLITE_TOOBIG: %s", sqlite3_errstr(rc));

	return false;
      }

      bool evalrc(int rc, sqlite3_stmt* stmt) {
	return evalrc(rc, sqlite3_db_handle(stmt));
      }

    }
  }
}
