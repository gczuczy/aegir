/*
  Stores ZMQ socket configurations
 */

#ifndef AEGIR_FERMD_ZMQCONFIG
#define AEGIR_FERMD_ZMQCONFIG

#include "common/ZMQ.hh"
#include "common/ServiceManager.hh"

namespace aegir {
  namespace fermd {

    class ZMQConfig: public aegir::ZMQConfig,
		     public Service {
    public:
      ZMQConfig();
      ZMQConfig(const ZMQConfig&)=delete;
      ZMQConfig(ZMQConfig&&)=delete;
      virtual ~ZMQConfig();

      virtual void bailout();
    };
  }
}


#endif
