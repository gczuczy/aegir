
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "SensorCache.hh"
#include "ZMQConfig.hh"
#include "Message.hh"

namespace aegir {
  namespace fermd {
    SensorCache::SensorCache(): aegir::Thread(), aegir::Service() {
    }

    SensorCache::~SensorCache() {
    }

    void SensorCache::init() {
      c_sock = ServiceManager::get<ZMQConfig>()->dstSocket("sensorbus");
      c_sock->setRecvTimeout(100);
      c_sock->subscribe("");
      c_sock->brrr();
    }

    void SensorCache::worker() {
      message_type msg;

      while ( c_run ) {
	if ( !(msg = c_sock->recv(true)) ) continue;

	if ( msg->group() == TiltReadingMessage::msg_group &&
	     msg->type() == TiltReadingMessage::msg_type ) {
	  auto tiltmsg = msg->as<TiltReadingMessage>();
#if 0
	  printf("SensorCache UUID:%s %.2fC %.4fSG\n",
		 boost::lexical_cast<std::string>(tiltmsg->uuid()).c_str(),
		 tiltmsg->temp(), tiltmsg->sg());
#endif
	  std::unique_lock g(c_mtx);
	  bool found=false;

	  for (auto& it: c_tiltreadings) {
	    if ( it.uuid == tiltmsg->uuid() ) {
	      it.time = tiltmsg->time();
	      it.temp = tiltmsg->temp();
	      it.sg   = tiltmsg->sg();
	      found = true;
	      break;
	    }
	  }
	  if ( !found ) {
	    tiltreading tr;
	    tr.uuid = tiltmsg->uuid();
	    tr.time = tiltmsg->time();
	    tr.temp = tiltmsg->temp();
	    tr.sg   = tiltmsg->sg();
	    c_tiltreadings.push_back(tr);
	  }
	}
      }
    }

    void SensorCache::stop() {
      c_run = false;
    }

    void SensorCache::bailout() {
      stop();
    }

    SensorCache::tiltreadings SensorCache::getTiltReadings() const {
      std::shared_lock g(c_mtx);
      return c_tiltreadings;
    }
  }
}
