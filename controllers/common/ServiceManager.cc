
#include "common/ServiceManager.hh"

namespace aegir {

  std::atomic<ServiceManager*> ServiceManager::c_instance{nullptr};

  Service::~Service() {
  }

  ServiceManager::ServiceManager() {
    if ( c_instance.load() != nullptr ) {
      throw Exception("ServiceManager already instantiated");
    }

    c_instance.store(this);
  }

  ServiceManager::~ServiceManager() {
    c_instance.store(nullptr);
  }
}
