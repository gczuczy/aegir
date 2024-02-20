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

  {
    auto txn = db->txn();
    auto tilts = txn->getTilthydrometers();
    REQUIRE(tilts.size() == 8);

    auto& cth = *tilts.begin();
    aegir::fermd::DB::tilthydrometer th = *cth;
    th.enabled = true;
    th.active = true;
    txn.setTilthydrometer(th);

    // now verify
    auto th2 = txn->getTilthydrometerByUUID(th.uuid);
    REQUIRE(th2->active == th.active);
    REQUIRE(th2->enabled == th.enabled);
  }
}
