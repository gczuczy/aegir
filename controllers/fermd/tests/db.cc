/*
  ThreadPool
 */

#include "DB.hh"

#include <stdio.h>

#include <catch2/catch_test_macros.hpp>

static auto makeDBInstance() {
  auto db = aegir::fermd::DB::getInstance();
  db->setDBfile(tmpnam(0));
  return db;
}

TEST_CASE("DB", "[db][fermd]") {
  makeDBInstance();
  auto db = aegir::fermd::DB::getInstance();

  db->init();

  auto tilts = db->getTilthydrometers();
  REQUIRE(tilts.size() == 8);
}
