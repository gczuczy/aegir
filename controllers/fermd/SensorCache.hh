/*
  Sensor cache, to have last readings available on demand
 */

#ifndef AEGIR_FERMD_SENSORCACHE
#define AEGIR_FERMD_SENSORCACHE

#include <list>
#include <shared_mutex>

#include "common/ThreadManager.hh"
#include "common/ServiceManager.hh"
#include "common/ZMQ.hh"
#include "uuid.hh"

namespace aegir {
  namespace fermd {

    class SensorCache: public aegir::Thread,
		       public aegir::Service {
      friend class aegir::ServiceManager;
    public:
      struct tiltreading {
	uuid_t uuid;
	time_t time;
	float temp;
	float sg;
      };
      typedef std::list<tiltreading> tiltreadings;

    protected:
      SensorCache();
      SensorCache(const SensorCache&)=delete;
      SensorCache(SensorCache&&)=delete;

    public:
      virtual ~SensorCache();

      virtual void init();
      virtual void worker();
      virtual void stop();
      virtual void bailout();

      tiltreadings getTiltReadings() const;

    private:
      aegir::zmqsocket_type c_sock;
      mutable std::shared_mutex c_mtx;
      tiltreadings c_tiltreadings;
    };
  }
}

#endif
