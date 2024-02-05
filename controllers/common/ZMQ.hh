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
  class ZMQConfig: public ConfigNode,
		   public std::enable_shared_from_this<ZMQConfig> {
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
      proxy_node src;
      proxy_node dst;
      proxy_node ctrl;
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
		  const std::string& _src_name, bool _src_src,
		  const std::string& _dst_name, bool _dst_src,
		  const std::string& _ctrl_name, bool _ctrl_src,
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
  class ZMQSocket {
    friend aegir::ZMQConfig;

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
    inline void send(Message& _msg, bool _wait=false) {
      send(_msg.serialize(), _msg.size(), _wait);
    }
    inline void send(const std::string& _buff, bool _wait=false) {
      send((const void*)_buff.data(), _buff.size(), _wait);
    };
    void send(const void *_buff, std::uint16_t _len, bool _wait=false);

    // makes the socket go brrr
    void brrr();

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
    ZMQProxy(zmqconfig_ptr _cfg,
	     zmqsocket_type _src,
	     zmqsocket_type _dst,
	     zmqsocket_type _ctrl,
	     zmqsocket_type _dbg);
    ZMQProxy()=delete;
    ZMQProxy(const ZMQProxy&) = delete;
    ZMQProxy(ZMQProxy&&)=delete;
  public:
    ~ZMQProxy();

  private:
    zmqconfig_ptr c_zmqcfg;
    zmqsocket_type c_src, c_dst, c_ctrl, c_dbg;
  };
}

#endif
