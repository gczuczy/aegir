
#include "ZMQ.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  ZMQ::Socket &ZMQ::Socket::send(const Message &_msg) {
    auto msg = _msg.serialize();

    int size = msg.size();
    char *buff = (char*)malloc(size);
    memcpy((void*)buff, msg.data(), size);
    zmq::message_t zmsg(buff, size, zmqfree);

    printf("Sending: %s\n", _msg.hexdebug().c_str());

    c_sock.send(zmsg, 0);

    return *this;
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
