#include "IOHandler.hh"

#include <stdio.h>

#include <chrono>
#include <map>
#include <string>

#include "ZMQ.hh"
#include "GPIO.hh"
#include "Config.hh"

namespace aegir {

  IOHandler *IOHandler::c_instance = 0;

  IOHandler::IOHandler(): c_mq_pub(ZMQ::SocketType::PUB) {
    auto thrmgr = ThreadManager::getInstance();

    c_mq_pub.bind("inproc://iopub");

    thrmgr->addThread("IOHandler", *this);
  }

  IOHandler::~IOHandler() {
  }

  IOHandler *IOHandler::getInstance() {
    if ( !c_instance) c_instance = new IOHandler();
    return c_instance;
  }

  void IOHandler::run() {
    printf("IOHandler started\n");

    std::chrono::microseconds ival(10000);
    std::map<std::string,int> inpins;
    std::set<std::string> outpins;
    GPIO &gpio(*GPIO::getInstance());

    // later we might need to handle unused pins for multiple configs here
    for ( auto &it: g_pinconfig ) {
      if ( it.second.mode == PinMode::IN ) {
	printf("IOHandle: Loading PIN %s\n", it.first.c_str());
	inpins[it.first] = -1;
      } else if ( it.second.mode == PinMode::OUT ) {
	outpins.insert(it.first);
      }
    }

    int newval;
    while ( c_run ) {
      // read the input pins
      for (auto &it: inpins) {
	newval = gpio[it.first].get();
	if ( newval != it.second ) {
	  printf("IOHandle: Pin %s %i -> %i\n ", it.first.c_str(),
		 it.second, newval);
	  it.second = newval;
	  auto msg = PinStateMessage(it.first, newval);
	  c_mq_pub.send(msg);
	}
      }
      std::this_thread::sleep_for(ival);
    }

    printf("IOHandler stopped\n");
  }
}
