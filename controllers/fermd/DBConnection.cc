
#include "DBConnection.hh"
#include "generated.hh"
#include "DB.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      static void s3log(void* _logger, int _errcode, const char* _msg) {
	LogChannel *lc = (LogChannel*)_logger;
	lc->error("SQLite3(%i) error: %s", _errcode, _msg);
      }

      Connection::Connection(): ConfigNode(), c_dbfile("/var/db/aegir/fermd.db"), c_db(0),
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

      Connection::~Connection() {
	c_statements.clear();
	if ( c_db ) sqlite3_close_v2(c_db);
      }

      std::shared_ptr<Connection> Connection::getInstance() {
	static std::shared_ptr<Connection> instance{new Connection()};
	return instance;
      }

      void Connection::marshall(ryml::NodeRef& _node) {
	_node |= ryml::MAP;
	auto tree = _node.tree();

	// the Connection file
	{
	  auto csubstr = tree->to_arena(c_dbfile);
	  _node["file"] = csubstr;
	}
      }

      void Connection::unmarshall(ryml::ConstNodeRef& _node) {
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

      void Connection::init() {
	if ( c_db ) return;

	// open the dbconn
	int rc;
	c_logger.info("Opening Connection file %s", c_dbfile.c_str());
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
	  c_logger.info("globals not found, initializing Connection");
	  for (auto it: c_schemas) it.apply(c_db);
	} else {
	  int version = Statement(c_db,
				  "SELECT CAST(value AS int) as version "
				  "FROM globals "
				  "WHERE name == 'version'", true)
	    .execute().fetch<int>("version");
	  if ( version != c_schemas.back().version() ) {
	    c_logger.error("Connection Upgrade not yet implemented");
	    throw Exception("Upgrade not yet implemented");
	  }
	}
	// prepare our statements
	prepare("begin", "BEGIN IMMEDIATE;");
	prepare("commit", "COMMIT;");
	prepare("get_tilthydrometers",
		"SELECT id,color,uuid,active,enabled,fermenterid,"
		"calibr_null,calibr_at,calibr_sg "
		"FROM tilthydrometers");
	prepare("set_tilthydrometer",
		"UPDATE tilthydrometers "
		"SET active=:active,enabled=:enabled,"
		"fermenterid=:fermenterid,calibr_null=:calibrnull,"
		"calibr_at=:calibrat,calibr_sg=:calibrsg "
		"WHERE id=:id "
		"RETURNING id,active,enabled,fermenterid,"
		"calibr_null,calibr_at,calibr_sg");

	reload_tilthydrometers();
      } // init

      void Connection::setConnectionFile(const std::string& _file) {
	if ( c_db )
	  throw Exception("Cannot set dbfile while db is open");
	c_dbfile = _file;
      }

      void Connection::reload_tilthydrometers() {
	std::unique_lock g(c_mtx_tilthydrometers);

	cache_tilthydrometers.clear();
	for ( auto r=c_statements.find("get_tilthydrometers")->second.execute();
	      r; ++r ) {
	  auto th = std::make_shared<tilthydrometer>();
	  th->id = r.fetch<int>("id");
	  th->color = r.fetch<std::string>("color");
	  th->uuid = r.fetch<uuid_t>("uuid");
	  th->active = r.fetch<bool>("active");
	  th->enabled = r.fetch<bool>("enabled");

	  if ( !r.isNull("calibr_null") ) {
	    auto c = std::make_shared<tilthydrometer::calibration>();
	    c->sg = r.fetch<float>("calibr_null");
	    th->calibr_null = c;
	  }

	  if ( !r.isNull("calibr_at") && !r.isNull("calibr_sg") ) {
	    auto c = std::make_shared<tilthydrometer::calibration>();
	    c->at = r.fetch<float>("calibr_at");
	    c->sg = r.fetch<float>("calibr_sg");
	    th->calibr_sg = c;
	  }
	  cache_tilthydrometers.emplace_back(th);
	}
      } // reload_tilthydrometers

      tilthydrometer_cdb Connection::getTilthydrometers() const {
	std::shared_lock g(c_mtx_tilthydrometers);
	std::list<std::shared_ptr<const tilthydrometer> > ret;
	for (auto it: cache_tilthydrometers)
	  ret.emplace_back(std::shared_ptr<const tilthydrometer>(it.get()));
	return ret;
      } // getTilthydrometers

      tilthydrometer::cptr Connection::getTilthydrometerByUUID(uuid_t _uuid) const {
	std::shared_lock g(c_mtx_tilthydrometers);
	for (auto it: cache_tilthydrometers)
	  if ( it->uuid == _uuid) return tilthydrometer::cptr(it.get());
	return nullptr;
      }

      void Connection::setTilthydrometer(const tilthydrometer& _item) {
	std::unique_lock g(c_mtx_tilthydrometers);
	auto& stmt(c_statements.find("set_tilthydrometer")->second);
	stmt.bind(":id", _item.id);
	stmt.bind(":active", _item.active?1:0);
	stmt.bind(":enabled", _item.enabled?1:0);

	if ( _item.calibr_null ) {
	  stmt.bind(":calibrnull", _item.calibr_null->sg);
	} else {
	  stmt.bind(":calibrnull");
	}

	if ( _item.calibr_sg ) {
	  stmt.bind(":calibrat", _item.calibr_sg->at);
	  stmt.bind(":calibrsg", _item.calibr_sg->sg);
	} else {
	  stmt.bind(":calibrat");
	  stmt.bind(":calibrsg");
	}

	// TODO: add fermenters

	// run the query
	auto r(stmt.execute());

	// update the cache entry
	int id = r.fetch<int>("id");
	for (auto& th: cache_tilthydrometers ) {
	  if ( th->id == id ) {
	    th->active = r.fetch<bool>("active");
	    th->enabled = r.fetch<bool>("enabled");

	    if ( !r.isNull("calibr_null") ) {
	      auto c = std::make_shared<tilthydrometer::calibration>();
	      c->sg = r.fetch<float>("calibr_null");
	      th->calibr_null = c;
	    }

	    if ( !r.isNull("calibr_at") && !r.isNull("calibr_sg") ) {
	      auto c = std::make_shared<tilthydrometer::calibration>();
	      c->at = r.fetch<float>("calibr_at");
	      c->sg = r.fetch<float>("calibr_sg");
	      th->calibr_sg = c;
	    }
	    // add fermenters
	  }
	}
      }

      void Connection::begin() {
	c_mtx.lock();
      }

      void Connection::commit() {
	c_mtx.unlock();
      }

      void Connection::prepare(const std::string& _name,
			       const std::string& _stmt,
			       bool _temporary) {
	if ( c_statements.find(_name) != c_statements.end() )
	  throw Exception("Prepared satement %s already exists", _name.c_str());

	c_statements.emplace(std::piecewise_construct,
			     std::forward_as_tuple(_name),
			     std::forward_as_tuple(c_db, _stmt, _temporary));
      } // prepare

    } // ns DB
  } // ns fermd
} // ns aegir
