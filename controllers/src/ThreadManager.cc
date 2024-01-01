
#include "ThreadManager.hh"

#include <stdio.h>
#include <signal.h>
#include <sys/event.h>
#include <string.h>
#include <pthread.h>

#include <functional>
#include <chrono>

#include "GPIO.hh"
#include "ZMQ.hh"

namespace aegir {

  static void sighandler(int _sig) {
    if ( _sig == SIGSEGV || _sig == SIGABRT) {
      GPIO &gpio = *GPIO::getInstance();
      try {
	gpio["mtheat"].low();
	gpio["mtpump"].low();
	gpio["bkpump"].low();
      }
      catch (...) {
      }
    }
  }

  static int g_sigs_blocked[] = {
    SIGINT,
    SIGHUP,
    SIGKILL,
    SIGPIPE,
    SIGALRM,
    SIGTERM,
    SIGUSR1,
    SIGUSR2
  };

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

  ThreadManager::thread::thread(const std::string &_name, ThreadBase &_base):
    name(_name), base(_base) {
  }

  ThreadManager *ThreadManager::c_instance = 0;

  ThreadManager::ThreadManager(): c_started(false), c_log("ThreadManager") {
  }

  ThreadManager::~ThreadManager() {
  }

  ThreadManager *ThreadManager::getInstance() {
    if ( !c_instance ) c_instance = new ThreadManager();
    return c_instance;
  }

  ThreadManager &ThreadManager::addThread(const std::string &_name, ThreadBase &_thread) {
    c_threads.emplace(std::make_pair(_name, thread(_name, _thread)));

    // if threads are already started, then late-start it
    if ( c_started ) {
      auto it = c_threads.find(_name);
      c_log.info("Late-starting thread named %s", it->first.c_str());
      std::function<void()> f = std::bind(&ThreadManager::wrapper, this, &it->second.base);
      it->second.thr = std::thread(f);
    }
    return *this;
  }

  ThreadManager &ThreadManager::start() {
    int signals[] = {
      SIGINT,
      SIGHUP,
      SIGPIPE,
      SIGALRM,
      SIGTERM,
      SIGUSR1,
      SIGUSR2
    };
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    for (int i=0; i<sizeof(signals)/sizeof(int); ++i) {
      if ( sigaddset(&sa.sa_mask, signals[i])!=0 ) {
	c_log.error("Unable to add signal %i to set", signals[i]);
	fprintf(stderr, "Unable to add signal %i to set\n", signals[i]);
      }
    }

    //sa.sa_handler = SIG_IGN;
    sa.sa_handler = &sighandler;
    sa.sa_sigaction = 0;
    sa.sa_flags = SA_RESTART;
    for (int i=0; i<sizeof(signals)/sizeof(int); ++i) {
      if ( sigaction(signals[i], &sa, 0) != 0 ) {
 	c_log.error("sigaction(%i) failed: %s", signals[i], strerror(errno));
 	fprintf(stderr, "sigaction(%i) failed: %s\n", signals[i], strerror(errno));
      }
    }

    // start the threads
    for (auto &it: c_threads) {
      c_log.info("Starting thread named %s", it.first.c_str());
      std::function<void()> f = std::bind(&ThreadManager::wrapper, this, &it.second.base);
      it.second.thr = std::thread(f);
    }
    c_started = true;

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
    c_log.trace("Starting loop");
    while (run) {
      n = kevent(kq, 0, 0, evlist, 16, 0);
      if ( n < 0 ) continue;

      for ( int i=0; i < n; ++i ) {
	if ( evlist[i].filter == EVFILT_SIGNAL ) {
	  c_log.info("Received signal %i", evlist[i].ident);
	  if ( evlist[i].ident == SIGINT || evlist[i].ident == SIGKILL ) {
	    run = 0;
	  }
	} // EVFILT_SIGNAL
      } // evlist check
    }

    // waiting for thread executions to finish
    for ( auto &it: c_threads ) {
      c_log.info("Stopping thread %s", it.second.name.c_str());
      it.second.base.stop();
    }

    // wait for the threads to finish
    for ( auto &it: c_threads ) {
      c_log.info("Waiting for thread %s", it.second.name.c_str());
      it.second.thr.join();
    }
    // close the ZMQ context
    ZMQ::getInstance().close();

    return *this;
  }

  void ThreadManager::wrapper(ThreadBase *_b) {
    sigset_t ss;
    sigemptyset(&ss);

    for (int i=0; i<sizeof(g_sigs_blocked)/sizeof(int); ++i)
      sigaddset(&ss, g_sigs_blocked[i]);

    if (int err = pthread_sigmask(SIG_SETMASK, &ss, 0); err != 0 ) {
      c_log.error("pthread_sigmask failed: %s", strerror(err));
    }
    _b->run();
  }
}
