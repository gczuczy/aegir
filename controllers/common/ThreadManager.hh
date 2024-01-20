/*
  Thread manager service base class
 */

#ifndef AEGIR_THREADMANAGER_HH
#define AEGIR_THREADMANAGER_HH

#include <string>
#include <memory>
#include <atomic>
#include <cstdint>
#include <thread>
#include <functional>
#include <list>
#include <map>

#include "common/misc.hh"
#include "common/Exception.hh"
#include "common/LogChannel.hh"

namespace aegir {

  class ThreadManager {
  public:
    // guard for run calls
    class RunGuard {
    public:
      RunGuard(std::atomic<bool>& _flag);
      ~RunGuard();
    private:
      std::atomic<bool>& c_flag;
    };

    // subclasses for tenants
    // Thread
    class Thread {

    public:
      Thread();
      Thread(const Thread&)=delete;
      virtual ~Thread()=0;
      virtual void init()=0;
      virtual void worker()=0;
      void stop();
      inline bool shouldRun() const { return c_run; };

    protected:
      std::atomic<bool> c_run;
    };
    // ThreadPool
    class ThreadPool {

    public:
      ThreadPool();
      ThreadPool(const ThreadPool&)=delete;
      virtual ~ThreadPool()=0;
      virtual void init()=0;
      virtual void controller()=0;
      virtual void worker(std::atomic<bool>& _run)=0;
      virtual std::uint32_t maxWorkers() const =0;
      void stop();
      inline bool shouldRun() const { return c_run; };

    protected:
      std::atomic<bool> c_run;
    };
    typedef std::shared_ptr<Thread> thread_type;
    typedef std::shared_ptr<ThreadPool> threadpool_type;

  private:
    struct single_thread {
      std::string name;
      std::thread thread;
      std::atomic<bool> running;
      std::shared_ptr<Thread> impl;
    };
    struct worker_thread {
      std::thread thread;
      std::uint32_t id;
      std::string name;
      std::shared_ptr<ThreadPool> impl;
      std::atomic<bool> running;
      std::atomic<bool> run;
    };
    struct thread_pool {
      std::string name;
      std::shared_ptr<ThreadPool> impl;
      std::thread thread;
      std::atomic<bool> running; // controller
      std::list<worker_thread> workers;
    };

  public:
    ThreadManager(const ThreadManager&) = delete;
    virtual ~ThreadManager()=0;

    void run();
    void stop();

  protected:
    ThreadManager();

    // these are for singletons
    template<Derived<Thread> T>
    void registerHandler(const std::string& _name) {
      // first look for dup names
      if ( c_threads.find(_name) != c_threads.end() )
	throw Exception("Thread %s already registred", _name.c_str());

      // do not start the thread up now
      c_threads[_name].name = _name;
      c_threads[_name].running = false;;
      c_threads[_name].impl = std::static_pointer_cast<Thread>(T::getInstance());
    };
    template<Derived<ThreadPool> T>
    void registerHandler(const std::string& _name) {
      if ( c_pools.find(_name) != c_pools.end() )
	throw Exception("Pool %s already registred", _name.c_str());

      c_pools[_name].name = _name;
      c_pools[_name].running = false;;
      c_pools[_name].impl = std::static_pointer_cast<ThreadPool>(T::getInstance());
    };

  private:
    template<typename T>
    void runWrapper(T& _subject) {
      throw Exception("Generic runWrapper not implemented");
    };
    template<>
    void runWrapper(single_thread& _subject);
    template<>
    void runWrapper(worker_thread& _subject);
    template<>
    void runWrapper(thread_pool& _subject);
    void spawnWorker(thread_pool& _pool);

  private:
    std::atomic<bool> c_run;
    LogChannel c_logger;
    std::map<std::string, single_thread> c_threads;
    std::map<std::string, thread_pool> c_pools;
  };
}

#endif
