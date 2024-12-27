
#ifndef AEGIR_FERMD_TESTS_PRBASE
#define AEGIR_FERMD_TESTS_PRBASE

#include "fermd/DB.hh"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <chrono>
#include <functional>
#include <set>

#include "common/Message.hh"
#include "common/tests/common.hh"
#include "common/ServiceManager.hh"
#include "DBConnection.hh"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

using Catch::Matchers::WithinRel;

class DBTestSM: public aegir::ServiceManager {
public:
  DBTestSM();
  virtual ~DBTestSM();
};

class DBFixture {
public:
  DBFixture();
  virtual ~DBFixture();

protected:
  inline aegir::fermd::DB::Connection::pointer_type db() {
    return c_db;
  };

private:
  FileGuard c_fg;
  DBTestSM c_sm;
  aegir::fermd::DB::Connection::pointer_type c_db;
};

#endif
