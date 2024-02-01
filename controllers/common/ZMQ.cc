
#include "common/ZMQ.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/Exception.hh"

namespace aegir {

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
  ZMQConfig::ZMQConfig(): ConfigNode(), c_specs(0), c_nspecs(0) {
    c_ctx = std::make_shared<ZMQctx>();
  }

  ZMQConfig::~ZMQConfig() {
    if ( c_specs ) free((void*)c_specs);
  }

  void ZMQConfig::marshall(ryml::NodeRef& _node) {
    _node |= ryml::MAP;
    auto tree = _node.tree();

    for ( uint32_t i=0; i < c_nspecs; ++i ) {
      if ( c_specs[i].proto == zmq_proto::TCP ) {
	auto name = tree->to_arena(c_specs[i].name);
	auto address = tree->to_arena(c_specs[i].address);
	_node[name] |= ryml::MAP;
	_node[name]["address"] << address;
	_node[name]["port"] << c_specs[i].port;
      }
      // else we don't serialize it
    }
  }

  void ZMQConfig::unmarshall(ryml::ConstNodeRef& _node) {
    if ( !_node.is_map() )
      throw Exception("zeromq node is not a map");

    for (ryml::ConstNodeRef node: _node.children()) {
      auto ckey = node.key();
      std::string key = std::string(ckey.data(), ckey.size());
      uint32_t idx = getEntryIndex(key);
    }
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
    //memset((void*)&c_specs[n], 0, sizeof(conn_spec));

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

  uint32_t ZMQConfig::getEntryIndex(const std::string& _name) {
    for ( uint32_t i=0; i < c_nspecs; ++i ) {
      if ( _name == c_specs[i].name ) return i;
    }
    throw Exception("Unable to find specs for connection %s", _name.c_str());
  }
}
