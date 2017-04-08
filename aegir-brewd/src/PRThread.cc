
#include "PRThread.hh"

#include <stdio.h>

#include "Config.hh"

namespace aegir {
  PRThread::PRThread(): c_mq_pr(ZMQ::SocketType::REP) {
    auto cfg = Config::getInstance();

    // bind the PR socket
    {
      char buff[64];
      snprintf(buff, 63, "tcp://127.0.0.1:%u", cfg->getPRPort());
      c_mq_pr.bind(buff);
    }

    auto thrmgr = ThreadManager::getInstance();
    thrmgr->addThread("PR", *this);
  }

  PRThread::~PRThread() {
  }

  void PRThread::run() {
    printf("PRThread started\n");

    std::chrono::microseconds ival(20000);
    while (c_run) {
      std::this_thread::sleep_for(ival);
    }

    printf("PRThread stopped\n");
  }
}
