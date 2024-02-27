/*
  Sensor bus proxy
 */

#ifndef AEGIR_FERMD_SENSORPROXY
#define AEGIR_FERMD_SENSORPROXY

#include "common/ThreadManager.hh"
#include "common/ZMQ.hh"
#include "common/ServiceManager.hh"

namespace aegir {
  namespace fermd {

    class SensorProxy;
    typedef std::shared_ptr<SensorProxy> sensorproxy_type;
    class SensorProxy: public aegir::Thread,
		       public Service {
      friend class aegir::ServiceManager;
    public:
      virtual ~SensorProxy();

      virtual void init();
      virtual void worker();
      virtual void stop();
      virtual void bailout();

    protected:
      SensorProxy();
      SensorProxy(const SensorProxy&)=delete;
      SensorProxy(SensorProxy&&)=delete;

    private:
      aegir::zmqproxy_type c_proxy;
    };
  }
}

#endif
