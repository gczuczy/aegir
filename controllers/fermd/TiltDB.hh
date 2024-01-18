/*
  Database of tilt hydrometers
 */

#ifndef AEGIR_FERMD_TILTDB_H
#define AEGIR_FERMD_TILTDB_H

#include <string>
#include <memory>
#include <cstdint>

#include <shared_mutex>

#include <boost/uuid/uuid.hpp>

#include "common/ConfigBase.hh"


namespace aegir {
  namespace fermd {

    typedef boost::uuids::uuid uuid_t;

    class TiltDB: public ConfigNode {
    public:
      // the struct for the stored data
      struct entry {
	struct calibration_t {
	  // in Specific Gravity
	  float raw;
	  float corrected;
	};
	uuid_t uuid;
	char color[16];
	bool active;
	calibration_t calibrations[2];

	std::string strUUID() const;
	void setUUID(const std::string& _uuid);
	void setColor(const std::string& _color);
	void calibrate(float _raw, float _corrected);
      };

    public:
      TiltDB();
      virtual ~TiltDB();

      virtual void marshall(ryml::NodeRef&);
      virtual void unmarshall(ryml::ConstNodeRef&);

      static std::shared_ptr<TiltDB> getInstance();

      std::list<entry> getEntries() const;

    private:
      void reset();
      entry& addEntry(const std::string& _uuid, const std::string& _color,
		      bool _active=false);

    private:
      uint32_t c_size;
      entry* c_entries;
      mutable std::shared_mutex c_mutex;
    };
  }
}

#endif
