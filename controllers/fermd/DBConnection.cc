
#include "DBConnection.hh"
#include "DB.hh"
#include <battery/embed.hpp>

namespace aegir {
	namespace fermd {
		namespace DB {
      static void s3log(void* _logger, int _errcode, const char* _msg) {
#if 0
				LogChannel *lc = (LogChannel*)_logger;
				printf("Logchannel: %p\n", (void*)lc);
				lc->error("SQLite3(%i) error: %s", _errcode, _msg);
#else
				LogChannel lc("SQLite3");
				lc.error("SQLite3(%i) error: %s", _errcode, _msg);
#endif
      }

			thread_local bool Connection::c_threadlocked(false);

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
#if 0
				c_schemas.emplace_back(Schema(1,
																			std::string((char*)sql_v1_sql,
																									sql_v1_sql_len)));
#else
				c_schemas.emplace_back(Schema(1, b::embed<"fermd/sql/v1.sql">()));
#endif
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
				Statement(c_db, "PRAGMA automatic_index = OFF", true)
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

				// yeasts
				prepare("get_yeasts",
								"SELECT id,name,attenuation,abv,mintemp,maxtemp FROM yeasts");
				prepare("update_yeasts",
								"UPDATE yeasts "
								"SET name=:name,attenuation=:attenuation,"
								"abv=:abv,mintemp=:mintemp,maxtemp=:maxtemp "
								"WHERE id=:id "
								"RETURNING id,name,attenuation,abv,mintemp,maxtemp");
				prepare("insert_yeasts",
								"INSERT INTO yeasts "
								"(name, attenuation, abv, mintemp, maxtemp) "
								"VALUES (:name,:attenuation,:abv,:mintemp,:maxtemp) "
								"RETURNING id,name,attenuation,abv,mintemp,maxtemp");
				prepare("delete_yeasts",
								"DELETE FROM yeasts WHERE id=:id ");

				// brews
				prepare("get_brews",
								"SELECT id,name,yeastid,brewdate,originalsg,"
								"sgoffset,finished,metadata FROM brews");
				prepare("update_brews",
								"UPDATE brews "
								"SET name=:name,yeastid=:yeastid,brewdate=:brewdate,"
								"originalsg=:originalsg,sgoffset=:sgoffset,"
								"finished=:finished,metadata=:metadata "
								"WHERE id=:id "
								"RETURNING id,name,yeastid,brewdate,originalsg,"
								"sgoffset,finished,metadata");
				prepare("insert_brews",
								"INSERT INTO brews "
								"(name,yeastid,brewdate,originalsg,sgoffset,finished,metadata) "
								"VALUES (:name,:yeastid,:brewdate,:originalsg,"
								":sgoffset,:finished,:metadata) "
								"RETURNING id,name,yeastid,brewdate,originalsg,"
								"sgoffset,finished,metadata");
				prepare("delete_brews",
								"DELETE FROM brews WHERE id=:id ");

				// transfers
				prepare("get_transfers",
								"SELECT id,brewid,fermenterid,transferdate FROM transfers");
				prepare("insert_transfers",
								"INSERT INTO transfers "
								"(brewid,fermenterid,transferdate) "
								"VALUES (:brewid,:fermenterid,:transferdate) "
								"RETURNING id,brewid,fermenterid,transferdate");

				// femerntationlog
				prepare("get_fermentationlog",
								"SELECT id,brewid,timestamp,sg,temperature FROM fermentationlog "
								"ORDER BY timestamp ASC");
				prepare("insert_fermentationlog",
								"INSERT INTO fermentationlog "
								"(brewid,timestamp,sg,temperature) "
								"VALUES (:brewid,:timestamp,:sg,:temperature) "
								"RETURNING id,brewid,timestamp,sg,temperature");

				// fermentingbrews
				prepare("get_fermentingbrews",
								"SELECT brewid,xferid,fermenterid FROM fermentingbrews");

				reload();
      } // init

      void Connection::setConnectionFile(const std::string& _file) {
				if ( c_db )
					throw Exception("Cannot set dbfile while db is open");
				c_dbfile = _file;
      }

      void Connection::reload() {
				// to ensure consistency, we lock everything here
				std::unique_lock g1(c_mtx_fermenter_types);
				std::unique_lock g2(c_mtx_fermenters);
				std::unique_lock g3(c_mtx_tilthydrometers);
				std::unique_lock g4(c_mtx_yeasts);
				std::unique_lock g5(c_mtx_brews);
				std::unique_lock g6(c_mtx_transfers);
				std::unique_lock g7(c_mtx_fermentationlogs);

				c_threadlocked = true;

				try {
					reload_fermenter_types();
					reload_fermenters();
					reload_tilthydrometers();
					reload_yeasts();
					reload_brews();
					reload_transfers();
					reload_fermentationlog();
					reload_fermentingbrews();
				}
				catch (Exception &e) {
					c_threadlocked = false;
					throw e;
				}
				catch (std::exception &e) {
					c_threadlocked = false;
					throw e;
				}
				catch (...) {
					c_threadlocked = false;
					throw Exception("Unknown exception happened");
				}

				c_threadlocked = false;
      }

      void Connection::reload_fermenter_types() {
				cache_fermenter_types.clear();
				for ( auto r=c_statements.find("get_fermenter_types")->second.execute();
							r; ++r) {
					auto ft = std::make_shared<fermenter_types>();
					*ft = r;
					cache_fermenter_types.emplace_back(ft);
				}
      }

      void Connection::reload_fermenters() {
				cache_fermenters.clear();
				for ( auto r=c_statements.find("get_fermenters")->second.execute();
							r; ++r) {
					auto ft = std::make_shared<fermenter>();
					*ft = r;
					cache_fermenters.emplace_back(ft);
				}
      }

      void Connection::reload_tilthydrometers() {
				cache_tilthydrometers.clear();
				for ( auto r=c_statements.find("get_tilthydrometers")->second.execute();
							r; ++r ) {
					auto th = std::make_shared<tilthydrometer>();
					*th = r;
					cache_tilthydrometers.emplace_back(th);
				}
      } // reload_tilthydrometers

      void Connection::reload_yeasts() {
				cache_yeasts.clear();
				for ( auto r=c_statements.find("get_yeasts")->second.execute();
							r; ++r ) {
					auto th = std::make_shared<yeast>();
					*th = r;
					cache_yeasts.emplace_back(th);
				}
      }

      void Connection::reload_brews() {
				cache_brews.clear();
				for ( auto r=c_statements.find("get_brews")->second.execute();
							r; ++r ) {
					auto th = std::make_shared<brew>();
					*th = r;
					cache_brews.emplace_back(th);
				}
      }

      void Connection::reload_transfers() {
				cache_transfers.clear();
				for ( auto r=c_statements.find("get_transfers")->second.execute();
							r; ++r ) {
					auto th = std::make_shared<transfer>();
					*th = r;
					cache_transfers.emplace_back(th);
				}
      }

      void Connection::reload_fermentationlog() {
				cache_fermentationlogs.clear();
				for ( auto r=c_statements.find("get_fermentationlog")->second.execute();
							r; ++r ) {
					auto th = std::make_shared<fermentationlog>();
					*th = r;
					cache_fermentationlogs.emplace_back(th);
				}
      }

			void Connection::reload_fermentingbrews() {
				// first clear the current cache keys
				for (auto &it: cache_brews) it->cache_fermenterid.reset();
				for (auto &it: cache_fermenters) it->cache_brewid.reset();

				// now check what we have and emplace the new cache keys
				for ( auto r=c_statements.find("get_fermentingbrews")->second.execute();
							r; ++r ) {
					int brewid = r.fetch<int>("brewid");
					int fermid = r.fetch<int>("fermenterid");

					// fermenterid -> brew
					for (auto &it: cache_brews) {
						if (it->id != brewid) continue;
						it->cache_fermenterid = fermid;
						break;
					}

					// brewid -> fermenter
					for (auto &it: cache_fermenters) {
						if (it->id != fermid) continue;
						it->cache_brewid = brewid;
						break;
					}
				}
			}

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

      tilthydrometer::cptr Connection::getTilthydrometerByID(int _id) const {
				std::shared_lock g(c_mtx_tilthydrometers);
				for (auto it: cache_tilthydrometers)
					if ( it->id == _id) return it;
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
					//printf("Binding calibrat to %.2f\n", _item.calibr_sg->at);
					stmt.bind(":calibrat", _item.calibr_sg->at);
					//printf("Binding calibrsg to %.2f\n", _item.calibr_sg->sg);
					stmt.bind(":calibrsg", _item.calibr_sg->sg);
				} else {
					//printf("binding calibr at+sg @ nul\n");
					stmt.bind(":calibrat");
					stmt.bind(":calibrsg");
				}

				// TODO: add fermenters
				if ( _item.fermenter ) {
					stmt.bind(":fermenterid", _item.fermenter->id);
				} else {
					stmt.bind(":fermenterid");
				}

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
				auto f = [this, _id]() -> fermenter_types::cptr {
					for (auto it: cache_fermenter_types)
						if ( it->id == _id) return it;
					return nullptr;
				};
				if ( c_threadlocked ) return f();
				std::shared_lock g(c_mtx_fermenter_types);
				return f();
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
				auto f = [this, _id]() -> fermenter::cptr {
					for (auto it: cache_fermenters)
						if ( it->id == _id) return it;
					return nullptr;
				};
				if ( c_threadlocked ) return f();
				std::shared_lock g(c_mtx_fermenters);
				return f();
      }

			brew::cptr Connection::getFermenterBrew(int _fid) const {
				std::unique_lock g(c_mtx_transfers);
				transfer::cptr last;

				for (auto it: cache_transfers) {
					if ( it->brew->finished || it->fermenter->id != _fid ) continue;
					if ( last ) {
						if ( it->transferdate > last->transferdate ) last = it;
					} else {
						last = it;
					}
				}
				if ( last ) return last->brew;
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

      /*
       * Yeasts
       */
      yeast_cdb Connection::getYeasts() const {
				std::shared_lock g(c_mtx_yeasts);
				std::list<yeast::cptr> ret;
				for (auto it: cache_yeasts)
					ret.emplace_back(std::const_pointer_cast<const yeast>(it));
				return ret;
      }

      yeast::cptr Connection::getYeastByID(int _id) const {
				auto f = [this, _id]() -> yeast::cptr {
					for (auto it: cache_yeasts)
						if ( it->id == _id) return it;
					return nullptr;
				};
				if ( c_threadlocked ) return f();
				std::shared_lock g(c_mtx_yeasts);
				return f();
			}

      void Connection::updateYeast(const yeast& _item) {
				std::unique_lock g(c_mtx_yeasts);
				auto& stmt(c_statements.find("update_yeasts")->second);
				stmt.bind(":id", _item.id);
				stmt.bind(":name", _item.name);
				stmt.bind(":attenuation", _item.attenuation);
				stmt.bind(":abv", _item.abv);
				stmt.bind(":mintemp", _item.mintemp);
				stmt.bind(":maxtemp", _item.maxtemp);

				auto r(stmt.execute());
				int id = r.fetch<int>("id");
				for (auto& ft: cache_yeasts) {
					if ( ft->id == id ) {
						*ft = r;
						break;
					}
				}
      }

      yeast::cptr Connection::addYeast(const yeast& _item) {
				std::unique_lock g(c_mtx_yeasts);
				auto& stmt(c_statements.find("insert_yeasts")->second);
				stmt.bind(":name", _item.name);
				stmt.bind(":attenuation", _item.attenuation);
				stmt.bind(":abv", _item.abv);
				stmt.bind(":mintemp", _item.mintemp);
				stmt.bind(":maxtemp", _item.maxtemp);

				auto r(stmt.execute());
				auto ret = std::make_shared<yeast>();
				*ret = r;
				cache_yeasts.emplace_back(ret);
				return ret;
      }

      void Connection::deleteYeast(int _id) {
				std::unique_lock g(c_mtx_yeasts);
				auto& stmt(c_statements.find("delete_yeasts")->second);
				stmt.bind(":id", _id);
				stmt.execute();

				for (auto it = cache_yeasts.begin();
						 it != cache_yeasts.end(); ++it) {
					if ( (*it)->id == _id ) {
						cache_yeasts.erase(it);
						break;
					}
				}
      }

      /*
       * brew
       */
      brew_cdb Connection::getBrews() const {
				std::shared_lock g(c_mtx_brews);
				std::list<brew::cptr> ret;
				for (auto it: cache_brews)
					ret.emplace_back(std::const_pointer_cast<const brew>(it));
				return ret;
      }

      brew::cptr Connection::getBrewByID(int _id) const {
				auto f = [this, _id]() -> brew::cptr {
					for (auto it: cache_brews)
						if ( it->id == _id) return it;
					return nullptr;
				};
				if ( c_threadlocked ) return f();
				std::shared_lock g(c_mtx_brews);
				return f();

      }

      void Connection::updateBrew(const brew& _item) {
				std::unique_lock gb(c_mtx_brews);
				std::shared_lock gt(c_mtx_transfers);
				std::unique_lock gf(c_mtx_fermenters);
				auto& stmt(c_statements.find("update_brews")->second);
				stmt.bind(":id", _item.id)
					.bind(":name", _item.name)
					.bind(":brewdate", _item.brewdate)
					.bind(":originalsg", _item.originalsg)
					.bind(":sgoffset", _item.sgoffset)
					.bind(":finished", _item.finished)
					.bind(":metadata", _item.metadata)
					.bind(":yeastid", _item.yeast->id);

				auto r(stmt.execute());
				int id = r.fetch<int>("id");
				for (auto& data: cache_brews) {
					if ( data->id == id ) {
						*data = r;
						break;
					}
				}
				reload_fermentingbrews();
      }

      brew::cptr Connection::addBrew(const brew& _item) {
				std::unique_lock g(c_mtx_brews);
				auto r(getStatement("insert_brews")
							 .bind(":name", _item.name)
							 .bind(":brewdate", _item.brewdate)
							 .bind(":originalsg", _item.originalsg)
							 .bind(":sgoffset", _item.sgoffset)
							 .bind(":finished", _item.finished)
							 .bind(":metadata", _item.metadata)
							 .bind(":yeastid", _item.yeast->id)
							 .execute());

				auto ret = std::make_shared<brew>();
				*ret = r;
				cache_brews.emplace_back(ret);
				return ret;
      }

      void Connection::deleteBrew(int _id) {
				std::unique_lock g(c_mtx_yeasts);
				getStatement("delete_brews")
					.bind(":id", _id)
					.execute();

				for (auto it = cache_brews.begin();
						 it != cache_brews.end(); ++it) {
					if ( (*it)->id == _id ) {
						cache_brews.erase(it);
						break;
					}
				}
      }

      /*
       * transfer
       */
      transfer_cdb Connection::getTransfers() const {
				std::shared_lock g(c_mtx_transfers);
				std::list<transfer::cptr> ret;
				for (auto it: cache_transfers)
					ret.emplace_back(std::const_pointer_cast<const transfer>(it));
				return ret;
      }

      transfer::cptr Connection::getTransferByID(int _id) const {
				std::shared_lock g(c_mtx_transfers);
				for (auto it: cache_transfers)
					if ( it->id == _id) return it;
				return nullptr;
      }

      transfer_cdb Connection::getTransfersByBrew(const brew& _brew) const {
				std::shared_lock g(c_mtx_transfers);
				std::list<transfer::cptr> ret;
				for (auto it: cache_transfers)
					if ( it->brew->id == _brew.id)
						ret.emplace_back(std::const_pointer_cast<const transfer>(it));
				return ret;
      }

      transfer::cptr Connection::addTransfer(const transfer& _item) {
				std::unique_lock g(c_mtx_transfers);
				std::unique_lock gb(c_mtx_brews);
				std::unique_lock gf(c_mtx_fermenters);
				auto r(getStatement("insert_transfers")
							 .bind(":brewid", _item.brew->id)
							 .bind(":fermenterid", _item.fermenter->id)
							 .bind(":transferdate", _item.transferdate)
							 .execute());

				auto ret = std::make_shared<transfer>();
				bool locking(false);
				if ( !c_threadlocked ) {
					locking = false;
					c_threadlocked = true;
				}
				*ret = r;
				cache_transfers.emplace_back(ret);
				reload_fermentingbrews();
				if ( locking ) c_threadlocked = false;
				return ret;
      }

      /*
       * fermentationlog
       */
      fermentationlog_cdb Connection::getFermentationlogsByBrew(const brew& _brew) const {
				std::shared_lock g(c_mtx_fermentationlogs);
				std::list<fermentationlog::cptr> ret;
				for (auto it: cache_fermentationlogs)
					if ( it->brew->id == _brew.id)
						ret.emplace_back(std::const_pointer_cast<const fermentationlog>(it));
				return ret;
      }

      fermentationlog::cptr Connection::addFermentationlog(const fermentationlog& _item) {
				std::unique_lock g(c_mtx_fermentationlogs);
				auto r(getStatement("insert_fermentationlog")
							 .bind(":brewid", _item.brew->id)
							 .bind(":timestamp", _item.timestamp)
							 .bind(":sg", _item.sg)
							 .bind(":temperature", _item.temperature)
							 .execute());

				auto ret = std::make_shared<fermentationlog>();
				*ret = r;
				cache_fermentationlogs.emplace_back(ret);
				return ret;
      }

      /*
       * txn stuff
       */

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

			Statement& Connection::getStatement(const std::string& _name) {
				auto it = c_statements.find(_name);
				if ( it == c_statements.end() )
					throw Exception("No such prepared statement %s", _name.c_str());
				return it->second;
      }
    } // ns DB
  } // ns fermd
} // ns aegir
