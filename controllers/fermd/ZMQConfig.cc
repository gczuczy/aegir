
#include "ZMQConfig.hh"

namespace aegir {
  namespace fermd {

    ZMQConfig::ZMQConfig(): aegir::ZMQConfig() {

      // the sensor bus
      addSpec("sensorfetch",
	      aegir::ZMQConfig::zmq_proto::INPROC,
	      ZMQ_PUB, ZMQ_XSUB,
	      "sensorfetch", 0);
      addSpec("sensorbus",
	      aegir::ZMQConfig::zmq_proto::INPROC,
	      ZMQ_XPUB, ZMQ_SUB,
	      "sensorbus", 0,
	      true);
      addSpec("sensorctrl",
	      aegir::ZMQConfig::zmq_proto::INPROC,
	      ZMQ_PUB, ZMQ_SUB,
	      "sensorctrl", 0,
	      false);
      addProxy("sensorbus",
	       "sensorfetch", false,
	       "sensorbus", true,
	       "sensorctrl", false);

      // The Public Relations socket
      addSpec("prpublic",
	      aegir::ZMQConfig::zmq_proto::TCP,
	      ZMQ_REQ, ZMQ_ROUTER,
	      "*", 42691,
	      false);
      addSpec("prbus",
	      aegir::ZMQConfig::zmq_proto::INPROC,
	      ZMQ_DEALER, ZMQ_REP,
	      "prbus", 0,
	      true);
      addSpec("prctrl",
	      aegir::ZMQConfig::zmq_proto::INPROC,
	      ZMQ_PUB, ZMQ_SUB,
	      "prctrl", 0,
	      false);

      addProxy("pr",
	       "prpublic", false,
	       "prbus", true,
	       "prctrl", false);
    }

    ZMQConfig::~ZMQConfig() {
    }

    std::shared_ptr<aegir::fermd::ZMQConfig> ZMQConfig::getInstance() {
      static std::shared_ptr<ZMQConfig> instance{new ZMQConfig()};
      return instance;
    }
  }
}
