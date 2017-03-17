
#include "ThreadManager.hh"

#include <functional>
#include <chrono>

namespace aegir {

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
    for (auto &it: c_threads) {
      std::function<void()> f = std::bind(&ThreadManager::wrapper, this, &it.second.base);
      it.second.thr = std::thread(f);
    }

    std::chrono::microseconds ival(100000);
    while (true) {
      std::this_thread::sleep_for(ival);
    }

    return *this;
  }

  void ThreadManager::wrapper(ThreadBase *_b) {
    _b->run();
  }
}
