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

  // Base ZMQ Config handler facility
  // to be subclassed
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

  public:
    ZMQConfig();
    ZMQConfig(const ZMQConfig&)=delete;
    ZMQConfig(ZMQConfig&&)=delete;
    virtual ~ZMQConfig();

  public:
    virtual void marshall(ryml::NodeRef&);
    virtual void unmarshall(ryml::ConstNodeRef&);

  public:
    zmqsocket_type srcSocket(const std::string& _conn);
    zmqsocket_type dstSocket(const std::string& _conn);

  protected:
    void addSpec(const std::string& _name,
		 zmq_proto _proto,
		 int _srctype, int _dsttype,
		 const std::string& _address,
		 const uint16_t _port=0,
		 bool _srcbind=false);

  private:
    uint32_t getEntryIndex(const std::string& _name);

  private:
    conn_spec* c_specs;
    uint32_t c_nspecs;
    zmqctx_type c_ctx;
  };

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

    // makes the socket go brrr
    void brrr();

  private:
    void *c_sock;
    zmqctx_type c_ctx;
  };
}

#endif
