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
      struct tilthydrometer_tuple {
	int id;
	std::string color;
	uuid_t uuid;
	bool active;
	bool enabled;
	bool has_calibr_null;
	float calibr_null;
	bool has_calibr_sg;
	float calibr_at;
	float calibr_sg;
      };
      static_assert(std::copy_constructible<tilthydrometer_tuple>,
		    "Tilthydrometer is not copy constructable");
      static_assert(std::move_constructible<tilthydrometer_tuple>,
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

	  template<typename T>
	  T fetch(int _idx) {
	    static_assert(sizeof(T)!=0, "Not implemented for type");
	  };

	  template<>
	  bool fetch(int _idx);
	  template<>
	  int fetch(int _idx);

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

      void setDBfile(const std::string& _file);
      void init();

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
    };
  }
}

#endif
