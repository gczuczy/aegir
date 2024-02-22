
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

	// reloads
      protected:
	void reload();
      private:
	void reload_fermenter_types();
	void reload_fermenters();
	void reload_tilthydrometers();

	// Tilt Hydrometers
      public:
	tilthydrometer_cdb getTilthydrometers() const;
	tilthydrometer::cptr getTilthydrometerByUUID(uuid_t _uuid) const;
      protected:
	void setTilthydrometer(const tilthydrometer& _item);

	// Fermenter Types
      public:
	fermenter_types_cdb getFermenterTypes() const;
	fermenter_types::cptr getFermenterTypeByID(int _id) const;
      protected:
	void updateFermenterType(const fermenter_types& _item);
	fermenter_types::cptr addFermenterType(const fermenter_types& _item);
	void deleteFermenterType(int _id);

	// fermenters
      public:
	fermenter_cdb getFermenters() const;
	fermenter::cptr getFermenterByID(int _id) const;
      protected:
	void updateFermenter(const fermenter& _item);
	fermenter::cptr addFermenter(const fermenter& _item);
	void deleteFermenter(int _id);

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
	mutable std::shared_mutex c_mtx_fermenter_types;
	fermenter_types_db cache_fermenter_types;
	mutable std::shared_mutex c_mtx_fermenters;
	fermenter_db cache_fermenters;
      }; // class Connection

    } // ns DB
  } // ns fermd
} // ns aegir

#endif

