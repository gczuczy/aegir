
#include "DBTypes.hh"

#include <iostream>

#include "common/ryml.hh"
#include <boost/uuid/uuid_io.hpp>

#include "DBResult.hh"
#include "DBConnection.hh"
#include "common/ServiceManager.hh"

#include <boost/core/demangle.hpp>

#define CONDEXTRACT(FIELD) \
  if ( _node.has_child(#FIELD) ) _node[#FIELD] >> _data.FIELD

namespace aegir {
  namespace fermd {
    namespace DB {
      /*
	fermenter_types
       */
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
				fermenter_types& _data) {
	CONDEXTRACT(id);
	CONDEXTRACT(capacity);
	CONDEXTRACT(name);
	CONDEXTRACT(imageurl);

	return _node;
      }

      /*
	fermenter
       */
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
				fermenter& _data) {
	CONDEXTRACT(id);
	CONDEXTRACT(name);

	if ( _node.has_child("type") && _node["type"].has_child("id") ) {
	  int ftid;
	  _node["type"]["id"] >> ftid;
	  _data.fermenter_type = ServiceManager::get<DB::Connection>()
	    ->getFermenterTypeByID(ftid);
	}

	return _node;
      }

      /*
	tilthydrometer
       */
      tilthydrometer& tilthydrometer::operator=(Result& r) {
	if ( r.hasField("id") )
	  id = r.fetch<int>("id");

	if ( r.hasField("color") )
	  color = r.fetch<std::string>("color");

	if ( r.hasField("uuid") )
	  uuid = r.fetch<uuid_t>("uuid");

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
				tilthydrometer& _data) {
	CONDEXTRACT(id);
	CONDEXTRACT(color);
	CONDEXTRACT(enabled);

	if ( _node.has_child("uuid") ) {
	  std::string uuid;
	  _node["uuid"] >> uuid;
	  _data.uuid = g_uuidstrgen(uuid);
	}

	if ( _node.has_child("calibr_null") ) {
	  if ( _node["calibr_null"].val_is_null() ) {
	    _data.calibr_null = nullptr;
	  } else {
	    if ( !_data.calibr_null )
	      _data.calibr_null = std::make_shared<tilthydrometer::calibration>();
	    _node["calibr_null"] >> _data.calibr_null->sg;
	  }
	}

	if ( _node.has_child("calibr_sg") &&  _node.has_child("calibr_at") ) {
	  if ( _node["calibr_sg"].val_is_null() &&
	       _node["calibr_at"].val_is_null() ) {
	    _data.calibr_sg = nullptr;
	  } else {
	    if ( !_data.calibr_sg )
	      _data.calibr_sg = std::make_shared<tilthydrometer::calibration>();
	    _node["calibr_at"] >> _data.calibr_sg->at;
	    _node["calibr_sg"] >> _data.calibr_sg->sg;
	  }
	}

	if ( _node.has_child("fermenter") ) {
	  ryml::ConstNodeRef f = _node["fermenter"];
	  if ( f.has_val() ) {
	    if ( f.val_is_null() ) {
	      _data.fermenter = nullptr;
	    } else {
	      throw Exception("fermenter is a scalar but not null");
	    }
	  } else if ( f.is_container() ) {
	    int fid;
	    _node["fermenter"]["id"] >> fid;
	    _data.fermenter = ServiceManager::get<DB::Connection>()
	      ->getFermenterByID(fid);
	    if ( !_data.fermenter )
	      throw Exception("Fermenter with id %i not found", fid);
	  }
	}
	return _node;
      }

      /*
	yeast
       */
      yeast& yeast::operator=(Result& r) {
	if ( r.hasField("id") )
	  id = r.fetch<int>("id");

	name = r.fetch<std::string>("name");
	attenuation = r.fetch<float>("attenuation");
	abv = r.fetch<float>("abv");
	mintemp = r.fetch<float>("mintemp");
	maxtemp = r.fetch<float>("maxtemp");

	return *this;
      }

      ryml::NodeRef& operator<<(ryml::NodeRef& _node, const yeast& _data) {
	auto tree = _node.tree();
	_node |= ryml::MAP;
	_node["id"] << _data.id;
	auto name = tree->to_arena(_data.name);
	_node["name"] << name;
	_node["attenuation"] << ryml::fmt::real(_data.attenuation, 1);
	_node["abv"] << ryml::fmt::real(_data.abv, 1);
	_node["mintemp"] << ryml::fmt::real(_data.mintemp, 1);
	_node["maxtemp"] << ryml::fmt::real(_data.maxtemp, 1);
	return _node;
      }

      ryml::ConstNodeRef& operator>>(ryml::ConstNodeRef& _node, yeast& _data) {
	CONDEXTRACT(id);
	CONDEXTRACT(name);
	CONDEXTRACT(attenuation);
	CONDEXTRACT(abv);
	CONDEXTRACT(mintemp);
	CONDEXTRACT(maxtemp);

	return _node;
      }

      /*
	brew
       */
      brew& brew::operator=(Result& r) {
	if ( r.hasField("id") )
	  id = r.fetch<int>("id");

	name = r.fetch<std::string>("name");
	brewdate = r.fetch<std::string>("brewdate");
	if ( r.isNull("originalsg") ) {
	  originalsg.reset();
	} else {
	  originalsg = r.fetch<float>("originalsg");
	}
	sgoffset = r.fetch<float>("sgoffset");
	finished = r.fetch<bool>("finished");
	if ( r.isNull("metadata") ) {
	  metadata.reset();
	} else {
	  metadata = r.fetch<std::string>("metadata");
	}

	int yid = r.fetch<int>("yeastid");
	yeast = ServiceManager::get<Connection>()
	  ->getYeastByID(yid);

	return *this;
      }

      ryml::NodeRef& operator<<(ryml::NodeRef& _node, const brew& _data) {

	auto tree = _node.tree();
	_node |= ryml::MAP;
	_node["id"] << _data.id;
	auto name = tree->to_arena(_data.name);
	_node["name"] << name;
	auto bd = tree->to_arena(_data.brewdate);
	_node["brewdate"] << bd;

	ryml::NodeRef yeast = _node["yeast"];
	yeast << *(_data.yeast);

	if ( _data.originalsg ) {
	  _node["originalsg"] << ryml::fmt::real(_data.sgoffset, 3)
			      <<_data.originalsg.value();
	} else {
	  _node["originalsg"] << nullptr;
	}

	_node["sgoffset"] << ryml::fmt::real(_data.sgoffset, 3);
	_node["finished"] << _data.finished;

	return _node;
      }

      ryml::ConstNodeRef& operator>>(ryml::ConstNodeRef& _node, brew& _data) {
	CONDEXTRACT(id);
	CONDEXTRACT(name);
	CONDEXTRACT(brewdate);
	CONDEXTRACT(sgoffset);
	CONDEXTRACT(finished);

	// originalsg
	if ( _node.has_child("originalsg") ) {
	  ryml::ConstNodeRef osg = _node["originalsg"];
	  if ( osg.has_val() && !osg.val_is_null() ) {
	    float fosg;
	    osg >> fosg;
	    _data.originalsg = fosg;
	  } else {
	    _data.originalsg.reset();
	  }
	}

	// metadata
	if ( _node.has_child("metadata") ) {
	  ryml::ConstNodeRef md = _node["metadata"];
	  if ( md.has_val() && !md.val_is_null() ) {
	    std::string mdata;
	    md >> mdata;
	    _data.metadata = mdata;
	  } else {
	    _data.metadata.reset();
	  }
	}

	// yeast
	if ( _node.has_child("yeast") ) {
	  ryml::ConstNodeRef y = _node["yeast"];
	  if ( y.has_val() )
	    throw Exception("yeast is expected to be a non-scalar");
	  if ( !y.is_container() )
	    throw Exception("yeast is expected to be a container");

	  int yid;
	  _node["yeast"]["id"] >> yid;
	  _data.yeast = ServiceManager::get<Connection>()
	    ->getYeastByID(yid);
	  if ( !_data.yeast )
	    throw Exception("Yeast not found");
	}
	return _node;
      }

      /*
	transfer
       */
      transfer& transfer::operator=(Result& r) {
	if ( r.hasField("id") )
	  id = r.fetch<int>("id");
	transferdate = r.fetch<std::string>("transferdate");

	auto dbc = ServiceManager::get<Connection>();
	int bid = r.fetch<int>("brewid");
	brew = dbc->getBrewByID(bid);
	int fid = r.fetch<int>("fermenterid");
	fermenter = dbc->getFermenterByID(fid);
	return *this;
      }

      ryml::NodeRef& operator<<(ryml::NodeRef& _node, const transfer& _data) {
	auto tree = _node.tree();
	_node |= ryml::MAP;
	_node["id"] << _data.id;

	ryml::NodeRef brew = _node["brew"];
	brew  << *(_data.brew);
	ryml::NodeRef fermenter = _node["brew"];
	fermenter << *(_data.fermenter);

	auto td = tree->to_arena(_data.transferdate);
	_node["transferdate"] = td;
	return _node;
      }

      ryml::ConstNodeRef& operator>>(ryml::ConstNodeRef& _node, transfer& _data) {
	CONDEXTRACT(id);
	CONDEXTRACT(transferdate);

	// brew
	if ( _node.has_child("brew") ) {
	  ryml::ConstNodeRef b = _node["brew"];
	  if ( b.has_val() )
	    throw Exception("brew is expected to be a non-scalar");
	  if ( !b.is_container() )
	    throw Exception("brew is expected to be a container");

	  int bid;
	  _node["brew"]["id"] >> bid;
	  _data.brew = ServiceManager::get<Connection>()
	    ->getBrewByID(bid);
	  if ( !_data.brew )
	    throw Exception("Brew not found");
	}

	// fermenter
	if ( _node.has_child("fermenter") ) {
	  ryml::ConstNodeRef f = _node["fermenter"];
	  if ( f.has_val() )
	    throw Exception("fermenter is expected to be a non-scalar");
	  if ( !f.is_container() )
	    throw Exception("fermenter is expected to be a container");

	  int fid;
	  _node["fermenter"]["id"] >> fid;
	  _data.fermenter = ServiceManager::get<Connection>()
	    ->getFermenterByID(fid);
	  if ( !_data.fermenter )
	    throw Exception("Fermenter not found");
	}
	return _node;
      }

      /*
	fermentationlog
       */
      fermentationlog& fermentationlog::operator=(Result& r) {
	if ( r.hasField("id") )
	  id = r.fetch<int>("id");

	timestamp = r.fetch<int>("timestamp");
	sg = r.fetch<float>("sg");
	temperature = r.fetch<float>("temperature");

	int bid = r.fetch<int>("brewid");
	brew = ServiceManager::get<Connection>()
	  ->getBrewByID(bid);

	return *this;
      }

      ryml::NodeRef& operator<<(ryml::NodeRef& _node,
				const fermentationlog& _data) {
	auto tree = _node.tree();
	_node |= ryml::MAP;
	_node["id"] << _data.id;
	_node["sg"] << ryml::fmt::real(_data.sg, 3);
	_node["temperature"] << ryml::fmt::real(_data.temperature, 1);
	ryml::NodeRef brew = _node["brew"];
	brew  << *(_data.brew);

	// timestamp
	std::tm tm{};
	gmtime_r(&_data.timestamp, &tm);
	char buffer[32];
	if ( std::strftime(buffer, 31, "%Y-%m-%dT%H:%M:%SZ", &tm) == 0 )
	  throw Exception("Unable to format timestamp: %s", strerror(errno));

	auto ts = tree->to_arena(buffer);
	_node["timestamp"] << ts;

	return _node;
      }

      ryml::ConstNodeRef& operator>>(ryml::ConstNodeRef& _node,
				     fermentationlog& _data) {
	CONDEXTRACT(id);
	CONDEXTRACT(sg);
	CONDEXTRACT(temperature);

	// timestamp
	if ( _node.has_child("timestamp") ) {
	  std::string ts;
	  _node["timestamp"] >> ts;
	  std::tm tm{};

	  if ( strptime(ts.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == nullptr )
	    throw Exception("Unable to parse timestamp %s", ts.c_str());

	  _data.timestamp = std::mktime(&tm);
	}

	if ( _node.has_child("brew") ) {
	  ryml::ConstNodeRef x = _node["brew"];
	  if ( x.has_val() ) {
	    if ( x.val_is_null() ) {
	      _data.brew = nullptr;
	    } else {
	      throw Exception("brew is a scalar but not null");
	    }
	  } else if ( x.is_container() ) {
	    int bid;
	    _node["brew"]["id"] >> bid;
	    _data.brew = ServiceManager::get<DB::Connection>()
	      ->getBrewByID(bid);
	    if ( !_data.brew )
	      throw Exception("Brew with id %i not found", bid);
	  }
	}
	return _node;
      }

    } // ns DB
  } // ns fermd
} // ns aegir

