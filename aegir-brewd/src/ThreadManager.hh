/*
  Threading implementation based on C++11 std::thread
 */

#ifndef AEGIR_THREADMANAGER_H
#define AEGIR_THREADMANAGER_H

#include <thread>
#include <string>
#include <map>
#include <atomic>

namespace aegir {

  class ThreadManager;

  class ThreadBase {
    friend class ThreadManager;
  public:
    ThreadBase();
    virtual ~ThreadBase() = 0;

    std::atomic<bool> c_run;

  protected:
    void stop() noexcept;
    virtual void run() = 0;
  };

  // A singleton threadmanager
  class ThreadManager {
    ThreadManager(const ThreadManager &) = delete;
    ThreadManager(ThreadManager &&) = delete;
    ThreadManager &operator=(const ThreadManager &) = delete;
    ThreadManager &operator=(ThreadManager &&) = delete;

  private:
    struct thread {
      thread(const std::string &_name, ThreadBase &_base);
      std::string name;
      std::thread thr;
      ThreadBase &base;
    };

  private:
    std::map<std::string, thread> c_threads;

  private:
    static ThreadManager *c_instance;
    ThreadManager();
    ~ThreadManager();

  public:
    static ThreadManager *getInstance();
    ThreadManager &addThread(const std::string &_name, ThreadBase &_thread);
    ThreadManager &start();

  private:
    void wrapper(ThreadBase *_b);
  };
}

#endif
