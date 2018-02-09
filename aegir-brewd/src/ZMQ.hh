/*
 * High-level zeromq wrapper class for internal use
 */

#ifndef AEGIR_ZMQ_H
#define AEGIR_ZMQ_H

extern "C" {
#include <zmq.h>
}

#include "Message.hh"

namespace aegir {

  class ZMQ {
    friend class Socket;
  public:
    enum class SocketType {
      REQ = ZMQ_REQ,
	REP = ZMQ_REP,
	DEALER = ZMQ_DEALER,
	ROUTER = ZMQ_ROUTER,
	PUB = ZMQ_PUB,
	SUB = ZMQ_SUB,
	PUSH = ZMQ_PUSH,
	PULL = ZMQ_PULL,
	PAIR = ZMQ_PAIR
	};
    enum class MessageFormat {
      INTERNAL,
	JSON
	};

    class Socket {
      friend class ZMQ;
      Socket() = delete;

    public:
      Socket(SocketType _type);
      ~Socket();
      Socket &bind(const std::string &_addr);
      Socket &connect(const std::string &_addr);
      Socket &subscribe(const std::string &_filter);
      Socket &send(const std::string &_msg, bool _more=false);
      Socket &send(const Message &_msg, bool _more=false);
      std::shared_ptr<Message> recv(MessageFormat _mf = MessageFormat::INTERNAL);
      Socket &setIdentity(const std::string &_id);

    private:
      SocketType c_type;
      void *c_sock;
    };

  private:
    ZMQ(ZMQ &&) = delete;
    ZMQ(const ZMQ &) = delete;
    ZMQ &operator=(ZMQ &&) = delete;
    ZMQ &operator=(const ZMQ &) = delete;

  protected:
    ZMQ();
    ~ZMQ();
    void *getContext();

  private:
    //zmq::context_t c_ctx;
    void *c_ctx;

  public:
    static ZMQ &getInstance();
    int proxy(Socket &_frontend, Socket &_backend, Socket &_ctrl);
    int proxy(Socket &_frontend, Socket &_backend, Socket &_capture, Socket &_ctrl);
    void close();
  };
}

#endif
