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

      void Transaction::reload() {
	c_db->reload();
      }

      void Transaction::setTilthydrometer(const tilthydrometer& _item) {
	c_db->setTilthydrometer(_item);
      }

      void Transaction::updateFermenterType(const fermenter_types& _item) {
	c_db->updateFermenterType(_item);
      }

      fermenter_types::cptr Transaction::addFermenterType(const fermenter_types& _item) {
	return c_db->addFermenterType(_item);
      }

      void Transaction::deleteFermenterType(int _id) {
	c_db->deleteFermenterType(_id);
      }
    } // ns DB
  } // ns fermd
} // ns aegir

