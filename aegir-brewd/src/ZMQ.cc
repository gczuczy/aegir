
#include "ZMQ.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "Exception.hh"
#include "JSONMessage.hh"

namespace aegir {
  static void zmqfree(void *_data, void*) {
    free(_data);
  }
  /*
   * ZMQ::Socket
   */

  ZMQ::Socket::Socket(SocketType _type): c_type(_type), c_sock(0) {
    void *ctx = ZMQ::getInstance().getContext();
    c_sock = zmq_socket(ctx, (int)c_type);
#if 0
    printf("ZMQ::Socket::Socket() on context %p: %p\n", ctx, c_sock);
#endif
    if ( !c_sock )
      throw Exception("zmq_socket failed: %i/%s", errno, strerror(errno));
  }

  ZMQ::Socket::~Socket() {
    zmq_close(c_sock);
  }

  ZMQ::Socket &ZMQ::Socket::bind(const std::string &_addr) {
    int rc = zmq_bind(c_sock, _addr.c_str());
    if ( rc != 0 )
      throw Exception("zmq_bind(%p, %s):%i failed: %i/%s", c_sock, _addr.c_str(), rc, errno, strerror(errno));
    return *this;
  }

  ZMQ::Socket &ZMQ::Socket::connect(const std::string &_addr) {
    int rc = zmq_connect(c_sock, _addr.c_str());
    if ( rc != 0 )
      throw Exception("zmq_connect(%p, %s):%i failed: %i/%s", c_sock, _addr.c_str(), rc, errno, strerror(errno));
    return *this;
  }

  ZMQ::Socket &ZMQ::Socket::subscribe(const std::string &_filter) {
    int rc = zmq_setsockopt(c_sock, ZMQ_SUBSCRIBE,
			    _filter.c_str(), _filter.size());
    if ( rc != 0 )
      throw Exception("zmq_setsockopt(%p):%i failed: %i/%s", c_sock, rc, errno, strerror(errno));
    return *this;
  }

  ZMQ::Socket &ZMQ::Socket::send(const std::string &_msg, bool _more) {
    int flags = 0;
    if ( _more ) flags |= ZMQ_SNDMORE;
    if ( zmq_send(c_sock, _msg.data(), _msg.size(), flags) < 0)
      throw Exception("zmq_send(%p) failed: %i/%s", c_sock, errno, strerror(errno));

    return *this;
  }

  ZMQ::Socket &ZMQ::Socket::send(const Message &_msg, bool _more) {
    auto msg = _msg.serialize();

#ifdef AEGIR_DEBUG
    printf("Sending: %s\n", _msg.hexdebug().c_str());
#endif

    int flags = 0;
    if ( _more ) flags |= ZMQ_SNDMORE;
    if ( zmq_send(c_sock, msg.data(), msg.size(), flags) < 0)
      throw Exception("zmq_send(%p) failed: %i/%s", c_sock, errno, strerror(errno));

    return *this;
  }

  std::shared_ptr<Message> ZMQ::Socket::recv(MessageFormat _mf) {
    zmq_msg_t zmsg;
    zmq_msg_init(&zmsg);

    if ( zmq_msg_recv(&zmsg, c_sock, ZMQ_DONTWAIT) < 0) {
      zmq_msg_close(&zmsg);
      return nullptr;
    }

    msgstring msg((uint8_t*)zmq_msg_data(&zmsg), zmq_msg_size(&zmsg));
    zmq_msg_close(&zmsg);
#ifdef AEGIR_DEBUG
    printf("ZMQ::Socket::recv(): %s\n", hexdump(msg).c_str());
#endif
    if ( _mf == MessageFormat::INTERNAL ) {
      try {
	return MessageFactory::getInstance().create(msg);
      }
      catch (Exception &e) {
	//printf("MessageFactory unknown message type(%i): %s\n", __LINE__, e.what());
	return nullptr;
      }
    } else if ( _mf == MessageFormat::JSON ) {
	return std::make_shared<JSONMessage>(msg);
    }
    return nullptr;
  }

  ZMQ::Socket &ZMQ::Socket::setIdentity(const std::string &_id) {
    int rc = zmq_setsockopt(c_sock, ZMQ_IDENTITY,
			    _id.c_str(), _id.length());
    if ( rc != 0)
      throw Exception("zmq_setsockopt(%p):%i failed: %i/%s", c_sock, rc, errno, strerror(errno));
    return *this;
  }

  /*
   * ZMQ
   */
  ZMQ::ZMQ() {
    c_ctx = zmq_ctx_new();

    if ( !c_ctx )
      throw Exception("zmq init failed: %i/%s", errno, strerror(errno));

    zmq_ctx_set(c_ctx, ZMQ_IO_THREADS, 2);

#if 0
    printf("Got zmq context: %p\n", c_ctx);
#endif
  }

  ZMQ::~ZMQ() {
    if ( c_ctx ) zmq_ctx_term(c_ctx);
  }

  ZMQ &ZMQ::getInstance() {
    static ZMQ instance;
    return instance;
  }

  void *ZMQ::getContext() {
    return c_ctx;
  }

  int ZMQ::proxy(ZMQ::Socket &_frontend, ZMQ::Socket &_backend, ZMQ::Socket &_ctrl) {
    return zmq_proxy_steerable( _frontend.c_sock, _backend.c_sock, 0, _ctrl.c_sock);
  }

  int ZMQ::proxy(ZMQ::Socket &_frontend, ZMQ::Socket &_backend, ZMQ::Socket &_capture, ZMQ::Socket &_ctrl) {
    return zmq_proxy_steerable( _frontend.c_sock, _backend.c_sock, _capture.c_sock, _ctrl.c_sock);
  }

  void ZMQ::close() {
    zmq_ctx_term(c_ctx);
    c_ctx = 0;
  }
}
