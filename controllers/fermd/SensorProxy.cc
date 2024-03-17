
#include <stdio.h>

#include "SensorProxy.hh"
#include "ZMQConfig.hh"

namespace aegir {
  namespace fermd {

    SensorProxy::SensorProxy(): aegir::Thread(), Service() {
      c_proxy = ServiceManager::get<ZMQConfig>()->proxy("sensorbus");
    }

    SensorProxy::~SensorProxy() {
    }

    void SensorProxy::init() {
    }

    void SensorProxy::worker() {
      while ( c_run ) c_proxy->run();
    }

    void SensorProxy::stop() {
      c_run = false;
      c_proxy->terminate();
    }

    void SensorProxy::bailout() {
      stop();
    }
  }
}
