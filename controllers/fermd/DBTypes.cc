
#include "DBTypes.hh"

#include "common/ryml.hh"
#include <boost/uuid/uuid_io.hpp>

#include "DBResult.hh"
#include "DBConnection.hh"
#include "common/ServiceManager.hh"

#include <boost/core/demangle.hpp>

namespace aegir {
  namespace fermd {
    namespace DB {
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

      ryml::NodeRef& operator<<(ryml::NodeRef& _node,
				const fermenter_types& _ft) {
	auto tree = _node.tree();
	_node |= ryml::MAP;
	_node["id"] << _ft.id;
	_node["capacity"] << _ft.capacity;

	auto name = tree->to_arena(_ft.name);
	_node["name"] = name;
	auto imageurl = tree->to_arena(_ft.imageurl);
	_node["imageurl"] = imageurl;
	return _node;
      }

      ryml::ConstNodeRef& operator>>(ryml::ConstNodeRef& _node,
				fermenter_types& _ft) {
	if ( _node.has_child("id") )
	  _node["id"] >> _ft.id;
	if ( _node.has_child("capacity") )
	  _node["capacity"] >> _ft.capacity;
	if ( _node.has_child("name") )
	  _node["name"] >> _ft.name;
	if ( _node.has_child("imageurl") )
	  _node["imageurl"] >> _ft.imageurl;

	return _node;
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

      ryml::NodeRef& operator<<(ryml::NodeRef& _node, const fermenter& _f) {
	auto tree = _node.tree();
	_node |= ryml::MAP;
	_node["id"] << _f.id;

	auto name = tree->to_arena(_f.name);
	_node["name"] = name;

	ryml::NodeRef ftype = _node["type"];
	ftype << *(_f.fermenter_type);
	return _node;
      }

      ryml::ConstNodeRef& operator>>(ryml::ConstNodeRef& _node,
				fermenter& _ft) {
	if ( _node.has_child("id") )
	  _node["id"] >> _ft.id;
	if ( _node.has_child("name") )
	  _node["name"] >> _ft.name;

	if ( _node.has_child("type") && _node["type"].has_child("id") ) {
	  int ftid;
	  _node["type"]["id"] >> ftid;
	  _ft.fermenter_type = ServiceManager::get<DB::Connection>()
	    ->getFermenterTypeByID(ftid);
	}

	return _node;
      }

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

	if ( r.isNull("fermenterid") ) {
	  fermenter = nullptr;
	} else {
	  fermenter = ServiceManager::get<Connection>()
	    ->getFermenterByID(r.fetch<int>("fermenterid"));
	}
	return *this;
      }

      ryml::NodeRef& operator<<(ryml::NodeRef& _node,
				const tilthydrometer& _th) {
	auto tree = _node.tree();
	_node |= ryml::MAP;
	_node["id"] << _th.id;

	auto color = tree->to_arena(_th.color);
	_node["color"] << color;
	auto uuid = tree->to_arena(boost::uuids::to_string(_th.uuid));
	_node["uuid"] << uuid;
	_node["active"] << ryml::fmt::boolalpha(_th.active);
	_node["enabled"] << ryml::fmt::boolalpha(_th.enabled);

	if ( _th.calibr_null ) {
	  _node["calibr_null"] << ryml::fmt::real(_th.calibr_null->sg, 3);
	} else {
	  _node["calibr_null"] = nullptr;
	}

	if ( _th.calibr_sg ) {
	  _node["calibr_at"] << ryml::fmt::real(_th.calibr_sg->at, 3);
	  _node["calibr_sg"] << ryml::fmt::real(_th.calibr_sg->sg, 3);
	} else {
	  _node["calibr_at"] = nullptr;
	  _node["calibr_sg"] = nullptr;
	}

	ryml::NodeRef fermenter = _node["fermenter"];
	if ( _th.fermenter ) {
	  fermenter << *(_th.fermenter);
	} else {
	  fermenter = nullptr;
	}
	return _node;
      }

      ryml::ConstNodeRef& operator>>(ryml::ConstNodeRef& _node,
				tilthydrometer& _th) {
	if ( _node.has_child("id") )
	     _node["id"] >> _th.id;

	if ( _node.has_child("color") )
	  _node["color"] >> _th.color;

	if ( _node.has_child("uuid") ) {
	  std::string uuid;
	  _node["uuid"] >> uuid;
	  _th.uuid = g_uuidstrgen(uuid);
	}

	if ( _node.has_child("active") )
	  _node["active"] >> _th.active;
	if ( _node.has_child("enabled") )
	  _node["enabled"] >> _th.enabled;

	if ( _node.has_child("calibr_null") ) {
	  if ( _node["calibr_null"].val_is_null() ) {
	    _th.calibr_null = nullptr;
	  } else {
	    if ( !_th.calibr_null )
	      _th.calibr_null = std::make_shared<tilthydrometer::calibration>();
	    _node["calibr_null"] >> _th.calibr_null->sg;
	  }
	}

	if ( _node.has_child("calibr_sg") &&  _node.has_child("calibr_at") ) {
	  if ( _node["calibr_sg"].val_is_null() &&
	       _node["calibr_at"].val_is_null() ) {
	    _th.calibr_sg = nullptr;
	  } else {
	    if ( !_th.calibr_sg )
	      _th.calibr_sg = std::make_shared<tilthydrometer::calibration>();
	    _node["calibr_sg"] >> _th.calibr_sg->at;
	    _node["calibr_at"] >> _th.calibr_sg->sg;
	  }
	}

	if ( _node.has_child("fermenter") ) {
	  if ( _node["fermenter"].val_is_null() ) {
	    _th.fermenter = nullptr;
	  } else {
	    if ( _node["fermenter"].has_child("id") ) {
	      int fid;
	      _node["fermenter"]["id"] >> fid;
	      _th.fermenter = ServiceManager::get<DB::Connection>()
		->getFermenterByID(fid);
	    }
	  }
	}
	return _node;
      }
    } // ns DB
  } // ns fermd
} // ns aegir

