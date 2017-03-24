
#include "ThreadManager.hh"

#include <stdio.h>
#include <signal.h>
#include <sys/event.h>
#include <string.h>

#include <functional>
#include <chrono>

namespace aegir {

  static void sighandler(int) {
  }

  /*
   * ThreadBase definition
   */
  ThreadBase::ThreadBase(): c_run(true) {
  }

  ThreadBase::~ThreadBase() = default;

  void ThreadBase::stop() noexcept {
    c_run = false;
  }

  /*
   * ThreadManager definition
   */

  ThreadManager::thread::thread(const std::string &_name, ThreadBase &_base): name(_name), base(_base) {
  }

  ThreadManager *ThreadManager::c_instance = 0;

  ThreadManager::ThreadManager() {
  }

  ThreadManager::~ThreadManager() {
  }

  ThreadManager *ThreadManager::getInstance() {
    if ( !c_instance ) c_instance = new ThreadManager();
    return c_instance;
  }

  ThreadManager &ThreadManager::addThread(const std::string &_name, ThreadBase &_thread) {
    c_threads.emplace(std::make_pair(_name, thread(_name, _thread)));
    return *this;
  }

  ThreadManager &ThreadManager::start() {
    // signal handling with a dummy handler, stuff will be
    // taken care of from kevent() later
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGKILL);
    sigaddset(&sa.sa_mask, SIGPIPE);
    sigaddset(&sa.sa_mask, SIGALRM);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaddset(&sa.sa_mask, SIGUSR2);
    sa.sa_handler = sighandler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGHUP, &sa, 0);
    sigaction(SIGKILL, &sa, 0);
    sigaction(SIGPIPE, &sa, 0);
    sigaction(SIGALRM, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);

    // start the threads
    for (auto &it: c_threads) {
      printf("Starting thread named %s\n", it.first.c_str());
      std::function<void()> f = std::bind(&ThreadManager::wrapper, this, &it.second.base);
      it.second.thr = std::thread(f);
    }

    // start our main loop, handling signals, etc
    bool run(true);
    int kq = kqueue();
    struct kevent evlist[16];
    memset(evlist, 0, sizeof(evlist));

    // set up the event handling
    // we still need to add EVFILT_USER for manually interrupting the kevent() call
    int n;
    EV_SET(&evlist[0], SIGINT, EVFILT_SIGNAL, EV_ADD|EV_CLEAR|EV_ENABLE, 0, 0, 0);
    EV_SET(&evlist[1], SIGKILL, EVFILT_SIGNAL, EV_ADD|EV_CLEAR|EV_ENABLE, 0, 0, 0);
    n = kevent(kq, evlist, 2, 0, 0, 0);
    printf("Starting loop\n");
    while (run) {
      n = kevent(kq, 0, 0, evlist, 16, 0);
      if ( n < 0 ) continue;

      for ( int i=0; i < n; ++i ) {
	if ( evlist[i].filter == EVFILT_SIGNAL ) {
	  if ( evlist[i].ident == SIGINT || evlist[i].ident == SIGKILL ) {
	    run = 0;
	  }
	} // EVFILT_SIGNAL
      } // evlist check
    }

    // waiting for thread executions to finish
    for ( auto &it: c_threads ) {
      it.second.base.stop();
      it.second.thr.join();
    }

    return *this;
  }

  void ThreadManager::wrapper(ThreadBase *_b) {
    _b->run();
  }
}
