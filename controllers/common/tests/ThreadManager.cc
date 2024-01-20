/*
  ThreadPool
 */

#include "common/ThreadManager.hh"

#include <chrono>

#include <catch2/catch_test_macros.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

class TestThread: public aegir::ThreadManager::Thread {

private:
  TestThread(): c_initialized(false) {};

public:
  virtual ~TestThread() {
  };
  virtual void init() {
    c_initialized = true;
  }

  virtual void worker() {
    std::chrono::milliseconds s(50);

    while ( c_run ) {
      std::this_thread::sleep_for(s);
    }
  }
  static std::shared_ptr<TestThread> getInstance() {
    static std::shared_ptr<TestThread> instance{new TestThread()};
    return instance;
  };
  inline bool isInitialized() const { return c_initialized; };

private:
  std::atomic<bool> c_initialized;
};

class TestPool: public aegir::ThreadManager::ThreadPool {

private:
  TestPool(): c_initialized(false) {};

public:
  virtual ~TestPool() {
  };
  virtual void init() {
    c_initialized = true;
  }

  virtual void controller() {
    std::chrono::milliseconds s(50);

    while ( c_run ) {
      std::this_thread::sleep_for(s);
    }
  }

  virtual void worker(std::atomic<bool>& _run) {
    std::chrono::milliseconds s(50);

    while ( c_run && _run ) {
      std::this_thread::sleep_for(s);
    }
  }

  virtual std::uint32_t maxWorkers() const {
    return 3;
  }

  static std::shared_ptr<TestPool> getInstance() {
    static std::shared_ptr<TestPool> instance{new TestPool()};
    return instance;
  };
  inline bool isInitialized() const { return c_initialized; };

private:
  std::atomic<bool> c_initialized;
};

class TestManager: public aegir::ThreadManager {

private:
  TestManager(): ThreadManager() {
    registerHandler<TestThread>("test");
    registerHandler<TestPool>("test");
  };
public:
  virtual ~TestManager() {
  };
  static std::shared_ptr<TestManager> getInstance() {
    static std::shared_ptr<TestManager> instance{new TestManager()};
    return instance;
  };
};

std::atomic<bool> g_run(true);

void runManager() {
  auto tm = TestManager::getInstance();

  tm->run();
}

TEST_CASE("ThreadPool", "[common]") {
  auto tm = TestManager::getInstance();

  g_run = true;

  std::thread tmt(runManager);

  auto testthread = TestThread::getInstance();
  std::chrono::milliseconds s(100);
  std::this_thread::sleep_for(s);

  REQUIRE(testthread->isInitialized());
  g_run = false;
  tm->stop();
  tmt.join();
}
