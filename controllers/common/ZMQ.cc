
#include "common/ZMQ.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cerrno>

#include "common/Exception.hh"

namespace aegir {

  static std::string zmqTypeToStr(int _type) {
    std::string st;

    switch(_type) {
    case ZMQ_PUB:
      st = "PUB";
      break;
    case ZMQ_SUB:
      st = "SUB";
      break;
    case ZMQ_REQ:
      st = "REQ";
      break;
    case ZMQ_REP:
      st = "REP";
      break;
    case ZMQ_DEALER:
      st = "DEALER";
      break;
    case ZMQ_ROUTER:
      st = "ROUTER";
      break;
    default:
      st = "unknown";
    };
    return st;
  }

  /*
    ZMQctx
   */

  ZMQctx::ZMQctx(): c_ctx(0) {
    if ( (c_ctx = zmq_ctx_new()) == 0 )
      throw Exception("zmq_ctx_new failed");
  }

  ZMQctx::~ZMQctx() {
    zmq_ctx_term(c_ctx);
  }

  /*
    ZMQConfig
   */
  ZMQConfig::ZMQConfig(): ConfigNode(), c_specs(0), c_nspecs(0),
			  c_proxies(0), c_nproxies(0) {
    c_ctx = std::make_shared<ZMQctx>();
  }

  ZMQConfig::~ZMQConfig() {
    if ( c_specs ) free((void*)c_specs);
    if ( c_proxies ) free((void*)c_proxies);
  }

  void ZMQConfig::marshall(ryml::NodeRef& _node) {
    _node |= ryml::MAP;
    auto tree = _node.tree();

    _node["sockets"] |= ryml::MAP;
    auto sockets = _node["sockets"];

    for ( uint32_t i=0; i < c_nspecs; ++i ) {
      if ( c_specs[i].proto == zmq_proto::TCP ) {
	auto name = tree->to_arena(c_specs[i].name);
	auto address = tree->to_arena(c_specs[i].address);
	sockets[name] |= ryml::MAP;
	sockets[name]["address"] << address;
	sockets[name]["port"] << c_specs[i].port;
      }
      // else we don't serialize it
    }

    // proxies only expose the debug flag
    // and only when there's a debug socket defined
    // on it
    _node["proxies"] |= ryml::MAP;
    auto proxies = _node["proxies"];

    for ( uint32_t i=0; i<c_nproxies; ++i ) {
      if ( c_proxies[i].has_dbg ) {
	auto name = tree->to_arena(c_proxies[i].name);
	proxies[name] |= ryml::MAP;
	proxies[name]["debug"] << c_proxies[i].debug;
      }
    }
  }

  void ZMQConfig::unmarshall(ryml::ConstNodeRef& _node) {
    if ( !_node.is_map() )
      throw Exception("zeromq node is not a map");

    if ( !_node.has_child("sockets") )
	 throw Exception("zeromq.sockets missing");

    // sockets
    auto sockets = _node["sockets"];
    if ( !sockets.is_map() )
      throw Exception("zeromq.sockets is not a map");

    for (ryml::ConstNodeRef node: sockets.children()) {
      auto ckey = node.key();
      std::string key = std::string(ckey.data(), ckey.size());
      uint32_t idx = getSocketIndex(key);

      if ( node.has_child("address") ) {
	auto addr = node["address"];
	if ( !addr.has_val() )
	  throw Exception("Address has no value");

	std::string address;
	addr >> address;
	if ( address.size() > 63 )
	  throw Exception("Address(%s) longer than 63", address.c_str());
	memset((void*)&c_specs[idx].address, 0, sizeof(c_specs[idx].address));
	strncpy(c_specs[idx].address, address.c_str(), address.size());
      }

      if ( node.has_child("port") ) {
	auto portnode = node["port"];
	if ( !portnode.has_val() )
	  throw Exception("Address has no value");

	portnode >> c_specs[idx].port;
      }
    }

    // proxies
    if ( !_node.has_child("proxies") )
	 throw Exception("zeromq.proxies missing");

    auto proxies = _node["proxies"];
    if ( !proxies.is_map() )
      throw Exception("zeromq.sockets is not a map");

    for (ryml::ConstNodeRef node: proxies.children()) {
      auto ckey = node.key();
      std::string key = std::string(ckey.data(), ckey.size());
      uint32_t idx = getProxyIndex(key);

      // options we're supporting
      if ( node.has_child("debug") ) {
	node["debug"] >> c_proxies[idx].debug;
      }
    }
  }

  zmqsocket_type ZMQConfig::srcSocket(const std::string& _conn) {
    auto idx = getSocketIndex(_conn);

    return std::shared_ptr<ZMQSocket>{new ZMQSocket(c_ctx,
						    c_specs[idx].src_type,
						    connString(c_specs[idx]),
						    c_specs[idx].src_binds)};
  }

  zmqsocket_type ZMQConfig::dstSocket(const std::string& _conn) {
    auto idx = getSocketIndex(_conn);

    return std::shared_ptr<ZMQSocket>{new ZMQSocket(c_ctx,
						    c_specs[idx].dst_type,
						    connString(c_specs[idx]),
						    !c_specs[idx].src_binds)};
  }

  zmqproxy_type ZMQConfig::proxy(const std::string& _name) {
    auto idx = getProxyIndex(_name);

    zmqsocket_type dbg(nullptr);
    if ( c_proxies[idx].has_dbg && c_proxies[idx].debug ) {
      auto n = c_proxies[idx].dbg.idx;
      dbg = getSocket(c_specs[n].name, c_specs[n].src_binds);
    }

    auto srcidx = c_proxies[idx].src.idx;
    auto dstidx = c_proxies[idx].dst.idx;
    auto ctrlidx = c_proxies[idx].ctrl.idx;
    return std::shared_ptr<ZMQProxy>{
      new ZMQProxy(shared_from_this(),
		   getSocket(c_specs[srcidx].name, c_specs[srcidx].src_binds),
		   getSocket(c_specs[dstidx].name, c_specs[dstidx].src_binds),
		   getSocket(c_specs[ctrlidx].name, c_specs[ctrlidx].src_binds),
		   dbg)};
  }

  void ZMQConfig::addSpec(const std::string& _name,
			  zmq_proto _proto,
			  int _srctype, int _dsttype,
			  const std::string& _address,
			  const uint16_t _port,
			  bool _srcbind) {
    if ( _name.size()>15 )
      throw Exception("ZMQ conn spec name longer than 15");

    if ( _address.size()>63 )
      throw Exception("ZMQ conn spec address longer than 63");

    // verify the working pairs - only what the stack is using
    if ( _srctype == ZMQ_PUB ) {
      if ( _dsttype != ZMQ_SUB && _dsttype != ZMQ_XSUB )
	throw Exception("PUB needs SUB or XSUB");
    } else if ( _srctype == ZMQ_SUB ) {
      if ( _dsttype != ZMQ_PUB && _dsttype != ZMQ_XPUB )
	throw Exception("SUB needs PUB XPUB");
    } else if ( _srctype == ZMQ_XPUB ) {
      if ( _dsttype != ZMQ_SUB && _dsttype != ZMQ_XSUB )
	throw Exception("XPUB needs SUB or XSUB");
    } else if ( _srctype == ZMQ_XSUB ) {
      if ( _dsttype != ZMQ_PUB && _dsttype != ZMQ_XPUB )
	throw Exception("XSUB needs PUB or XPUB");
    } else if ( _srctype == ZMQ_REQ ) {
      if ( _dsttype != ZMQ_REP && _dsttype != ZMQ_ROUTER )
	throw Exception("REQ needs REP or ROUTER");
    } else if ( _srctype == ZMQ_REP ) {
      if ( _dsttype != ZMQ_REQ && _dsttype != ZMQ_DEALER )
	throw Exception("REP needs REQ or DEALER");
    } else if ( _srctype == ZMQ_ROUTER ) {
      if ( _dsttype != ZMQ_DEALER && _dsttype != ZMQ_REQ
	   && _dsttype != ZMQ_ROUTER )
	throw Exception("ROUTER needs DEALER, REQ or ROUTER");
    } else if ( _srctype == ZMQ_DEALER ) {
      if ( _dsttype != ZMQ_DEALER && _dsttype != ZMQ_REP
	   && _dsttype != ZMQ_ROUTER )
	throw Exception("DEALER needs DEALER, REP or ROUTER");
    } else {
      throw Exception("Unknown src socket type");
    }

    // verify the proto and the addresses
    if ( _proto == zmq_proto::INPROC ) {
      // nothing to check
    } else if ( _proto == zmq_proto::TCP ) {
      // nothing to check
    } else {
      throw Exception("zmq proto type not implemented");
    }

    // now allocate memory for the entry
    uint32_t n = c_nspecs++;
    c_specs = (conn_spec*)realloc((void*)c_specs,
				  sizeof(conn_spec)*c_nspecs);
    memset((void*)&c_specs[n], 0, sizeof(conn_spec));

    memcpy((void*)&c_specs[n].address,
	   (void*)_address.data(), _address.size());
    memcpy((void*)&c_specs[n].name,
	   (void*)_name.data(), _name.size());
    c_specs[n].src_type = _srctype;
    c_specs[n].dst_type = _dsttype;
    c_specs[n].port = _port;
    c_specs[n].proto = _proto;
    c_specs[n].src_binds = _srcbind;
  }

  void ZMQConfig::addProxy(const std::string& _name,
			   const std::string& _src_name, bool _src_src,
			   const std::string& _dst_name, bool _dst_src,
			   const std::string& _ctrl_name, bool _ctrl_src,
			   const std::string& _dbg_name, bool _dbg_src) {
    std::uint32_t srcidx, dstidx, ctrlidx, dbgidx(0);

    bool has_dbg = _dbg_name!="";

    if ( _name.length() <2 )
      throw Exception("ZMQ proxy name too short");

    srcidx = getSocketIndex(_src_name);
    dstidx = getSocketIndex(_dst_name);
    ctrlidx = getSocketIndex(_ctrl_name);
    if ( has_dbg) dbgidx = getSocketIndex(_dbg_name);

    uint32_t n = c_nproxies++;
    c_proxies = (proxy_spec*)realloc((void*)c_proxies,
				     sizeof(proxy_spec)*c_nproxies);
    memset((void*)&c_proxies[n], 0, sizeof(proxy_spec));

    c_proxies[n].src.idx = srcidx;
    c_proxies[n].src.source = _src_src;
    c_proxies[n].dst.idx = dstidx;
    c_proxies[n].dst.source = _dst_src;
    c_proxies[n].ctrl.idx = ctrlidx;
    c_proxies[n].ctrl.source = _ctrl_src;
    if ( has_dbg ) {
      c_proxies[n].dbg.idx = dbgidx;
      c_proxies[n].dbg.source = _dbg_src;
      c_proxies[n].has_dbg = true;
    } else {
      c_proxies[n].dbg.idx = 0;
      c_proxies[n].dbg.source = false;
      c_proxies[n].has_dbg = false;
    }
    c_proxies[n].debug = false;
  }

  uint32_t ZMQConfig::getSocketIndex(const std::string& _name) {
    for ( uint32_t i=0; i < c_nspecs; ++i ) {
      if ( _name == c_specs[i].name ) return i;
    }
    throw Exception("Unable to find specs for socket %s", _name.c_str());
  }

  uint32_t ZMQConfig::getProxyIndex(const std::string& _name) {
    for ( uint32_t i=0; i < c_nproxies; ++i ) {
      if ( _name == c_proxies[i].name ) return i;
    }
    throw Exception("Unable to find specs for proxy %s", _name.c_str());
  }

  std::string ZMQConfig::connString(const conn_spec& _spec) {
    std::string address;
    if ( _spec.proto == zmq_proto::TCP ) {
      char buff[128];
      auto len = snprintf(buff, sizeof(buff)-1, "tcp://%s:%u",
			  _spec.address,
			  _spec.port);
      address = std::string(buff, len);
    } else if ( _spec.proto == zmq_proto::INPROC ) {
      address = "inproc://";
      address += _spec.address;
    } else {
      throw Exception("Unknown socket type");
    }
    return address;
  }
  /*
    ZMQSocket
   */
  ZMQSocket::ZMQSocket(zmqctx_type& _ctx, int _type,
		       const std::string& _address, bool _bind)
    : c_ctx(_ctx), c_address(_address), c_bind(_bind) {
    void *ctx = _ctx->getCtx();

    c_sock = zmq_socket(ctx, _type);
  }

  ZMQSocket::~ZMQSocket() {
    zmq_close(c_sock);
  }

  void ZMQSocket::subscribe(const std::string& _str) {
    zmq_setsockopt(c_sock, ZMQ_SUBSCRIBE,
		   (const void*)_str.data(), _str.size());
  }

  void ZMQSocket::unsubscribe(const std::string& _str) {
    zmq_setsockopt(c_sock, ZMQ_UNSUBSCRIBE,
		   (const void*)_str.data(), _str.size());
  }

  void ZMQSocket::setRecvTimeout(int _to) {
    zmq_setsockopt(c_sock, ZMQ_RCVTIMEO,
		   (const void*)&_to, sizeof(int));
  }

  void ZMQSocket::setSendTimeout(int _to) {
    zmq_setsockopt(c_sock, ZMQ_SNDTIMEO,
		   (const void*)&_to, sizeof(int));
  }

  message_type ZMQSocket::recv(bool _wait){
    constexpr int buffsize = 65536;
    thread_local char buff[buffsize];

    int flags=0;
    if ( !_wait ) flags |= ZMQ_DONTWAIT;

    int rc = zmq_recv(c_sock, buff, buffsize-1, flags);
    if ( rc < 0 ) {
      if ( errno == EAGAIN ) return nullptr;
      int type;
      size_t s = sizeof(type);
      zmq_getsockopt(c_sock, ZMQ_TYPE, (void *)&type, &s);
      throw Exception("zmq_recv(%s): %s", zmqTypeToStr(type).c_str(),
		      std::strerror(errno));
    } else if ( rc == 0 ) {
      return nullptr;
    }

    return MessageFactory::getInstance()->parse(buff, rc);
  }

  bool ZMQSocket::recv(char *_buff, std::uint16_t _len, bool _wait) {
    int flags=0;
    if ( !_wait ) flags |= ZMQ_DONTWAIT;

    int rc = zmq_recv(c_sock, _buff, _len, flags);
    if ( rc < 0 ) {
      if ( errno == EAGAIN ) return false;
      int type;
      size_t s = sizeof(type);
      zmq_getsockopt(c_sock, ZMQ_TYPE, (void *)&type, &s);
      throw Exception("zmq_recv(%s): %s", zmqTypeToStr(type).c_str(),
		      std::strerror(errno));
    } else if ( rc == 0 ) {
      return false;
    }
    return true;
  }

  void ZMQSocket::send(const void *_buff, std::uint16_t _len, bool _wait) {
    int flags=0;
    if ( !_wait ) flags |= ZMQ_DONTWAIT;

    do {
      if ( zmq_send(c_sock, _buff, _len, flags) >= 0 ) break;
      if ( errno == EAGAIN ) continue;
      int type;
      size_t s = sizeof(type);
      zmq_getsockopt(c_sock, ZMQ_TYPE, (void *)&type, &s);
      throw Exception("zmq_send(%s): %s", zmqTypeToStr(type).c_str(),
		      std::strerror(errno));
    } while (true);
  }

  void ZMQSocket::brrr() {
    if ( c_bind ) {
      if ( zmq_bind(c_sock, c_address.c_str()) != 0 )
	throw Exception("ZMQ bind(%s) failed", c_address.c_str());
    } else {
      if ( zmq_connect(c_sock, c_address.c_str()) != 0 )
	throw Exception("ZMQ connect(%s) failed", c_address.c_str());
    }
  }

  /*
    ZMQProxy
   */
  ZMQProxy::ZMQProxy(zmqconfig_ptr _cfg,
		     zmqsocket_type _src,
		     zmqsocket_type _dst,
		     zmqsocket_type _ctrl,
		     zmqsocket_type _dbg)
    : c_zmqcfg(_cfg), c_src(_src), c_dst(_dst), c_ctrl(_ctrl), c_dbg(_dbg) {

  }

  ZMQProxy::~ZMQProxy() {
  }
}
