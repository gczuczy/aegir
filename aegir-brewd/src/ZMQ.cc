
#include "ZMQ.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exception.hh"
#include "JSONMessage.hh"

namespace aegir {
  static void zmqfree(void *_data, void*) {
    free(_data);
  }
  /*
   * ZMQ::Socket
   */

  ZMQ::Socket::Socket(SocketType _type): c_type(_type), c_sock(ZMQ::getInstance().getContext(), (int)c_type) {
  }

  ZMQ::Socket::~Socket() {
  }

  ZMQ::Socket &ZMQ::Socket::bind(const std::string &_addr) {
    c_sock.bind(_addr.c_str());
    return *this;
  }

  ZMQ::Socket &ZMQ::Socket::connect(const std::string &_addr) {
    c_sock.connect(_addr.c_str());
    return *this;
  }

  ZMQ::Socket &ZMQ::Socket::subscribe(const std::string &_filter) {
    c_sock.setsockopt(ZMQ_SUBSCRIBE, _filter.c_str(), _filter.size());
    return *this;
  }

  ZMQ::Socket &ZMQ::Socket::send(const Message &_msg) {
    auto msg = _msg.serialize();

    int size = msg.size();
    char *buff = (char*)malloc(size);
    memcpy((void*)buff, msg.data(), size);
    zmq::message_t zmsg(buff, size, zmqfree);

#ifdef AEGIR_DEBUG
    printf("Sending: %s\n", _msg.hexdebug().c_str());
#endif

    if ( !c_sock.send(zmsg, 0) ) {
      printf("Send failed\n");
    }

    return *this;
  }

  std::shared_ptr<Message> ZMQ::Socket::recv(MessageFormat _mf) {
    zmq::message_t zmsg;

    if ( !c_sock.recv(&zmsg, ZMQ_NOBLOCK) ) {
      return nullptr;
    }

    msgstring msg((uint8_t*)zmsg.data(), zmsg.size());
#ifdef AEGIR_DEBUG
    printf("ZMQ::Socket::recv(): %s\n", hexdump(msg).c_str());
#endif
    if ( _mf == MessageFormat::INTERNAL ) {
      try {
	return MessageFactory::getInstance().create(msg);
      }
      catch (Exception &e) {
	printf("MessageFactory unknown message type: %s", e.what());
	return nullptr;
      }
    } else if ( _mf == MessageFormat::JSON ) {
      return std::make_shared<JSONMessage>(msg);
    }
    return nullptr;
  }

  /*
   * ZMQ
   */
  ZMQ::ZMQ(): c_ctx(2) {
  }

  ZMQ::~ZMQ() {
  }

  ZMQ &ZMQ::getInstance() {
    static ZMQ instance;
    return instance;
  }

  zmq::context_t &ZMQ::getContext() {
    return c_ctx;
  }
}
