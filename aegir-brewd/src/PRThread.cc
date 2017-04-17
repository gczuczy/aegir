
#include "PRThread.hh"

#include <stdio.h>

#include "Config.hh"
#include "JSONMessage.hh"

namespace aegir {
  PRThread::PRThread(): c_mq_pr(ZMQ::SocketType::REP) {
    auto cfg = Config::getInstance();

    // bind the PR socket
    {
      char buff[64];
      snprintf(buff, 63, "tcp://127.0.0.1:%u", cfg->getPRPort());
      c_mq_pr.bind(buff);
    }

    // Load the JSON Message handlers
    c_handlers["loadProgram"] = std::bind(&PRThread::handleLoadProgram, this, std::placeholders::_1);

    auto thrmgr = ThreadManager::getInstance();
    thrmgr->addThread("PR", *this);
  }

  PRThread::~PRThread() {
  }

  void PRThread::run() {
    printf("PRThread started\n");

    std::chrono::microseconds ival(20000);
    std::shared_ptr<Message> msg;
    while (c_run) {
      while ( (msg = c_mq_pr.recv(ZMQ::MessageFormat::JSON)) ) {
	// if it's not a JSON type, then send an error response
	try {
	  if ( msg->type() != MessageType::JSON ) {
	    printf("PRThread: Got a non-JSON message\n");
	    Json::Value root;
	    root["status"] = "error";
	    root["message"] = "Not a JSON Message";
	    c_mq_pr.send(JSONMessage(root));
	    continue;
	  }
	  // Handling the JSON message
	  auto jsonmsg = std::static_pointer_cast<JSONMessage>(msg);
	  auto reply = handleJSONMessage(jsonmsg->getJSON());
	  c_mq_pr.send(JSONMessage(*reply));
	}
	catch (Exception &e) {
	  printf("PRThread: Exception while handling zmq message: %s\n", e.what());
	  Json::Value root;
	  root["status"] = "error";
	  root["message"] = e.what();
	  c_mq_pr.send(JSONMessage(root));
	}
	catch (std::exception &e) {
	  printf("PRThread: Exception while handling zmq message: %s\n", e.what());
	  Json::Value root;
	  root["status"] = "error";
	  root["message"] = e.what();
	  c_mq_pr.send(JSONMessage(root));
	}
	catch (...) {
	  printf("PRThread: unknown exception while recv zmq message\n");
	  Json::Value root;
	  root["status"] = "error";
	  root["message"] = "Unknown exception";
	  c_mq_pr.send(JSONMessage(root));
	}
      } // c_mq_recv
      std::this_thread::sleep_for(ival);
    }

    printf("PRThread stopped\n");
  }

  std::shared_ptr<Json::Value> PRThread::handleJSONMessage(const Json::Value &_msg) {

    if ( _msg.type() != Json::ValueType::objectValue ) {
      std::string typestr("unknown");
      switch(_msg.type()) {
      case Json::ValueType::nullValue:
	typestr = "null";
	break;
      case Json::ValueType::intValue:
	typestr = "int";
	break;
      case Json::ValueType::uintValue:
	typestr = "uint";
	break;
      case Json::ValueType::realValue:
	typestr = "real";
	break;
      case Json::ValueType::stringValue:
	typestr = "string";
	break;
      case Json::ValueType::booleanValue:
	typestr = "boolean";
	break;
      case Json::ValueType::arrayValue:
	typestr = "array";
	break;
      case Json::ValueType::objectValue:
	typestr = "object";
	break;
      }
      Json::Value root;
      root["status"] = "error";
      root["message"] = std::string("Input's type must be an object, and not a ") + typestr;
      return std::make_shared<Json::Value>(root);
    }

    // check whether we have the "command" defined
    if ( !_msg.isMember("command") ) {
      printf("PRThread::handleJSONMessage(): command is not defined\n");
      Json::Value root;
      root["status"] = "error";
      root["message"] = "command not supplied";
      return std::make_shared<Json::Value>(root);
    }

    // check whether command is a string
    auto cmd = _msg["command"];
    if ( !cmd.isString() ) {
      Json::Value root;
      root["status"] = "error";
      root["message"] = "command not a string";
      return std::make_shared<Json::Value>(root);
    }
    // fetch the command
    std::string cmdstr = cmd.asString();

    // now check whether we have data
    if ( !_msg.isMember("data") ) {
      Json::Value root;
      root["status"] = "error";
      root["message"] = "data is not supplied";
      return std::make_shared<Json::Value>(root);
    }

    // Check whether the handler for the command is defined
    auto cmdit = c_handlers.find(cmdstr);
    if ( cmdit == c_handlers.end() ) {
      Json::Value root;
      root["status"] = "error";
      root["message"] = "Unknown command";
      return std::make_shared<Json::Value>(root);
    }

    return cmdit->second(_msg["data"]);
  }

  std::shared_ptr<Json::Value> PRThread::handleLoadProgram(const Json::Value &_data) {
    Json::Value root;
    root["status"] = "success";
    root["message"] = "Command Loaded";
    return std::make_shared<Json::Value>(root);
  }
}
