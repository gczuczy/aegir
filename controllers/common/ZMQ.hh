/*
  ZMQ base layer
 */

#ifndef AEGIR_ZMQ_H
#define AEGIR_ZMQ_H

#include <cstdint>
#include <string>
#include <memory>

#include <zmq.h>

#include "common/ConfigBase.hh"
#include "common/Message.hh"

namespace aegir {

  // The global ZMQ context guard
  class ZMQctx {
  public:
    ZMQctx();
    ZMQctx(const ZMQctx&)=delete;
    ZMQctx(ZMQctx&&)=delete;
    ~ZMQctx();

    inline void *getCtx() const {return c_ctx;};
  private:
    void* c_ctx;
  };
  typedef std::shared_ptr<ZMQctx> zmqctx_type;

  class ZMQSocket;
  typedef std::shared_ptr<ZMQSocket> zmqsocket_type;

  class ZMQProxy;
  typedef std::shared_ptr<ZMQProxy> zmqproxy_type;

  // Base ZMQ Config handler facility
  // to be subclassed
  class ZMQConfig;
  typedef std::shared_ptr<ZMQConfig> zmqconfig_ptr;
  class ZMQConfig: public ConfigNode {
  public:
    enum class zmq_proto {
      INPROC=0,
      TCP,
    };
  private:
    struct alignas(sizeof(long)) conn_spec {
      char address[64];
      char name[16];
      int src_type;
      int dst_type;
      std::uint16_t port;
      zmq_proto proto;
      bool src_binds;
    };

    struct alignas(sizeof(long)) proxy_node {
      std::uint32_t idx;
      bool source;
    };

    struct alignas(sizeof(long)) proxy_spec {
      char name[16];
      proxy_node front;
      proxy_node back;
      proxy_node dbg;
      bool has_dbg;
      bool debug;
    };

  protected:
    ZMQConfig();
  public:
    ZMQConfig(const ZMQConfig&)=delete;
    ZMQConfig(ZMQConfig&&)=delete;
    virtual ~ZMQConfig();

  public:
    virtual void marshall(ryml::NodeRef&);
    virtual void unmarshall(ryml::ConstNodeRef&);

  public:
    zmqsocket_type srcSocket(const std::string& _conn);
    zmqsocket_type dstSocket(const std::string& _conn);
    inline zmqsocket_type getSocket(const std::string& _conn, bool _src_binds) {
      return _src_binds ? srcSocket(_conn) : dstSocket(_conn);
    };
    zmqproxy_type proxy(const std::string& _name);

  protected:
    void addSpec(const std::string& _name,
		 zmq_proto _proto,
		 int _srctype, int _dsttype,
		 const std::string& _address,
		 const uint16_t _port=0,
		 bool _srcbind=false);
    void addProxy(const std::string& _name,
		  const std::string& _front_name, bool _front_src,
		  const std::string& _back_name, bool _back_src,
		  const std::string& _dbg_name="", bool _dbg_src=false);

  private:
    uint32_t getSocketIndex(const std::string& _name);
    uint32_t getProxyIndex(const std::string& _name);
    std::string connString(const conn_spec& _spec);

  private:
    conn_spec* c_specs;
    uint32_t c_nspecs;
    proxy_spec* c_proxies;
    uint32_t c_nproxies;
    zmqctx_type c_ctx;
  };

  /*
    ZMQSocket
   */
  class ZMQProxy;
  class ZMQSocket {
    friend aegir::ZMQConfig;
    friend aegir::ZMQProxy;

  protected:
    ZMQSocket()=delete;
    ZMQSocket(const ZMQSocket&)=delete;
    ZMQSocket(ZMQSocket&&)=delete;
    ZMQSocket(zmqctx_type& _ctx, int _type,
	      const std::string& _address, bool _bind);
  public:
    ~ZMQSocket();

    // sock opts
    void subscribe(const std::string& _str);
    void unsubscribe(const std::string& _str);
    void setRecvTimeout(int _to);
    void setSendTimeout(int _to);

    // message
    message_type recv(bool _wait=false);
    bool recv(char *_buff, std::uint16_t _len, bool _wait=false);
    inline void send(message_type _msg, bool _wait=false) {
      send(_msg->serialize(), _msg->size(), _wait);
    }
    inline void send(const Message& _msg, bool _wait=false) {
      send(_msg.serialize(), _msg.size(), _wait);
    }
    inline void send(const std::string& _buff, bool _wait=false) {
      send((const void*)_buff.data(), _buff.size(), _wait);
    };
    void send(const void *_buff, std::uint16_t _len, bool _wait=false);

    // makes the socket go brrr
    void brrr();

  protected:
    inline void* nativeSocket() const { return c_sock; };

  private:
    void *c_sock;
    zmqctx_type c_ctx;
    std::string c_address;
    bool c_bind;
  };

  /*
    ZMQ Proxy
   */
  class ZMQProxy {
    friend aegir::ZMQConfig;

    protected:
    ZMQProxy(zmqctx_type _ctx,
	     zmqsocket_type _front,
	     zmqsocket_type _back,
	     zmqsocket_type _dbg);
    ZMQProxy()=delete;
    ZMQProxy(const ZMQProxy&) = delete;
    ZMQProxy(ZMQProxy&&)=delete;
  public:
    ~ZMQProxy();

    void run();
    void terminate();

  private:
    static std::atomic<std::uint32_t> c_control_index;
    zmqctx_type c_ctx;
    zmqsocket_type c_front, c_back, c_dbg;
    void *c_ctrl;
    void *c_ctrlclient;
  };
}

#endif
