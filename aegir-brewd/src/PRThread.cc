
#include "PRThread.hh"

#include <stdio.h>

#include <string>
#include <chrono>
#include <thread>

#include "Config.hh"
#include "PRWorkerThread.hh"

namespace aegir {

  PRThread::PRThread(): c_mq_pr(ZMQ::SocketType::ROUTER),
			c_mq_prw(ZMQ::SocketType::DEALER),
			c_mq_prctrl(ZMQ::SocketType::SUB),
			c_mq_prdbg(ZMQ::SocketType::PUB) {
    auto cfg = Config::getInstance();

    // bind the PR socket / ROUTER
    {
      char buff[64];
      snprintf(buff, 63, "tcp://127.0.0.1:%u", cfg->getPRPort());
      c_mq_pr.bind(buff);
    }

    // DEALER
    c_mq_prw.bind("inproc://prworkers");

    c_mq_prctrl.bind("inproc://prctrl").subscribe("");

    c_mq_prdbg.bind("tcp://127.0.0.1:42011");

    auto thrmgr = ThreadManager::getInstance();
    thrmgr->addThread("PR", *this);
  }

  PRThread::~PRThread() {
  }

  void PRThread::run() {
    printf("PRThread started\n");

    // start the workers first
    // FIXME: later this has to be dynamic, now it's a fixed 3 threads
    for (int i=0; i<3; ++i) {
      char name[32];
      snprintf(name, sizeof(name)-1, "prworker-%i", i);
      PRWorkerThread *thr = new PRWorkerThread(std::string(name));
      c_workers.insert(thr);
    }

    ZMQ::getInstance().proxy(c_mq_pr, c_mq_prw, c_mq_prdbg, c_mq_prctrl);

    for (auto &it: c_workers) {
      delete it;
    }

    printf("PRThread stopped\n");
  }

  void PRThread::stop() noexcept {
    printf("PRThread::stop() called\n");
    c_run = false;
#if 0
    ZMQ::Socket ctrl(ZMQ::SocketType::PUB);

    try {
      ctrl.connect("inproc://prctrl");
      ctrl.send("TERMINATE");
    }
    catch (Exception &e) {
      printf("PRThread::stop() ZMQ failed: %s\n", e.what());
    }
#endif
  }
}
