#ifndef AEGIR_FERMD_UUID
#define AEGIR_FERMD_UUID

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace aegir {
  namespace fermd {
    extern boost::uuids::string_generator g_uuidstrgen;
    typedef boost::uuids::uuid uuid_t;
  }
}

#endif
