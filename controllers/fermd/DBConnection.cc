
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

      Connection::Connection(): ConfigNode(), Service(), LogChannel("DB"),
				c_dbfile("/var/db/aegir/fermd.db"), c_db(0) {

	if ( sqlite3_config(SQLITE_CONFIG_SERIALIZED) != SQLITE_OK ) {
	  warn("Cannot set SQLITE_CONFIG_SERIALIZED");
	}

	if ( sqlite3_config(SQLITE_CONFIG_LOG, &s3log, (void*)this)
	     != SQLITE_OK ) {
	  warn("Unable to set SQLITE_CONFIG_LOG");
	}

	// load the schemas
	c_schemas.emplace_back(Schema(1,
				      std::string((char*)sql_v1_sql,
						  sql_v1_sql_len)));
      }

      Connection::~Connection() {
	c_statements.clear();
	if ( c_db ) {
	  info("Closing database");
	  sqlite3_close_v2(c_db);
	}
	sqlite3_config(SQLITE_CONFIG_LOG, 0, 0);
      }

      void Connection::bailout() {
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
	  warn("Cannot unmarshall config: db already open");
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
	info("Opening Connection file %s", c_dbfile.c_str());
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
	  info("globals not found, initializing Connection");
	  for (auto it: c_schemas) it.apply(c_db);
	} else {
	  int version = Statement(c_db,
				  "SELECT CAST(value AS int) as version "
				  "FROM globals "
				  "WHERE name == 'version'", true)
	    .execute().fetch<int>("version");
	  if ( version != c_schemas.back().version() ) {
	    error("Connection Upgrade not yet implemented");
	    throw Exception("Upgrade not yet implemented");
	  }
	}
	// prepare our statements
	prepare("begin", "BEGIN IMMEDIATE;");
	prepare("commit", "COMMIT;");
	prepare("get_tilthydrometers",
		"SELECT id,color,uuid,enabled,fermenterid,"
		"calibr_null,calibr_at,calibr_sg "
		"FROM tilthydrometers");
	prepare("set_tilthydrometer",
		"UPDATE tilthydrometers "
		"SET enabled=:enabled,"
		"fermenterid=:fermenterid,calibr_null=:calibrnull,"
		"calibr_at=:calibrat,calibr_sg=:calibrsg "
		"WHERE id=:id "
		"RETURNING id,enabled,fermenterid,"
		"calibr_null,calibr_at,calibr_sg");

	// fermenter types
	prepare("get_fermenter_types",
		"SELECT id,name,capacity,imageurl "
		"FROM fermenter_types");
	prepare("update_fermenter_types",
		"UPDATE fermenter_types "
		"SET name=:name,capacity=:capacity,imageurl=:imageurl "
		"WHERE id=:id "
		"RETURNING id,name,capacity,imageurl");
	prepare("insert_fermenter_types",
		"INSERT INTO fermenter_types "
		"(name, capacity, imageurl) "
		"VALUES (:name,:capacity,:imageurl) "
		"RETURNING id,name,capacity,imageurl");
	prepare("delete_fermenter_types",
		"DELETE FROM fermenter_types WHERE id=:id ");

	// fermenters
	prepare("get_fermenters",
		"SELECT id,name,typeid FROM fermenters");
	prepare("update_fermenters",
		"UPDATE fermenters "
		"SET name=:name,typeid=:typeid "
		"WHERE id=:id "
		"RETURNING id,name,typeid");
	prepare("insert_fermenters",
		"INSERT INTO fermenters "
		"(name, typeid) "
		"VALUES (:name,:typeid) "
		"RETURNING id,name,typeid");
	prepare("delete_fermenters",
		"DELETE FROM fermenters WHERE id=:id ");

	reload();
      } // init

      void Connection::setConnectionFile(const std::string& _file) {
	if ( c_db )
	  throw Exception("Cannot set dbfile while db is open");
	c_dbfile = _file;
      }

      void Connection::reload() {
	reload_fermenter_types();
	reload_fermenters();
	reload_tilthydrometers();
      }

      void Connection::reload_fermenter_types() {
	std::unique_lock g(c_mtx_fermenter_types);

	cache_fermenter_types.clear();
	for ( auto r=c_statements.find("get_fermenter_types")->second.execute();
	      r; ++r) {
	  auto ft = std::make_shared<fermenter_types>();
	  *ft = r;
	  cache_fermenter_types.emplace_back(ft);
	}
      }

      void Connection::reload_fermenters() {
	std::unique_lock g(c_mtx_fermenters);

	cache_fermenters.clear();
	for ( auto r=c_statements.find("get_fermenters")->second.execute();
	      r; ++r) {
	  auto ft = std::make_shared<fermenter>();
	  *ft = r;
	  cache_fermenters.emplace_back(ft);
	}
      }

      void Connection::reload_tilthydrometers() {
	std::unique_lock g(c_mtx_tilthydrometers);

	cache_tilthydrometers.clear();
	for ( auto r=c_statements.find("get_tilthydrometers")->second.execute();
	      r; ++r ) {
	  auto th = std::make_shared<tilthydrometer>();
	  *th = r;
	  cache_tilthydrometers.emplace_back(th);
	}
      } // reload_tilthydrometers

      tilthydrometer_cdb Connection::getTilthydrometers() const {
	std::shared_lock g(c_mtx_tilthydrometers);
	std::list<tilthydrometer::cptr> ret;
	for (auto& it: cache_tilthydrometers)
	  ret.emplace_back(it);
	return ret;
      } // getTilthydrometers

      tilthydrometer::cptr Connection::getTilthydrometerByUUID(uuid_t _uuid) const {
	std::shared_lock g(c_mtx_tilthydrometers);
	for (auto it: cache_tilthydrometers)
	  if ( it->uuid == _uuid) return it;
	return nullptr;
      }

      void Connection::setTilthydrometer(const tilthydrometer& _item) {
	std::unique_lock g(c_mtx_tilthydrometers);
	auto& stmt(c_statements.find("set_tilthydrometer")->second);
	stmt.bind(":id", _item.id);
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
	    *th = r;
	    break;
	  }
	}
      }

      fermenter_types_cdb Connection::getFermenterTypes() const {
	std::shared_lock g(c_mtx_fermenter_types);
	std::list<fermenter_types::cptr> ret;
	for (auto it: cache_fermenter_types)
	  ret.emplace_back(it);
	return ret;
      }

      fermenter_types::cptr Connection::getFermenterTypeByID(int _id) const {
	std::shared_lock g(c_mtx_fermenter_types);
	for (auto it: cache_fermenter_types)
	  if ( it->id == _id) return it;
	return nullptr;
      }

      void Connection::updateFermenterType(const fermenter_types& _item) {
	std::unique_lock g(c_mtx_fermenter_types);
	auto& stmt(c_statements.find("update_fermenter_types")->second);
	stmt.bind(":id", _item.id);
	stmt.bind(":name", _item.name);
	stmt.bind(":capacity", _item.capacity);
	if ( _item.imageurl.length()>0 )
	  stmt.bind(":imageurl", _item.imageurl);
	else stmt.bind(":imageurl");

	auto r(stmt.execute());
	int id = r.fetch<int>("id");
	for (auto& ft: cache_fermenter_types) {
	  if ( ft->id == id ) {
	    *ft = r;
	    break;
	  }
	}
      }

      fermenter_types::cptr Connection::addFermenterType(const fermenter_types& _item) {
	std::unique_lock g(c_mtx_fermenter_types);
	auto& stmt(c_statements.find("insert_fermenter_types")->second);
	stmt.bind(":name", _item.name);
	stmt.bind(":capacity", _item.capacity);
	if ( _item.imageurl.length()>0 )
	  stmt.bind(":imageurl", _item.imageurl);
	else stmt.bind(":imageurl");

	auto r(stmt.execute());
	auto ft = std::make_shared<fermenter_types>();
	*ft = r;
	cache_fermenter_types.emplace_back(ft);
	return ft;
      }

      void Connection::deleteFermenterType(int _id) {
	std::unique_lock g(c_mtx_fermenter_types);
	auto& stmt(c_statements.find("delete_fermenter_types")->second);
	stmt.bind(":id", _id);
	stmt.execute();

	for (auto it = cache_fermenter_types.begin();
	     it != cache_fermenter_types.end(); ++it) {
	  if ( (*it)->id == _id ) {
	    cache_fermenter_types.erase(it);
	    break;
	  }
	}
      }

      fermenter_cdb Connection::getFermenters() const {
	std::shared_lock g(c_mtx_fermenters);
	std::list<fermenter::cptr> ret;
	for (auto it: cache_fermenters)
	  ret.emplace_back(std::const_pointer_cast<const fermenter>(it));
	return ret;
      }

      fermenter::cptr Connection::getFermenterByID(int _id) const {
	std::shared_lock g(c_mtx_fermenters);
	for (auto it: cache_fermenters)
	  if ( it->id == _id) return it;
	return nullptr;
      }

      void Connection::updateFermenter(const fermenter& _item) {
	std::unique_lock g(c_mtx_fermenters);
	auto& stmt(c_statements.find("update_fermenters")->second);
	stmt.bind(":id", _item.id);
	stmt.bind(":name", _item.name);
	stmt.bind(":typeid", _item.fermenter_type->id);

	auto r(stmt.execute());
	int id = r.fetch<int>("id");
	for (auto& ft: cache_fermenters) {
	  if ( ft->id == id ) {
	    *ft = r;
	    break;
	  }
	}
      }

      fermenter::cptr Connection::addFermenter(const fermenter& _item) {
	std::unique_lock g(c_mtx_fermenters);
	auto& stmt(c_statements.find("insert_fermenters")->second);
	stmt.bind(":name", _item.name);
	if ( !_item.fermenter_type )
	  throw Exception("Connection::addFermenter(): fermenter_type unset");
	stmt.bind(":typeid", _item.fermenter_type->id);

	auto r(stmt.execute());
	auto f = std::make_shared<fermenter>();
	*f = r;
	cache_fermenters.emplace_back(f);
	return f;
      }

      void Connection::deleteFermenter(int _id) {
	std::unique_lock g(c_mtx_fermenters);
	auto& stmt(c_statements.find("delete_fermenters")->second);
	stmt.bind(":id", _id);
	stmt.execute();

	for (auto it = cache_fermenters.begin();
	     it != cache_fermenters.end(); ++it) {
	  if ( (*it)->id == _id ) {
	    cache_fermenters.erase(it);
	    break;
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
