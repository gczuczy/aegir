
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

#define DBTLISTS(T) \
  typedef std::list<T::ptr> T##_db; \
  typedef const std::list<T::cptr> T##_cdb

#define DBTOPS(T) \
  ryml::NodeRef& operator<<(ryml::NodeRef&, const T&);	\
  ryml::ConstNodeRef& operator>>(ryml::ConstNodeRef&, T&)

#define DBTSTUFF(T) \
  DBTASSERT(T); \
  DBTLISTS(T); \
  DBTOPS(T);

namespace c4 {
  namespace yml {
    class NodeRef;
    class ConstNodeRef;
  }
}

namespace ryml {
  using c4::yml::NodeRef;
  using c4::yml::ConstNodeRef;
}

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
      DBTSTUFF(fermenter_types);

      // fermenters
      struct fermenter {
	DBTPTRS(fermenter);
	fermenter& operator=(Result&);
	int id;
	std::string name;
	fermenter_types::cptr fermenter_type;
      };
      DBTSTUFF(fermenter);

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
	fermenter::cptr fermenter;
      };
      DBTSTUFF(tilthydrometer);
    } // ns DB
  } // ns fermd
} // ns aegir

#endif
