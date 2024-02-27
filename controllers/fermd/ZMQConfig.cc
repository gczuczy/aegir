
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
      addProxy("sensorbus",
	       "sensorfetch", false,
	       "sensorbus", true);

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

      addProxy("pr",
	       "prpublic", false,
	       "prbus", true);
    }

    ZMQConfig::~ZMQConfig() {
    }

    void ZMQConfig::bailout() {
    }
  }
}
