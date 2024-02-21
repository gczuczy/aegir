
#ifndef AEGIR_FERMD_DB_TRANSACTION
#define AEGIR_FERMD_DB_TRANSACTION

#include "DBStatement.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      class Connection;
      class Transaction {
	friend Connection;
      protected:
	Transaction(Connection* _db);
      public:
	~Transaction();

	inline Connection* operator->() {
	  return c_db;
	};
	void setTilthydrometer(const tilthydrometer& _item);

      private:
	Connection* c_db;
      };
    } // ns DB
  } // ns fermd
} // ns aegir

#endif

