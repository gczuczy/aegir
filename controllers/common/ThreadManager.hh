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
#include <map>
#include <chrono>
#include <concepts>

#include <boost/circular_buffer.hpp>

#include "common/misc.hh"
#include "common/Exception.hh"
#include "common/LogChannel.hh"
#include "common/ServiceManager.hh"

namespace aegir {

  class Thread;
  class ThreadPool;

  // Thread
  class Thread {
  public:
    Thread();
    Thread(const Thread&)=delete;
    virtual ~Thread()=0;
    virtual void init()=0;
    virtual void worker()=0;
    virtual void stop();
    inline bool shouldRun() const { return c_run; };

  protected:
    std::atomic<bool> c_run;
  };
  typedef std::shared_ptr<Thread> thread_type;
  template<typename T>
  concept IsThread = std::is_base_of<Thread, T>::value &&
    std::is_base_of<Service, T>::value;

  // ThreadPool
  class ThreadPool {
  protected:
    class ActivityGuard {
    public:
      ActivityGuard()=delete;
      ActivityGuard(const ActivityGuard&) = delete;
      ActivityGuard(ActivityGuard&&) = delete;
      ActivityGuard(ThreadPool* _pool);
      ~ActivityGuard();

    private:
      ThreadPool* c_pool;
      std::chrono::time_point<std::chrono::system_clock> c_start;
    };
    friend class ActivityGuard;

  public:
    ThreadPool();
    ThreadPool(const ThreadPool&)=delete;
    virtual ~ThreadPool()=0;
    virtual void init()=0;
    virtual void controller()=0;
    virtual void worker(std::atomic<bool>& _run)=0;
    virtual std::uint32_t maxWorkers() const =0;
    virtual void stop();
    inline bool shouldRun() const { return c_run; };
    inline std::uint32_t getActiveCount() const {return c_activity;};

  protected:
    std::atomic<bool> c_run;
    std::atomic<std::uint32_t> c_activity;
  };
  template<typename T>
  concept IsThreadPool = std::is_base_of<ThreadPool, T>::value &&
    std::is_base_of<Service, T>::value;
  typedef std::shared_ptr<ThreadPool> threadpool_type;

  class ThreadManager: public Service,
		       private LogChannel {
  public:
    // guard for run calls
    class RunGuard {
    public:
      RunGuard(std::atomic<bool>& _flag);
      ~RunGuard();
    private:
      std::atomic<bool>& c_flag;
    };


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
    struct pool_metrics_type {
      uint32_t active;
      uint32_t total;
    };
    struct thread_pool {
      std::string name;
      std::shared_ptr<ThreadPool> impl;
      std::thread thread;
      std::atomic<bool> running; // controller
      std::map<std::uint32_t, worker_thread> workers;
      boost::circular_buffer<pool_metrics_type> metrics;
    };

  public:
    ThreadManager(const ThreadManager&) = delete;
    virtual ~ThreadManager()=0;

    void run();
    void stop();
    bool isRunning();
    virtual void bailout();

  protected:
    ThreadManager();

    virtual void init();
    // these are for singletons
    template<IsThread T>
    void registerHandler(const std::string& _name) {
      // first look for dup names
      if ( c_threads.find(_name) != c_threads.end() )
	throw Exception("Thread %s already registred", _name.c_str());

      // do not start the thread up now
      c_threads[_name].name = _name;
      c_threads[_name].running = false;;
      c_threads[_name].impl =
	std::static_pointer_cast<Thread>(ServiceManager::get<T>());
    };
    template<IsThreadPool T>
    void registerHandler(const std::string& _name) {
      if ( c_pools.find(_name) != c_pools.end() )
	throw Exception("Pool %s already registred", _name.c_str());

      c_pools[_name].name = _name;
      c_pools[_name].running = false;;
      c_pools[_name].metrics.set_capacity(c_metrics_samples);
      c_pools[_name].impl =
	std::static_pointer_cast<ThreadPool>(ServiceManager::get<T>());
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
    std::map<std::string, single_thread> c_threads;
    std::map<std::string, thread_pool> c_pools;
    int c_kq;
  protected:
    std::uint32_t c_metrics_samples;
    float c_scale_down, c_scale_up;
  };
}

#endif
