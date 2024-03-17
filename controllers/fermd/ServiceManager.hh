
/*
  Fermd's servicemanager
 */

#ifndef AEGIR_FERMD_SERVICEMANAGER
#define AEGIR_FERMD_SERVICEMANAGER

#include "common/ServiceManager.hh"

namespace aegir {
  namespace fermd {

    class ServiceManager: public aegir::ServiceManager {

    public:
      ServiceManager();
      virtual ~ServiceManager();
    };
  }
}

#endif
