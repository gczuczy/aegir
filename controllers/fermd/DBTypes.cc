
#include "DBTypes.hh"
#include "DBResult.hh"
#include "DBConnection.hh"
#include "common/ServiceManager.hh"

namespace aegir {
  namespace fermd {
    namespace DB {
      tilthydrometer& tilthydrometer::operator=(Result& r) {
	if ( r.hasField("id") )
	  id = r.fetch<int>("id");

	if ( r.hasField("color") )
	  color = r.fetch<std::string>("color");

	if ( r.hasField("uuid") )
	  uuid = r.fetch<uuid_t>("uuid");

	active = r.fetch<bool>("active");
	enabled = r.fetch<bool>("enabled");

	if ( r.hasField("calibr_null") && !r.isNull("calibr_null") ) {
	  auto c = std::make_shared<tilthydrometer::calibration>();
	  c->sg = r.fetch<float>("calibr_null");
	  calibr_null = c;
	} else {
	  calibr_null = nullptr;
	}

	if ( r.hasField("calibr_at") && r.hasField("calibr_sg") &&
	     !r.isNull("calibr_at") && !r.isNull("calibr_sg") ) {
	  auto c = std::make_shared<tilthydrometer::calibration>();
	  c->at = r.fetch<float>("calibr_at");
	  c->sg = r.fetch<float>("calibr_sg");
	  calibr_sg = c;
	} else {
	  calibr_sg = nullptr;
	}
	return *this;
      }

      fermenter_types& fermenter_types::operator=(Result& r) {
	if ( r.hasField("id") )
	  id = r.fetch<int>("id");

	capacity = r.fetch<int>("capacity");

	name = r.fetch<std::string>("name");
	if ( !r.isNull("imageurl") )
	  imageurl = r.fetch<std::string>("imageurl");
	else imageurl = "";

	return *this;
      }

      fermenter& fermenter::operator=(Result& r) {
	if ( r.hasField("id") )
	  id = r.fetch<int>("id");

	name = r.fetch<std::string>("name");

	int tid = r.fetch<int>("typeid");
	fermenter_type = ServiceManager::get<Connection>()
	  ->getFermenterTypeByID(tid);
	return *this;
      }
    }
  }
}
