#include "DBTransaction.hh"
#include "DBConnection.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      Transaction::Transaction(Connection* _db): c_db(_db) {
	c_db->begin();
      }

      Transaction::~Transaction() {
	c_db->commit();
      }
      void Transaction::setTilthydrometer(const tilthydrometer& _item) {
	c_db->setTilthydrometer(_item);
      }
    } // ns DB
  } // ns fermd
} // ns aegir

