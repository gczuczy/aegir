/*
  ThreadPool
 */

#include "DB.hh"

#include <stdio.h>

#include "common/ServiceManager.hh"
#include "DBConnection.hh"
#include "common/Message.hh"

#include <catch2/catch_test_macros.hpp>

class DBTestSM: public aegir::ServiceManager {
public:
  DBTestSM() {
    add<aegir::MessageFactory>();
    add<aegir::fermd::DB::Connection>();
  };
  virtual ~DBTestSM() {};
};

static auto makeDBInstance() {
  auto db = aegir::ServiceManager::get<aegir::fermd::DB::Connection>();
  db->setConnectionFile(tmpnam(0));
  return db;
}

TEST_CASE("DB", "[db][fermd]") {
  DBTestSM sm;
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

  // fermenter_types
  {
    auto txn = db->txn();
    auto fs = txn->getFermenters();
    REQUIRE(fs.size() == 2);

    // insert
    aegir::fermd::DB::fermenter nf;
    nf.name = "third";
    nf.fermenter_type = txn->getFermenterTypes().front();
    int newid;
    {
      auto nfptr = txn.addFermenter(nf);
      newid = nfptr->id;
      REQUIRE(nfptr != nullptr);
      REQUIRE(nfptr->name == nf.name);
      REQUIRE(nfptr->fermenter_type->id == nf.fermenter_type->id);
    }

    // update
    nf.id = newid;
    nf.name = "thirdy";
    txn.updateFermenter(nf);

    {
      auto ufptr = txn->getFermenterByID(nf.id);
      REQUIRE(ufptr != nullptr);
      REQUIRE(ufptr->name == nf.name);
      REQUIRE(ufptr->fermenter_type->id == nf.fermenter_type->id);
    }

    // delete
    txn.deleteFermenter(nf.id);
    REQUIRE(txn->getFermenterByID(nf.id) == nullptr);
  }

  // tilts
  {
    auto txn = db->txn();
    auto tilts = txn->getTilthydrometers();
    REQUIRE(tilts.size() == 8);

    auto& cth = *tilts.begin();
    aegir::fermd::DB::tilthydrometer th = *cth;
    th.enabled = true;
    txn.setTilthydrometer(th);

    // now verify
    auto th2 = txn->getTilthydrometerByUUID(th.uuid);
    REQUIRE(th2->enabled == th.enabled);
  }
}
