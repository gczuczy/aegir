
#include "common/ThreadManager.hh"

#include <stdio.h>

#include <sys/event.h>

#include <iostream>
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
  Thread::Thread(): c_run(true) {
  }

  Thread::~Thread() {
  }

  void Thread::stop() {
    c_run = false;
  }

  /*
    ThreadPool
   */
  ThreadPool::ThreadPool(): c_run(true), c_activity(0) {
  }

  ThreadPool::~ThreadPool() {
  }

  void ThreadPool::stop() {
    c_run = false;
  }

  /*
    ThreadPool::ActivityGuard
   */
  ThreadPool::ActivityGuard::ActivityGuard(ThreadPool* _pool)
    : c_pool(_pool) {
    ++c_pool->c_activity;
  }

  ThreadPool::ActivityGuard::~ActivityGuard() {
    --c_pool->c_activity;
  }

  /*
    ThreadManager
   */
  ThreadManager::ThreadManager(): c_run(true), LogChannel("ThreadManager"),
				  c_metrics_samples(10),
				  c_scale_down(0.3), c_scale_up(0.8) {
    c_kq = kqueue();
  }

  ThreadManager::~ThreadManager() {
    close(c_kq);
  }

  void ThreadManager::run() {
    // call the child's init
    init();

    // first initialize everyone
    info("Initializing threads");
    for ( auto& it: c_threads ) it.second.impl->init();
    for ( auto& it: c_pools ) it.second.impl->init();

    // starting up standalone threads
    info("Starting single threads");
    for ( auto& it: c_threads ) {
      it.second.thread = std::thread(&ThreadManager::runWrapper<single_thread>,
				     this,
				     std::ref(it.second));
    }
    info("Starting threadpool controllers");
    for ( auto& it: c_pools ) {
      it.second.thread = std::thread(&ThreadManager::runWrapper<thread_pool>,
				     this,
				     std::ref(it.second));
    }
    info("Starting threadpool workers");
    for ( auto& it: c_pools ) {
      spawnWorker(it.second);
    }

    // set up a 100msec ticker with kqueue
    struct kevent ev;
    EV_SET(&ev, //event
	   42, //ident
	   EVFILT_TIMER, //filter
	   EV_ADD|EV_ENABLE, //flags
	   NOTE_MSECONDS, //fflags
	   100, //data
	   0); //udata
    kevent(c_kq, &ev, 1, 0, 0, 0);

    uint32_t counter;
    pool_metrics_type tempmetrics;
    while ( c_run ) {
      if ( kevent(c_kq, 0, 0, &ev, 1, 0)==0 ) continue;
      if ( ev.filter != EVFILT_TIMER && ev.ident != 42 ) continue;
      // in this main loop we only have to take care
      // of the pool worker scaling
      // the individual threads are restarted in the wrappers
      bool check = (counter = (++counter % c_metrics_samples)) == 0;

      for ( auto& it: c_pools ) {
	// collect metrics first
	tempmetrics.total = it.second.workers.size();
	tempmetrics.active = it.second.impl->getActiveCount();
	it.second.metrics.push_back(tempmetrics);

	// check the pool scaling if needed
	if ( check ) {
	  // count the activity
	  float activity(0);
	  for ( auto& mit: it.second.metrics )
	    activity += ((1.0f*mit.active)/mit.total)/c_metrics_samples;

	  // clean workers which are not running
	  bool found=false;
	  do {
	    found = false;
	    for ( auto& wit: it.second.workers ) {
	      if ( !wit.second.running && wit.second.thread.joinable() ) {
		found = true;
		wit.second.thread.join();
		info("Joined worker %s", wit.second.name.c_str());
		it.second.workers.erase(wit.first);
		break;
	      }
	    }
	  } while ( found );

	  // skip if noop
	  if ( activity > c_scale_down && activity < c_scale_up )
	    continue;

	  if ( it.second.workers.size()>1 && activity<c_scale_down ) {
	    info("Scaling pool %s down", it.second.name.c_str());
	    const auto& last = it.second.workers.crbegin();
	    it.second.workers[last->first].run = false;
	  } else if ( it.second.workers.size()<it.second.impl->maxWorkers() &&
		      activity > c_scale_up ) {
	    info("Scaling pool %s up", it.second.name.c_str());
	    spawnWorker(it.second);
	  }
	}
      }
    }

    // call the stop internal to signal every thread
    // to stop and bail out
    stop();

    auto s = std::chrono::milliseconds(5);
    debug("Waiting for threads to stop");
    // waiting for threads to stop
    while ( c_threads.size()>0 ) {
      for ( auto& it: c_threads ) {
	if ( !it.second.running ) {
	  if ( it.second.thread.joinable() ) it.second.thread.join();
	  info("Joined thread %s", it.second.name.c_str());
	  c_threads.erase(it.first);
	  break;
	}
      }
      std::this_thread::sleep_for(s);
    }

    debug("Waiting for pools to stop");
    // waiting for threads to stop
    while ( c_pools.size()>0 ) {
      for ( auto& it: c_pools ) {
	// first stop the workers
	for ( auto& wit: it.second.workers ) {
	  if ( !wit.second.running ) {
	    if ( wit.second.thread.joinable() )
	      wit.second.thread.join();
	    info("Joined worker %s", wit.second.name.c_str());
	    it.second.workers.erase(wit.first);
	    break;
	  }
	}

	// if all the workers stopped, see to the controller
	if ( it.second.workers.size() == 0  ) {

	  if ( it.second.running ) continue;

	  if ( it.second.thread.joinable() )
	    it.second.thread.join();

	  info("Joined pool controller %s",
	       it.second.name.c_str());

	  c_pools.erase(it.first);
	  break;
	}
      }
      std::this_thread::sleep_for(s);
    }

    info("All threads are stopped");
  }

  void ThreadManager::stop() {
    c_run = false;

    info("[ThreadManager] Stopping threads");

    // stop the threads
    for ( auto& it: c_threads ) it.second.impl->stop();
    // and the pools
    for ( auto& it: c_pools ) {
      try {
	it.second.impl->stop();
      }
      catch (aegir::Exception& e) {
	error("Pool \"%s\" controller stop(): %s",
	      it.second.name.c_str(), e.what());
      }
      catch (std::exception& e) {
	error("Pool \"%s\" controller stop(): %s",
	      it.second.name.c_str(), e.what());
      }
      catch (...) {
	error("Pool \"%s\" controller stop(): unknown exception",
	      it.second.name.c_str());
      }
      for ( auto& wit: it.second.workers )
	wit.second.run = false;
    }
  }

  bool ThreadManager::isRunning() {
    for (auto& it: c_threads)
      if ( !it.second.running ) return false;

    for (auto& it: c_pools) {
      if ( !it.second.running ) return false;

      // check the workers
      for (auto& wit: it.second.workers)
	if ( !wit.second.running ) return false;
    }
    return true;
  }

  void ThreadManager::bailout() {
    debug("ThreadManager::bailout");
    stop();
  }

  void ThreadManager::init() {
  }

  template<>
  void ThreadManager::runWrapper(single_thread& _subject) {
    RunGuard g(_subject.running);

    if ( !_subject.impl->shouldRun() )
      error("singlethread %s shouldRun off at start",
	    _subject.name.c_str());

    while (_subject.impl->shouldRun() ) {
      try {
	info("Starting thread %s", _subject.name.c_str());
	_subject.impl->worker();
      }
      catch (Exception& e) {
	warn("Restarting failed %s thread: %s",
	     e.what(), _subject.name.c_str());
	continue;
      }
      catch (std::exception& e) {
	warn("Restarting failed %s thread: %s",
	     e.what(), _subject.name.c_str());
	continue;
      }
      catch (...) {
	warn("Restarting failed %s thread",
	     _subject.name.c_str());
	continue;
      }
      if ( _subject.impl->shouldRun() ) {
	warn("Thread %s exited but still should run, restarting",
	     _subject.name.c_str());
      }
    }
  }
  template<>
  void ThreadManager::runWrapper(worker_thread& _subject) {
    RunGuard g(_subject.running);

    if ( !_subject.impl->shouldRun() )
      error("Worker thread %s shouldRun off at start",
	    _subject.name.c_str());

    if ( !_subject.run )
      error("Worker thread %s run off at start",
	    _subject.name.c_str());

    while (_subject.impl->shouldRun() && _subject.run ) {
      try {
	info("Starting worker %s", _subject.name.c_str());
	_subject.impl->worker(_subject.run);
      }
      catch (Exception& e) {
	warn("Restarting failed %s worker thread: %s",
	     e.what(), _subject.name.c_str());
	continue;
      }
      catch (std::exception& e) {
	warn("Restarting failed %s worker thread: %s",
	     e.what(), _subject.name.c_str());
	continue;
      }
      catch (...) {
	warn("Restarting failed %s worker thread",
	     _subject.name.c_str());
	continue;
      }
      if ( _subject.impl->shouldRun() && _subject.run ) {
	warn("worker thread %s exited but still should run, restarting",
	     _subject.name.c_str());
      }
    }
  }
  template<>
  void ThreadManager::runWrapper(thread_pool& _subject) {
    RunGuard g(_subject.running);

    if ( !_subject.impl->shouldRun() )
      error("Threadpool %s shouldRun off at start",
	    _subject.name.c_str());

    while (_subject.impl->shouldRun() ) {
      try {
	info("Starting pool controller %s", _subject.name.c_str());
	_subject.impl->controller();
      }
      catch (Exception& e) {
	warn("Restarting failed %s controller thread: %s",
	     e.what(), _subject.name.c_str());
	continue;
      }
      catch (std::exception& e) {
	warn("Restarting failed %s controller thread: %s",
	     e.what(), _subject.name.c_str());
	continue;
      }
      catch (...) {
	warn("Restarting failed %s controller thread",
	     _subject.name.c_str());
	continue;
      }
      if ( _subject.impl->shouldRun() ) {
	warn("Controller thread %s exited but still should run, restarting",
	     _subject.name.c_str());
      }
    }
  }

  void ThreadManager::spawnWorker(thread_pool& _pool) {
    std::uint32_t id(0);

    // look for an unused ID
    if ( _pool.workers.size()>0 ) {
      while ( true ) {
	if ( _pool.workers.find(id) == _pool.workers.end() )
	  break;
	++id;
      }
    }

    _pool.workers[id].id = id;
    { // assemble a name
      char name[64];
      uint32_t len;
      len = snprintf(name, sizeof(name)-1, "%s-%i",
		     _pool.name.c_str(), id);
      _pool.workers[id].name = std::string(name, len);
    }
    _pool.workers[id].impl = _pool.impl;
    _pool.workers[id].run = true;

    // structure is set up, we can launch it
    _pool.workers[id].thread = std::thread(&ThreadManager::runWrapper<worker_thread>,
					   this,
					   std::ref(_pool.workers[id]));
  }

}
