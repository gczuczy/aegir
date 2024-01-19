
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

  void ThreadManager::Thread::runWrapper(std::atomic<bool>& _flag) {
    ThreadManager::RunGuard g(_flag);
    run();
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
  ThreadManager::ThreadManager(): c_run(true) {
  }

  ThreadManager::~ThreadManager() {
  }

  void ThreadManager::run() {
    std::chrono::milliseconds s(100);
    // first initialize everyone
    for ( auto& it: c_threads ) it.second.impl->init();
    for ( auto& it: c_pools ) it.second.impl->init();

    // starting up threads

    while ( c_run ) {
      std::this_thread::sleep_for(s);
    }

    // waiting for threads to stop
    bool has_running = true;
    s = std::chrono::milliseconds(5);
    while (has_running) {
      has_running = false;

      for ( auto& it: c_threads ) {
	if ( !it.second.running ) {
	  if ( it.second.thread.joinable() ) it.second.thread.join();
	  has_running = true;
	  c_threads.erase(it.first);
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
    for ( auto& it: c_pools ) it.second.impl->stop();
  }
}
