/*
  ThreadPool
 */

#include "common/ThreadManager.hh"

#include <time.h>
#include <stdio.h>

#include <chrono>

#include "common/ServiceManager.hh"
#include "common/Message.hh"

#include <catch2/catch_test_macros.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

class TestThread: public aegir::Thread,
		  public aegir::Service {
  friend aegir::ServiceManager;
protected:
  TestThread(): Thread(), Service(), c_initialized(false) {};

public:
  virtual ~TestThread() {
  };
  virtual void init() {
    c_initialized = true;
  }

  virtual void bailout() {
    stop();
  };

  virtual void worker() {
    std::chrono::milliseconds s(50);

    while ( c_run ) {
      std::this_thread::sleep_for(s);
    }
  }
  inline bool isInitialized() const { return c_initialized; };

private:
  std::atomic<bool> c_initialized;
};

class TestPool: public aegir::ThreadPool,
		public aegir::Service {

  friend aegir::ServiceManager;
protected:
  TestPool(): ThreadPool(), Service(),
	      c_initialized(false), c_spin(false),
	      c_workers(0) {};

public:
  virtual ~TestPool() {
  };
  virtual void init() {
    c_initialized = true;
  }

  virtual void bailout() {
    stop();
  };

  virtual void controller() {
    std::chrono::milliseconds s(50);

    while ( c_run ) {
      std::this_thread::sleep_for(s);
    }
  }

  virtual void worker(std::atomic<bool>& _run) {
    std::chrono::milliseconds s(50);
    ++c_workers;

    while ( c_run && _run ) {
      if ( c_spin ) {
	ActivityGuard g(this);
	for (int i=0; i<128; ++i) time(0);
      } else {
	std::this_thread::sleep_for(s);
      }
    }
    --c_workers;
  }

  virtual std::uint32_t maxWorkers() const {
    return 3;
  }

  inline bool isInitialized() const { return c_initialized; };
  inline std::uint32_t getCurrentWorkers() const { return c_workers; };
  void spin(bool _val) {
    c_spin = _val;
  };

private:
  std::atomic<bool> c_initialized;
  std::atomic<bool> c_spin;
  std::atomic<std::uint32_t> c_workers;
};

class TestManager: public aegir::ThreadManager {
  friend class aegir::ServiceManager;
protected:
  TestManager(): ThreadManager() {
    registerHandler<TestThread>("test");
    registerHandler<TestPool>("test");
  };
public:
  virtual ~TestManager() {
  };
};

class ThreadManagerTestSM: public aegir::ServiceManager {
public:
  ThreadManagerTestSM() {
    add<aegir::MessageFactory>();
    add<TestThread>();
    add<TestPool>();
    add<TestManager>();
  };
  virtual ~ThreadManagerTestSM() {};
};

static void runManager() {
  auto tm = aegir::ServiceManager::get<TestManager>();

  tm->run();
}

TEST_CASE("ThreadPool", "[common][threading]") {
  ThreadManagerTestSM sm;
  auto tm = sm.get<TestManager>();

  std::thread tmt(runManager);

  auto testthread = sm.get<TestThread>();
  std::chrono::milliseconds s(100);
  std::this_thread::sleep_for(s);

  REQUIRE(testthread->isInitialized());

  // scale up the pool
  auto testpool = sm.get<TestPool>();
  REQUIRE(testpool->getCurrentWorkers() == 1);

  // spin it up
  {
    testpool->spin(true);
    auto s2 = std::chrono::seconds(2);
    std::this_thread::sleep_for(s2);
  }
  REQUIRE(testpool->getCurrentWorkers() == testpool->maxWorkers());

  // spin down
  {
    testpool->spin(false);
    auto s2 = std::chrono::seconds(2);
    std::this_thread::sleep_for(s2);
  }
  REQUIRE(testpool->getCurrentWorkers() == 1);

  // shut it down
  tm->stop();
  tmt.join();
}
