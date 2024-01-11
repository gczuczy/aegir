/*
  Database of tilt hydrometers
 */

#ifndef AEGIR_FERMD_TILTDB_H
#define AEGIR_FERMD_TILTDB_H

#include <uuid.h>

#include <string>
#include <memory>
#include <cstdint>

#include <shared_mutex>

#include "ConfigBase.hh"


namespace aegir {
  namespace fermd {

    class TiltDB: public ConfigNode {
    public:
      // the struct for the stored data
      struct entry {
	entry(const std::string& _uuid,
	      const std::string& _color,
	      bool _active=false);
	uuid_t uuid;
	char color[16];
	bool active;

	std::string strUUID() const;
      };

    public:
      TiltDB();
      virtual ~TiltDB();

      virtual void marshall(ryml::NodeRef&);
      virtual void unmarshall(ryml::ConstNodeRef&);

      static std::shared_ptr<TiltDB> getInstance();

      std::list<entry> getEntries() const;

    private:
      uint8_t c_size;
      entry* c_entries;
      mutable std::shared_mutex c_mutex;
    };
  }
}

#endif
