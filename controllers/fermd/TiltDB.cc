

#include "TiltDB.hh"

#include <string.h>
#include <stdlib.h>

#include "Exception.hh"

namespace aegir {
  namespace fermd {

    TiltDB::entry::entry(const std::string& _uuid,
			 const std::string& _color,
			 bool _active): active(_active) {

      // Parse the UUID str
      uint32_t uuidstatus;
      uuid_from_string(_uuid.c_str(), &uuid, &uuidstatus);
      if ( uuidstatus != uuid_s_ok ) {
	std::string msg;
	switch (uuidstatus) {
	case uuid_s_bad_version:
	  msg = "The UUID does not have a known version.";
	  break;
	case uuid_s_invalid_string_uuid:
	  msg = "The string representation of an UUID is not valid";
	  break;
	case uuid_s_no_memory:
	  msg = "The function can not allocate memory to store an UUID representation.";
	  break;
	default:
	  msg = "Unknown error";
	  break;
	}
	throw Exception("Error parsing UUID(%s): %s", _uuid.c_str(), msg.c_str());
      }

      // Check and store the color
      if ( _color.length() > sizeof(color)-1 )
	throw Exception("Color %s longer than %i", _color.c_str(), sizeof(color)-1);
      memset((void*)color, 0, sizeof(color));
      memcpy((void*)color, _color.c_str(), _color.length());
    }

    std::string TiltDB::entry::strUUID() const {
      char *str(0);
      uint32_t uuidstatus;

      uuid_to_string(&uuid, &str, &uuidstatus);
      if ( uuidstatus != uuid_s_ok ) {
	std::string msg;
	switch (uuidstatus) {
	case uuid_s_bad_version:
	  msg = "The UUID does not have a known version.";
	  break;
	case uuid_s_invalid_string_uuid:
	  msg = "The string representation of an UUID is not valid";
	  break;
	case uuid_s_no_memory:
	  msg = "The function can not allocate memory to store an UUID representation.";
	  break;
	default:
	  msg = "Unknown error";
	  break;
	}
	throw Exception("uuid_to_string: %s", msg.c_str());
      }

      std::string retval(str);

      if ( str ) free(str);
      return retval;
    }


    TiltDB::TiltDB(): ConfigNode(), c_size(0), c_entries(0) {
      // initialize with the default set
      c_size = 8;
      c_entries = (entry*)calloc(c_size, sizeof(entry));

      c_entries[0] = entry("10bb95a4-b1c5-444b-b512-1370f02d74de", "Red");
      c_entries[1] = entry("20bb95a4-b1c5-444b-b512-1370f02d74de", "Green");
      c_entries[2] = entry("30bb95a4-b1c5-444b-b512-1370f02d74de", "Black");
      c_entries[3] = entry("40bb95a4-b1c5-444b-b512-1370f02d74de", "Purple");
      c_entries[4] = entry("50bb95a4-b1c5-444b-b512-1370f02d74de", "Orange");
      c_entries[5] = entry("60bb95a4-b1c5-444b-b512-1370f02d74de", "Blue");
      c_entries[6] = entry("70bb95a4-b1c5-444b-b512-1370f02d74de", "Yellow");
      c_entries[7] = entry("80bb95a4-b1c5-444b-b512-1370f02d74de", "Pink");
    }

    TiltDB::~TiltDB() {
      if ( c_entries ) free(c_entries);
    }

    void TiltDB::marshall(ryml::NodeRef& _node) {
    }

    void TiltDB::unmarshall(ryml::ConstNodeRef& _node) {
    }

    std::shared_ptr<TiltDB> TiltDB::getInstance() {
      static std::shared_ptr<TiltDB> instance{new TiltDB()};
      return instance;
    }

    std::list<TiltDB::entry> TiltDB::getEntries() const {
      std::list<entry> retval;
      {
	std::shared_lock l(c_mutex);
	for (int i=0; i<c_size; ++i) {
	  retval.emplace_back(c_entries[i]);
	}
      }
      return retval;
    }
  }
}
