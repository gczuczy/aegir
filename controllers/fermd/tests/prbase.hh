
#ifndef AEGIR_FERMD_TESTS_PRBASE
#define AEGIR_FERMD_TESTS_PRBASE

#include "common/ThreadManager.hh"
#include "fermd/PRThread.hh"
#include "fermd/ZMQConfig.hh"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <chrono>
#include <functional>
#include <set>

#include "common/tests/common.hh"
#include "common/ServiceManager.hh"
#include "DBConnection.hh"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

using Catch::Matchers::WithinRel;

void runPRManager();

class PRManager: public aegir::ThreadManager {

  friend aegir::ServiceManager;
protected:
  PRManager();

public:
  virtual ~PRManager();
};

class PRTestSM: public aegir::ServiceManager {
public:
  PRTestSM();
  virtual ~PRTestSM();
};

class PRFixture {
public:
  PRFixture();
  virtual ~PRFixture();

protected:
  aegir::RawMessage::ptr send(const std::string&);
  bool isError(aegir::RawMessage::ptr&);

private:
  FileGuard c_fg;
  PRTestSM c_sm;
  std::thread c_manager;
  aegir::zmqsocket_type c_sock;
};

#endif
