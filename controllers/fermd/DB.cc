#include "DB.hh"

#include <utility>

#include "generated.hh"

namespace aegir {
  namespace fermd {

    static void s3log(void* _logger, int _errcode, const char* _msg) {
      LogChannel *lc = (LogChannel*)_logger;
      lc->error("SQLite3(%i) error: %s", _errcode, _msg);
    }

    // evaluates SQLite3 return codes
    // return true indicates it should be retried
    static bool evalrc(int rc, sqlite3* db) {

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
    static bool evalrc(int rc, sqlite3_stmt* stmt) {
      return evalrc(rc, sqlite3_db_handle(stmt));
    }
    /*
      DB::Statement::Result
     */
    DB::Statement::Result::Result(sqlite3_stmt* _stmt): c_statement(_stmt) {
      int rc;
      rc = sqlite3_step(c_statement);
      evalrc(rc, c_statement);
    }

    DB::Statement::Result::Result(Result&& _other) {
      c_statement = std::move(_other.c_statement);
    }

    DB::Statement::Result::~Result() {
    }

    template<>
    bool DB::Statement::Result::fetch(int _idx) {
      return sqlite3_column_int(c_statement, _idx) != 0;
    }

    template<>
    int DB::Statement::Result::fetch(int _idx) {
      return sqlite3_column_int(c_statement, _idx);
    }
    /*
      DB::Statement
     */
    DB::Statement::Statement(sqlite3 *_db,
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

    DB::Statement& DB::Statement::operator=(Statement&& other) {
      c_statement = std::move(other.c_statement);
      return *this;
    }

    DB::Statement::~Statement() {
      if ( c_statement ) sqlite3_finalize(c_statement);
    }

    DB::Statement::Result DB::Statement::execute() {
      int rc = sqlite3_reset(c_statement);
      evalrc(rc, c_statement);

      return Result(c_statement);
    }

    /*
      Schema
     */
    DB::Schema::Schema(int _version, const std::string& _script)
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

    DB::Schema::~Schema() {
    }

    void DB::Schema::apply(sqlite3* _db) {
      // apply the creation statements
      Statement(_db, "BEGIN EXCLUSIVE", true).execute();
      for (auto it: c_commands) {
	Statement(_db, it, true).execute();
      }
      Statement(_db, "COMMIT", true).execute();
    }

    /*
      DB
     */
    DB::DB(): ConfigNode(), c_dbfile("/var/db/aegir/fermd.db"), c_db(0),
	      c_logger("db") {

      if ( sqlite3_config(SQLITE_CONFIG_SERIALIZED) != SQLITE_OK ) {
	c_logger.warn("Cannot set SQLITE_CONFIG_SERIALIZED");
      }

      if ( sqlite3_config(SQLITE_CONFIG_LOG, &s3log, (void*)&c_logger)
	   != SQLITE_OK ) {
	c_logger.warn("Unable to set SQLITE_CONFIG_LOG");
      }

      // load the schemas
      c_schemas.emplace_back(Schema(1,
				    std::string((char*)sql_v1_sql,
						sql_v1_sql_len)));

    }

    DB::~DB() {
      c_statements.clear();
      if ( c_db ) sqlite3_close_v2(c_db);
    }

    std::shared_ptr<DB> DB::getInstance() {
      static std::shared_ptr<DB> instance{new DB()};
      return instance;
    }

    void DB::marshall(ryml::NodeRef& _node) {
      _node |= ryml::MAP;
      auto tree = _node.tree();

      // the DB file
      {
	auto csubstr = tree->to_arena(c_dbfile);
	_node["file"] = csubstr;
      }
    }

    void DB::unmarshall(ryml::ConstNodeRef& _node) {
      if ( c_db ) {
	c_logger.warn("Cannot unmarshall config: db already open");
	return;
      }

      if ( ! _node.is_map() )
	throw Exception("db node is not a map");

      if ( _node.has_child("file") ) {
	_node["file"] >> c_dbfile;
      }
    }

    void DB::init() {
      if ( c_db ) return;

      // open the dbconn
      int rc;
      c_logger.info("Opening DB file %s", c_dbfile.c_str());
      rc = sqlite3_open_v2(c_dbfile.c_str(),
			   &c_db,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
			   0);
      evalrc(rc, c_db);

      Statement(c_db, "PRAGMA foreign_keys = ON", true)
	.execute();
      Statement(c_db, "PRAGMA journal_mode = MEMORY", true)
	.execute();
      Statement(c_db, "PRAGMA temp_store = MEMORY", true)
	.execute();
      Statement(c_db, "PRAGMA trusted_schema = ON", true)
	.execute();

      // prepare queries
      bool hasglobals = Statement(c_db,
				  "SELECT count(*) AS count FROM sqlite_schema "
				  "WHERE type = 'table' AND name ='globals'", true)
	.execute()
	.fetch<bool>("count");

      if ( !hasglobals ) {
	c_logger.info("globals not found, initializing DB");
	for (auto it: c_schemas) it.apply(c_db);
      } else {
	int version = Statement(c_db,
				"SELECT CAST(value AS int) as version "
				"FROM globals "
				"WHERE name == 'version'", true)
	  .execute().fetch<int>("version");
	if ( version != c_schemas.back().version() ) {
	  c_logger.error("DB Upgrade not yet implemented");
	  throw Exception("Upgrade not yet implemented");
	}
      }
      /*
      */
    }

    void DB::setDBfile(const std::string& _file) {
      if ( c_db )
	throw Exception("Cannot set dbfile while db is open");
      c_dbfile = _file;
    }

    void DB::prepare(const std::string& _name,
		     const std::string& _stmt,
		     bool _temporary) {
      if ( c_statements.find(_name) != c_statements.end() )
	throw Exception("Prepared satement %s already exists", _name.c_str());

      c_statements.emplace(std::piecewise_construct,
			   std::forward_as_tuple(_name),
			   std::forward_as_tuple(c_db, _stmt, _temporary));
    }
  }
}
