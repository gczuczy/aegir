/*
  SQLite DB connection
 */

#ifndef AEGIR_FERMD_DB
#define AEGIR_FERMD_DB

#include <string>
#include <memory>
#include <map>
#include <list>
#include <concepts>
#include <shared_mutex>

#include <boost/uuid/uuid.hpp>

#include <sqlite3.h>

#include "common/Exception.hh"
#include "common/ConfigBase.hh"
#include "common/LogChannel.hh"

namespace aegir {
  namespace fermd {

    typedef boost::uuids::uuid uuid_t;

    class DB: public aegir::ConfigNode {
    public:
      typedef std::shared_ptr<DB> pointer_type;
      struct tilthydrometer {
	typedef std::shared_ptr<tilthydrometer> ptr;
	struct calibration {
	  float at;
	  float sg;
	};
	int id;
	std::string color;
	uuid_t uuid;
	bool active;
	bool enabled;
	std::shared_ptr<calibration> calibr_null;
	std::shared_ptr<calibration> calibr_sg;
      };
      typedef std::list<tilthydrometer::ptr> tilthydrometer_db;
      static_assert(std::copy_constructible<tilthydrometer>,
		    "Tilthydrometer is not copy constructable");
      static_assert(std::move_constructible<tilthydrometer>,
		    "Tilthydrometer is not move constructable");

    private:
      class Statement {
      public:
	class Result {
	  friend Statement;
	protected:
	  Result(sqlite3_stmt* _stmt);
	public:
	  Result()=delete;
	  Result(const Result&)=delete;
	  Result(Result&&);
	  ~Result();

	  inline Result& operator++() {
	    sqlite3_step(c_statement);
	    return *this;
	  };
	  inline operator bool() const {
	    return sqlite3_stmt_busy(c_statement) == 1;
	  }
	  bool isNull(const std::string& _name);
	  bool isNull(int _idx);

	  template<typename T>
	  T fetch(int _idx) {
	    static_assert(sizeof(T)!=0, "Not implemented for type");
	  };

	  template<>
	  bool fetch(int _idx);
	  template<>
	  int fetch(int _idx);
	  template<>
	  std::string fetch(int _idx);
	  template<>
	  float fetch(int _idx);
	  template<>
	  uuid_t fetch(int _idx);

	  template<typename T>
	  T fetch(const std::string& _name) {
	    for (int i=0; i<=sqlite3_data_count(c_statement); ++i ) {
	      if ( _name == sqlite3_column_name(c_statement, i) )
		return fetch<T>(i);
	    }
	    throw Exception("Column \"%s\" not found", _name.c_str());
	  };
	private:
	  sqlite3_stmt *c_statement;
	};
      public:
	Statement()=delete;
	Statement(sqlite3 *_db,
		  const std::string& _stmt, bool _temporary=false);
	Statement(const Statement&)=delete;
	Statement(Statement&&)=delete;
	Statement& operator=(Statement&&);
	~Statement();

	Result execute();

      private:
	sqlite3_stmt *c_statement;
      }; // class Statement

      // The DB schemas
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

    private:
      DB();

    public:
      DB(const DB&)=delete;
      DB(DB&&)=delete;
      virtual ~DB();

      static std::shared_ptr<DB> getInstance();

      virtual void marshall(ryml::NodeRef&);
      virtual void unmarshall(ryml::ConstNodeRef&);

      void init();
      void setDBfile(const std::string& _file);
      void reload_tilthydrometers();

      tilthydrometer_db getTilthydrometers() const;
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
      // storages
      mutable std::shared_mutex c_mtx;
      tilthydrometer_db cache_tilthydrometers;
    };
  }
}

#endif
