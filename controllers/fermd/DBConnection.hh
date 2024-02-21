
#ifndef AEGIR_FERMD_DB_CONNECTION
#define AEGIR_FERMD_DB_CONNECTION

#include <string>
#include <list>
#include <shared_mutex>

#include <sqlite3.h>

#include "DBTypes.hh"
#include "DBStatement.hh"
#include "DBTransaction.hh"
#include "DBResult.hh"
#include "DBSchema.hh"
#include "common/ConfigBase.hh"
#include "common/LogChannel.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      class Transaction;
      class Connection: public aegir::ConfigNode {
	friend class Transaction;
      public:
	typedef std::shared_ptr<Connection> pointer_type;

      private:
	Connection();

      public:
	Connection(const Connection&)=delete;
	Connection(Connection&&)=delete;
	virtual ~Connection();

	static std::shared_ptr<Connection> getInstance();

	virtual void marshall(ryml::NodeRef&);
	virtual void unmarshall(ryml::ConstNodeRef&);

	void init();
	void setConnectionFile(const std::string& _file);

	// Tilt Hydrometers
      public:
	void reload_tilthydrometers();
	tilthydrometer_cdb getTilthydrometers() const;
	tilthydrometer::cptr getTilthydrometerByUUID(uuid_t _uuid) const;
      protected:
	void setTilthydrometer(const tilthydrometer& _item);

      public:
	inline Transaction txn() {
	  return Transaction(this);
	};
      protected:
	void begin();
	void commit();

      private:
	void prepare(const std::string& _name,
		     const std::string& _stmt,
		     bool _temporary=false);


      private:
	std::string c_dbfile;
	sqlite3 *c_db;
	LogChannel c_logger;
	std::map<std::string, Statement> c_statements;
	std::list<Schema> c_schemas;
	mutable std::mutex c_mtx;
	// storages
	mutable std::shared_mutex c_mtx_tilthydrometers;
	tilthydrometer_db cache_tilthydrometers;
      }; // class Connection

    } // ns DB
  } // ns fermd
} // ns aegir

#endif

