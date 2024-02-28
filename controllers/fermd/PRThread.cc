
#include "PRThread.hh"
#include "ZMQConfig.hh"
#include "Message.hh"
#include "common/ServiceManager.hh"
#include "DBConnection.hh"

#define PRCMD(NAME) \
  void PRThread::handle_##NAME(ryml::ConstNodeRef& _req, ryml::NodeRef& _rep)

#define REGCMD(NAME) \
  c_handlers[#NAME] = std::bind(&PRThread::handle_##NAME, this, \
				std::placeholders::_1,		\
				std::placeholders::_2)

namespace aegir {
  namespace fermd {

    PRThread::PRThread(): ThreadPool(), Service(), LogChannel("PR") {
      c_proxy = ServiceManager::get<ZMQConfig>()->proxy("pr");

      REGCMD(getFermenterTypes);
      REGCMD(getFermenters);
      REGCMD(getTilthydrometers);
    }

    PRThread::~PRThread() {
    }

    void PRThread::init() {
      trace("PR thread initializing");
    }

    void PRThread::stop() {
      info("PR thread stopping");
      c_run = false;
      c_proxy->terminate();
    }

    void PRThread::bailout() {
      stop();
    }

    void PRThread::controller() {
      info("PR proxy starting");
      while ( c_run ) c_proxy->run();
    }

    void PRThread::worker(std::atomic<bool>& _run) {
      std::string command;
      auto zmqsock = ServiceManager::get<ZMQConfig>()->dstSocket("prbus");
      zmqsock->setSendTimeout(100);
      zmqsock->setRecvTimeout(100);

      zmqsock->brrr();

      info("PR worker starting");
      while ( c_run && _run) {
	auto msg = zmqsock->recvRaw(true);

	if ( msg == nullptr ) continue;
	trace("Received message, size: %u", msg->size());

	ryml::Tree resptree;
	ryml::NodeRef resproot = resptree.rootref();
	resproot |= ryml::MAP;
	resproot["status"] = "success";
	std::string response;
	try {
	  ryml::NodeRef respdata = resproot["data"];

	  // parse the input
	  auto indata = c4::to_csubstr((const char*)msg->data());
	  ryml::Tree reqtree = ryml::parse_in_arena(indata);
	  ryml::ConstNodeRef reqroot = reqtree.rootref();

	  // check for the command member
	  if ( !reqroot.has_child("command") )
	    throw Exception("Missing \"command\" in request");

	  reqtree["command"] >> command;

	  const auto& cmdit = c_handlers.find(command);
	  if ( cmdit == c_handlers.end() ) {
	    error("No handler for requested command \"%s\"", command.c_str());
	    throw Exception("Command \"%s\" not found", command.c_str());
	  }

	  ryml::ConstNodeRef reqdata = reqtree["data"];
	  cmdit->second(reqdata, respdata);

	  // generate the response message
	  response = ryml::emitrs_json<std::string>(resptree);
	  if ( response.length() >=
	       std::numeric_limits<Message::size_type>::max() ) {
	    response.clear();
	    throw Exception("Response too large: %i", response.size());
	  }
	}
	catch (Exception& e) {
	  resproot["status"] = "error";
	  auto cmsg = resptree.to_arena(e.what());
	  resproot["message"] = cmsg;
	}
	catch (std::exception& e) {
	  resproot["status"] = "error";
	  auto cmsg = resptree.to_arena(e.what());
	  resproot["message"] = cmsg;
	}
	catch (...) {
	  resproot["status"] = "error";
	  resproot["message"] = "Unknown exception";
	}

	// if response generation failed, then we're in
	// an error block and need to switch to
	// generating an error message
	if ( response.size() == 0 ) {
	  if ( resproot.has_child("data") )
	    resproot.remove_child("data");
	  response = ryml::emitrs_json<std::string>(resptree);
	}
	zmqsock->send(response.data(), response.size(), true);
      }
    }

    uint32_t PRThread::maxWorkers() const {
      return 4;
    }

    void PRThread::setFermenterType(ryml::NodeRef& _node,
				    DB::fermenter_types::cptr _ft) {
      auto tree = _node.tree();
      _node |= ryml::MAP;
      _node["id"] << _ft->id;
      _node["capacity"] << _ft->capacity;

      auto name = tree->to_arena(_ft->name);
      _node["name"] = name;
      auto imageurl = tree->to_arena(_ft->imageurl);
      _node["imageurl"] = imageurl;
    }
    PRCMD(getFermenterTypes) {
      auto fts = ServiceManager::get<DB::Connection>()->getFermenterTypes();

      _rep |= ryml::SEQ;

      for (auto& it: fts) {
	ryml::NodeRef node = _rep.append_child();
	setFermenterType(node, it);
      }
    }

    PRCMD(getFermenters) {
      auto fs = ServiceManager::get<DB::Connection>()->getFermenters();

      auto tree = _rep.tree();
      _rep |= ryml::SEQ;

      for (auto& it: fs) {
	ryml::NodeRef node = _rep.append_child();
	node |= ryml::MAP;
	node["id"] << it->id;
	auto name = tree->to_arena(it->name);
	node["name"] = name;

	ryml::NodeRef ftype = node["type"];
	setFermenterType(ftype, it->fermenter_type);
      }
    }

    PRCMD(getTilthydrometers) {
    }
  }
}
