
#include "common/ThreadManager.hh"

#include <stdio.h>

#include <chrono>

namespace aegir {
  /*
    RunGuard
   */
  ThreadManager::RunGuard::RunGuard(std::atomic<bool>& _flag): c_flag(_flag) {
    c_flag = true;
  }
  ThreadManager::RunGuard::~RunGuard() {
    c_flag = false;
  }
  /*
    Thread
   */
  ThreadManager::Thread::Thread(): c_run(true) {
  }

  ThreadManager::Thread::~Thread() {
  }

  void ThreadManager::Thread::stop() {
    c_run = false;
  }

  /*
    ThreadPool
   */
  ThreadManager::ThreadPool::ThreadPool(): c_run(true) {
  }

  ThreadManager::ThreadPool::~ThreadPool() {
  }

  void ThreadManager::ThreadPool::stop() {
    c_run = false;
  }


  /*
    ThreadManager
   */
  ThreadManager::ThreadManager(): c_run(true), c_logger("ThreadManager") {
  }

  ThreadManager::~ThreadManager() {
  }

  void ThreadManager::run() {
    std::chrono::milliseconds s(100);

    // first initialize everyone
    c_logger.info("Initializing threads");
    for ( auto& it: c_threads ) it.second.impl->init();
    for ( auto& it: c_pools ) it.second.impl->init();

    // starting up standalone threads
    c_logger.info("Starting single threads");
    for ( auto& it: c_threads ) {
      it.second.thread = std::thread(&ThreadManager::runWrapper<single_thread>,
				     this,
				     std::ref(it.second));
    }
    c_logger.info("Starting threadpool controllers");
    for ( auto& it: c_pools ) {
      it.second.thread = std::thread(&ThreadManager::runWrapper<thread_pool>,
				     this,
				     std::ref(it.second));
    }
    c_logger.info("Starting threadpool workers");
    for ( auto& it: c_pools ) {
      spawnWorker(it.second);
    }

    while ( c_run ) {
      std::this_thread::sleep_for(s);
    }

    // call the stop internal to signal every thread
    // to stop and bail out
    stop();

    // waiting for threads to stop
    bool has_running = true;
    s = std::chrono::milliseconds(5);
    while (has_running) {
      has_running = false;

      // single threads
      for ( auto& it: c_threads ) {
	if ( !it.second.running ) {
	  if ( it.second.thread.joinable() ) it.second.thread.join();
	  has_running = true;
	  c_logger.info("Joined thread %s", it.second.name.c_str());
	  c_threads.erase(it.first);
	  break;
	} else {
	  has_running = true;
	}
      }
      // pools
      for ( auto& it: c_pools ) {
	// first stop the workers
	uint32_t workers(0);
	for ( auto& wit: it.second.workers ) {
	  if ( !wit.running && wit.thread.joinable() ) {
	    wit.thread.join();
	    c_logger.info("Joined worker %s", wit.name.c_str());
	  } else if ( wit.running ) {
	    ++workers;
	    has_running = true;
	  }
	}

	// if we have no more workers than kill the controller
	if ( workers == 0 ) {
	  if ( !it.second.running ) {
	    if ( it.second.thread.joinable() ) it.second.thread.join();
	    c_logger.info("Joined pool controller %s", it.second.name.c_str());
	    c_pools.erase(it.first);
	    break;
	  } else {
	    has_running = true;
	  }
	}
      }
      std::this_thread::sleep_for(s);
    }
  }

  void ThreadManager::stop() {
    c_run = false;

    // stop the threads
    for ( auto& it: c_threads ) it.second.impl->stop();
    // and the pools
    for ( auto& it: c_pools ) {
      it.second.impl->stop();
      for ( auto& wit: it.second.workers )
	wit.run = false;
    }
  }

  template<>
  void ThreadManager::runWrapper(single_thread& _subject) {
    RunGuard g(_subject.running);
    while (_subject.impl->shouldRun() ) {
      try {
	c_logger.info("Starting thread %s", _subject.name.c_str());
	_subject.impl->worker();
      }
      catch (Exception& e) {
	c_logger.warn("Restarting failed %s thread: %s",
		      e.what(),
		      _subject.name.c_str());
	continue;
      }
      catch (std::exception& e) {
	c_logger.warn("Restarting failed %s thread: %s",
		      e.what(),
		      _subject.name.c_str());
	continue;
      }
      catch (...) {
	c_logger.warn("Restarting failed %s thread",
		      _subject.name.c_str());
	continue;
      }
      if ( _subject.impl->shouldRun() ) {
	c_logger.warn("Thread %s exited but still should run, restarting",
		      _subject.name.c_str());
      }
    }
  }
  template<>
  void ThreadManager::runWrapper(worker_thread& _subject) {
    RunGuard g(_subject.running);
    while (_subject.impl->shouldRun() ) {
      try {
	c_logger.info("Starting worker %s", _subject.name.c_str());
	_subject.impl->worker(_subject.run);
      }
      catch (Exception& e) {
	c_logger.warn("Restarting failed %s worker thread: %s",
		      e.what(),
		      _subject.name.c_str());
	continue;
      }
      catch (std::exception& e) {
	c_logger.warn("Restarting failed %s worker thread: %s",
		      e.what(),
		      _subject.name.c_str());
	continue;
      }
      catch (...) {
	c_logger.warn("Restarting failed %s worker thread",
		      _subject.name.c_str());
	continue;
      }
      if ( _subject.impl->shouldRun() ) {
	c_logger.warn("worker thread %s exited but still should run, restarting",
		      _subject.name.c_str());
      }
    }
  }
  template<>
  void ThreadManager::runWrapper(thread_pool& _subject) {
    RunGuard g(_subject.running);
    while (_subject.impl->shouldRun() ) {
      try {
	c_logger.info("Starting pool controller %s", _subject.name.c_str());
	_subject.impl->controller();
      }
      catch (Exception& e) {
	c_logger.warn("Restarting failed %s controller thread: %s",
		      e.what(),
		      _subject.name.c_str());
	continue;
      }
      catch (std::exception& e) {
	c_logger.warn("Restarting failed %s controller thread: %s",
		      e.what(),
		      _subject.name.c_str());
	continue;
      }
      catch (...) {
	c_logger.warn("Restarting failed %s controller thread",
		      _subject.name.c_str());
	continue;
      }
      if ( _subject.impl->shouldRun() ) {
	c_logger.warn("Controller thread %s exited but still should run, restarting",
		      _subject.name.c_str());
      }
    }
  }

  void ThreadManager::spawnWorker(thread_pool& _pool) {
    std::uint32_t id(0);

    // first get the last id
    for ( auto& it: _pool.workers )
      if ( it.id > id ) id = it.id;

    ++id;
    auto& ref = _pool.workers.emplace_back();
    ref.id = id;
    { // assemble a name
      char name[64];
      uint32_t len;
      len = snprintf(name, sizeof(name)-1, "%s-%i",
		     _pool.name.c_str(), ref.id);
      ref.name = std::string(name, len);
    }
    ref.impl = _pool.impl;
    ref.run = true;

    // structure is set up, we can launch it
    ref.thread = std::thread(&ThreadManager::runWrapper<worker_thread>,
			     this,
			     std::ref(ref));
  }

}
