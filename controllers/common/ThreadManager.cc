
#include "common/ThreadManager.hh"

#include <stdio.h>

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
  ThreadManager::ThreadPool::ThreadPool(): c_run(true), c_activity(0) {
  }

  ThreadManager::ThreadPool::~ThreadPool() {
  }

  void ThreadManager::ThreadPool::stop() {
    c_run = false;
  }

  /*
    ThreadPool::ActivityGuard
   */
  ThreadManager::ThreadPool::ActivityGuard::ActivityGuard(ThreadPool* _pool)
    : c_pool(_pool) {
    ++c_pool->c_activity;
  }

  ThreadManager::ThreadPool::ActivityGuard::~ActivityGuard() {
    --c_pool->c_activity;
  }

  /*
    ThreadManager
   */
  ThreadManager::ThreadManager(): c_run(true), c_logger("ThreadManager"),
				  c_metrics_samples(10),
				  c_scale_down(0.3), c_scale_up(0.8) {
  }

  ThreadManager::~ThreadManager() {
  }

  void ThreadManager::run() {
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

    uint32_t counter;
    pool_metrics_type tempmetrics;
    std::chrono::milliseconds s(100);
    while ( c_run ) {
      std::this_thread::sleep_for(s);
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
		c_logger.info("Joined worker %s", wit.second.name.c_str());
		it.second.workers.erase(wit.first);
		break;
	      }
	    }
	  } while ( found );

	  // skip if noop
	  if ( activity > c_scale_down && activity < c_scale_up )
	    continue;

	  if ( it.second.workers.size()>1 && activity<c_scale_down ) {
	    c_logger.info("Scaling pool %s down", it.second.name.c_str());
	    const auto& last = it.second.workers.crbegin();
	    it.second.workers[last->first].run = false;
	  } else if ( it.second.workers.size()<it.second.impl->maxWorkers() &&
		      activity > c_scale_up ) {
	    c_logger.info("Scaling pool %s up", it.second.name.c_str());
	    spawnWorker(it.second);
	  }
	}
      }
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
	for ( auto& wit: it.second.workers ) {
	  if ( !wit.second.running ) {
	    if ( wit.second.thread.joinable() )
	      wit.second.thread.join();
	    c_logger.info("Joined worker %s", wit.second.name.c_str());
	    has_running = true;
	    it.second.workers.erase(wit.first);
	    break;
	  }
	}

	// if all the workers stopped, see to the controller
	if ( it.second.workers.size() ) {
	  has_running = true;
	} else {
	  // no more workers, stop the controller
	  if ( it.second.running ) continue;

	  if ( it.second.thread.joinable() )
	    it.second.thread.join();

	  c_logger.info("Joined pool controller %s",
			it.second.name.c_str());

	  has_running = true;
	  c_pools.erase(it.first);
	  break;
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
	wit.second.run = false;
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
    while (_subject.impl->shouldRun() && _subject.run ) {
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
      if ( _subject.impl->shouldRun() && _subject.run ) {
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
