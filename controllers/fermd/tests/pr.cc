/*
  PR thread/pool testing
 */

#include "common/ThreadManager.hh"
#include "fermd/PRThread.hh"
#include "fermd/ZMQConfig.hh"

#include <stdio.h>

#include <chrono>

#include "common/ServiceManager.hh"

#include <catch2/catch_test_macros.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

class PRManager: public aegir::ThreadManager {

  friend aegir::ServiceManager;
protected:
  PRManager(): aegir::ThreadManager() {
    registerHandler<aegir::fermd::PRThread>("PR");
  };

public:
  virtual ~PRManager() {};
};

class PRTestSM: public aegir::ServiceManager {
public:
  PRTestSM() {
    add<aegir::fermd::ZMQConfig>();
    add<aegir::fermd::PRThread>();
    add<PRManager>();
  };
  virtual ~PRTestSM() {};
};

static void runManager() {
  auto tm = aegir::ServiceManager::get<PRManager>();

  tm->run();
}

TEST_CASE("PR", "[fermd][pr]") {
  PRTestSM sm;
  auto tm = sm.get<PRManager>();

  std::thread tmt(runManager);

  auto csock = sm.get<aegir::fermd::ZMQConfig>()
    ->srcSocket("prpublic");

  // wait a bit for everythint to start up
  {
    std::chrono::milliseconds s(10);
    while ( !tm->isRunning() )
      std::this_thread::sleep_for(s);
  }
  printf("foo!\n");

  tm->stop();
  tmt.join();
}
