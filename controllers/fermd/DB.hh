/*
  SQLite DB connection
 */

#ifndef AEGIR_FERMD_DB
#define AEGIR_FERMD_DB

#include <sqlite3.h>

#include "DBTypes.hh"
#include "DBTransaction.hh"
#include "DBStatement.hh"
#include "DBResult.hh"
#include "DBSchema.hh"
#include "DBConnection.hh"

namespace aegir {
  namespace fermd {
    namespace DB {

      bool evalrc(int rc, sqlite3* db);
      bool evalrc(int rc, sqlite3_stmt* stmt);

    }
  }
}

#endif
