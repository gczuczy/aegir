

#include "TiltDB.hh"

#include <string.h>
#include <stdlib.h>

#include <iostream>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common/Exception.hh"
#include "common/misc.hh"

namespace aegir {
  namespace fermd {

    static boost::uuids::string_generator g_uuidstrgen;

    std::string TiltDB::entry::strUUID() const {
      return boost::uuids::to_string(uuid);
    }

    void TiltDB::entry::setUUID(const std::string& _uuid) {
      uuid = g_uuidstrgen(_uuid);
    }

    void TiltDB::entry::setColor(const std::string& _color) {
      if ( _color.length() > sizeof(color)-1 )
	throw Exception("Color %s longer than %i", _color.c_str(), sizeof(color)-1);
      memset((void*)color, 0, sizeof(color));
      memcpy((void*)color, _color.c_str(), _color.length());
    }

    void TiltDB::entry::calibrate(float _raw, float _corrected) {
      // we handle 2 calibration points: 0 and one arbitrarily specified
      // first check if we already have something in that
      if ( floateq(_raw, 1.0f) ) {
	calibrations[0].raw = _raw;
	calibrations[0].corrected = _corrected;
      } else {
	calibrations[1].raw = _raw;
	calibrations[1].corrected = _corrected;
      }
    }


    TiltDB::TiltDB(): ConfigNode(), c_size(0), c_entries(0) {
      // initialize with the default set

      addEntry("10bb95a4-b1c5-444b-b512-1370f02d74de", "Red");
      addEntry("20bb95a4-b1c5-444b-b512-1370f02d74de", "Green");
      addEntry("30bb95a4-b1c5-444b-b512-1370f02d74de", "Black");
      addEntry("40bb95a4-b1c5-444b-b512-1370f02d74de", "Purple");
      addEntry("50bb95a4-b1c5-444b-b512-1370f02d74de", "Orange");
      addEntry("60bb95a4-b1c5-444b-b512-1370f02d74de", "Blue");
      addEntry("70bb95a4-b1c5-444b-b512-1370f02d74de", "Yellow");
      addEntry("80bb95a4-b1c5-444b-b512-1370f02d74de", "Pink");
    }

    TiltDB::~TiltDB() {
      reset();
    }

    void TiltDB::marshall(ryml::NodeRef& _node) {
      //std::cout << "TiltDB::marshall" << std::endl;
      _node |= ryml::SEQ;

      std::shared_lock l(c_mutex);
      for ( int i=0; i < c_size; ++i ) {
	_node[i] |= ryml::MAP;
	_node[i]["uuid"] << c_entries[i].strUUID();
	_node[i]["color"] << c_entries[i].color;
	_node[i]["active"] << ryml::fmt::boolalpha(c_entries[i].active);

	// check the calibrations
	if ( c_entries[i].calibrations[0].corrected != 0 ||
	     c_entries[i].calibrations[1].raw != 0 ) {
	  _node[i]["calibrations"] |= ryml::MAP;


	  // the zero-point
	  if ( c_entries[i].calibrations[0].corrected != 0 ) {
	    _node[i]["calibrations"][0]["raw"] << ryml::fmt::real(0.0f, 5);
	    _node[i]["calibrations"][0]["corrected"]
	      << ryml::fmt::real(c_entries[i].calibrations[0].corrected, 5);
	  }
	  // the custom point
	  if ( c_entries[i].calibrations[1].raw != 0 ) {
	    _node[i]["calibrations"][0]["raw"]
	      << ryml::fmt::real(c_entries[i].calibrations[1].raw, 5);
	    _node[i]["calibrations"][0]["corrected"]
	      << ryml::fmt::real(c_entries[i].calibrations[1].corrected, 5);
	  }
	}
      }
    }

    void TiltDB::unmarshall(ryml::ConstNodeRef& _node) {
      if ( !_node.is_seq() )
	throw Exception("TiltDB node is not a seq");

      std::unique_lock l(c_mutex);
      reset();
      for (ryml::ConstNodeRef tilt: _node.children()) {
	if ( !tilt.is_map() )
	  throw Exception("Tilt node is not a map");

	std::string uuid, color;
	bool active;
	//std::cout << tilt << std::endl;
	if ( !tilt.has_child("uuid") )
	  throw Exception("tilt node missing key: uuid");
	if ( !tilt.has_child("color") )
	  throw Exception("tilt node missing key: color");
	if ( !tilt.has_child("active") )
	  throw Exception("tilt node missing key: active");

	tilt["uuid"] >> uuid;
	tilt["color"] >> color;
	tilt["active"] >> active;

	auto t = addEntry(uuid, color, active);

	// now check for calibrations
	if ( tilt.has_child("calibrations") ) {
	  if ( !tilt["calibrations"].is_seq() )
	    throw Exception("Tilt calibrations are not a seq");

	  for (ryml::ConstNodeRef calibration: tilt["calibrations"].children()) {
	    if ( !calibration.has_child("raw") )
	      throw Exception("Calibration missing key: raw");
	    if ( !calibration.has_child("corrected") )
	      throw Exception("Calibration missing key: corrected");

	    float raw, corrected;
	    calibration["raw"] >> raw;
	    calibration["corrected"] >> corrected;
	    t.calibrate(raw, corrected);
	  }
	}
      }
    }

    std::shared_ptr<TiltDB> TiltDB::getInstance() {
      static std::shared_ptr<TiltDB> instance{new TiltDB()};
      return instance;
    }

    std::list<TiltDB::entry> TiltDB::getEntries() const {
      std::list<entry> retval;
      {
	std::shared_lock l(c_mutex);
	for (int i=0; i < c_size; ++i ) {
	  retval.emplace_back(c_entries[i]);
	}
      }
      return retval;
    }

    TiltDB::entry& TiltDB::addEntry(const std::string& _uuid,
				    const std::string& _color,
				    bool _active) {
      uint32_t idx = c_size;
      // allocation
      if ( !c_entries ) {
	c_size = 1;
	c_entries = (entry*)malloc(sizeof(entry));
      } else {
	c_entries = (entry*)realloc((void*)c_entries, sizeof(entry)*++c_size);
      }

      c_entries[idx].setUUID(_uuid);
      c_entries[idx].setColor(_color);
      c_entries[idx].active = _active;
      c_entries[idx].calibrations[0].raw = 0;
      c_entries[idx].calibrations[0].corrected = 0;
      c_entries[idx].calibrations[1].raw = 0;
      c_entries[idx].calibrations[1].corrected = 0;
      return c_entries[idx];
    }

    void TiltDB::reset() {
      if ( c_entries ) free(c_entries);
      c_entries = 0;
      c_size = 0;
    }

  }
}
