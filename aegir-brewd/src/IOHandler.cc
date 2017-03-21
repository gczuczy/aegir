#include "IOHandler.hh"

#include <stdio.h>

#include <chrono>

namespace aegir {

  IOHandler *IOHandler::c_instance = 0;

  IOHandler::IOHandler() {
    auto thrmgr = ThreadManager::getInstance();
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

    std::chrono::microseconds ival(100000);
    while ( c_run ) {
      std::this_thread::sleep_for(ival);
    }

    printf("IOHandler stopped\n");
  }
}
