
#ifndef AEGIR_FERMD_DB_TYPES
#define AEGIR_FERMD_DB_TYPES

#include <string>
#include <memory>
#include <list>

#include "uuid.hh"

namespace aegir {
  namespace fermd {
    namespace DB {

      // Tilt Hydrometer
      struct tilthydrometer {
	typedef std::shared_ptr<tilthydrometer> ptr;
	typedef std::shared_ptr<const tilthydrometer> cptr;
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
      typedef const std::list<tilthydrometer::cptr> tilthydrometer_cdb;
      static_assert(std::copy_constructible<tilthydrometer>,
		    "Tilthydrometer is not copy constructable");
      static_assert(std::move_constructible<tilthydrometer>,
		    "Tilthydrometer is not move constructable");
    } // ns DB
  } // ns fermd
} // ns aegir

#endif
