
#ifndef AEGIR_FERMD_DB_TYPES
#define AEGIR_FERMD_DB_TYPES

#include <string>
#include <memory>
#include <list>

#include "uuid.hh"

#define DBTASSERT(T) \
  static_assert(std::copy_constructible<T>, \
		#T" is not copy constructable"); \
  static_assert(std::move_constructible<T>,	\
		#T" is not move constructable")

#define DBTPTRS(T) \
  typedef std::shared_ptr<T> ptr; \
  typedef std::shared_ptr<const T> cptr

namespace aegir {
  namespace fermd {
    namespace DB {

      class Result;

      // fermenter types
      struct fermenter_types {
	DBTPTRS(fermenter_types);
	fermenter_types& operator=(Result&);
	int id;
	int capacity;
	std::string name;
	std::string imageurl;
      };
      typedef std::list<fermenter_types::ptr> fermenter_types_db;
      typedef const std::list<fermenter_types::cptr> fermenter_types_cdb;
      DBTASSERT(fermenter_types);

      // fermenters
      struct fermenter {
	DBTPTRS(fermenter);
	fermenter& operator=(Result&);
	int id;
	std::string name;
	fermenter_types::cptr fermenter_type;
      };
      typedef std::list<fermenter::ptr> fermenter_db;
      typedef const std::list<fermenter::cptr> fermenter_cdb;
      DBTASSERT(fermenter);

      // Tilt Hydrometer
      struct tilthydrometer {
	DBTPTRS(tilthydrometer);
	struct calibration {
	  float at;
	  float sg;
	};
	tilthydrometer& operator=(Result&);
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
      DBTASSERT(tilthydrometer_cdb);
    } // ns DB
  } // ns fermd
} // ns aegir

#endif
