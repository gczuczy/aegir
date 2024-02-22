/*
  ThreadPool
 */

#include "DB.hh"

#include <stdio.h>

#include <catch2/catch_test_macros.hpp>

static auto makeDBInstance() {
  auto db = aegir::fermd::DB::Connection::getInstance();
  db->setConnectionFile(tmpnam(0));
  return db;
}

TEST_CASE("DB", "[db][fermd]") {
  auto db = makeDBInstance();

  db->init();

  // fermenter_types
  {
    auto txn = db->txn();
    auto fts = txn->getFermenterTypes();
    REQUIRE(fts.size() == 6);

    // insert
    aegir::fermd::DB::fermenter_types nft;
    nft.capacity = 20;
    nft.name = "shitbucket";
    nft.imageurl = "http://dev.null/";
    auto nftptr = txn.addFermenterType(nft);
    REQUIRE(nftptr != nullptr);
    REQUIRE(nftptr->capacity == nft.capacity);
    REQUIRE(nftptr->name == nft.name);
    REQUIRE(nftptr->imageurl == nft.imageurl);

    // update
    nft.id = nftptr->id;
    nft.capacity = 25;
    nft.name = "goopoo";
    nft.imageurl = "https://dev.secure.null/";
    txn.updateFermenterType(nft);

    auto uftptr = txn->getFermenterTypeByID(nft.id);
    REQUIRE(uftptr != nullptr);
    REQUIRE(uftptr->capacity == nft.capacity);
    REQUIRE(uftptr->name == nft.name);
    REQUIRE(uftptr->imageurl == nft.imageurl);

    // delete
    txn.deleteFermenterType(nft.id);
    REQUIRE(txn->getFermenterTypeByID(nft.id) == nullptr);
  }

  // tilts
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
