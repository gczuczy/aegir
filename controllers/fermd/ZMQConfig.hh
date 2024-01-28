/*
  Stores ZMQ socket configurations
 */

#ifndef AEGIR_FERMD_ZMQCONFIG
#define AEGIR_FERMD_ZMQCONFIG

#include "common/ZMQ.hh"

namespace aegir {
  namespace fermd {

    class ZMQConfig: public aegir::ZMQConfig {
    public:
      ZMQConfig();
      ZMQConfig(const ZMQConfig&)=delete;
      ZMQConfig(ZMQConfig&&)=delete;
      virtual ~ZMQConfig();
      static std::shared_ptr<ZMQConfig> getInstance();
    };
  }
}


#endif
