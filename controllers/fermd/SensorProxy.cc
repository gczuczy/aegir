
#include <stdio.h>

#include "SensorProxy.hh"
#include "ZMQConfig.hh"

namespace aegir {
  namespace fermd {

    SensorProxy::SensorProxy(): aegir::ThreadManager::Thread() {
      c_proxy = ZMQConfig::getInstance()->proxy("sensorbus");
    }

    SensorProxy::~SensorProxy() {
    }

    sensorproxy_type SensorProxy::getInstance() {
      static std::shared_ptr<SensorProxy> instance{new SensorProxy()};
      return instance;
    };

    void SensorProxy::init() {
    }

    void SensorProxy::worker() {
      while ( c_run ) c_proxy->run();
    }

    void SensorProxy::stop() {
      c_run = false;
      c_proxy->terminate();
    }
  }
}
