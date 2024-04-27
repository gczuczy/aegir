#include "fermd/tests/dbbase.hh"

TEST_CASE_METHOD(DBFixture, "db_transfers", "[fermd][db][transfers]") {

  aegir::fermd::DB::brew b;
  b.name = "test";
  b.yeast = *db()->getYeasts().begin();
  b.brewdate = "2024-04-27";
  b.sgoffset = 0;
  b.originalsg = 1.044f;
  b.finished = false;
  b.metadata = "{\"meta\": \"data\"}";
  // add a brew first
  {

    auto txn = db()->txn();
    auto newbrew = txn.addBrew(b);
    REQUIRE(newbrew->name == b.name);
    REQUIRE(newbrew->yeast->id == b.yeast->id);
    REQUIRE(newbrew->brewdate == b.brewdate);
    REQUIRE_THAT(newbrew->sgoffset, WithinRel(b.sgoffset));
    REQUIRE_THAT(newbrew->originalsg.value(), WithinRel(b.originalsg.value()));
    REQUIRE(newbrew->finished == b.finished);
    REQUIRE(newbrew->metadata.value() == b.metadata.value());
    b.id = newbrew->id;
  }

  // get the fermenters, we need at least 2
  auto fermenters = db()->getFermenters();
  REQUIRE(fermenters.size() >= 2);
  aegir::fermd::DB::fermenter::cptr f1, f2;
  {
    auto fit = fermenters.begin();
    f1 = *fit;
    f2 = *++fit;
  }

  // do 2 transfers
  aegir::fermd::DB::transfer::cptr t1, t2;
  {
    aegir::fermd::DB::transfer t;
    t.brew = db()->getBrewByID(b.id);
    t.fermenter = f1;
    t.transferdate = "2024-01-02";
    t1 = db()->txn().addTransfer(t);
    REQUIRE(t1->brew->id == t.brew->id);
    REQUIRE(t1->fermenter->id == t.fermenter->id);
    REQUIRE(t1->transferdate == t.transferdate);
    t.fermenter = f2;
    t.transferdate = "2024-01-03";
    t2 = db()->txn().addTransfer(t);
    REQUIRE(t2->brew->id == t.brew->id);
    REQUIRE(t2->fermenter->id == t.fermenter->id);
    REQUIRE(t2->transferdate == t.transferdate);
  }

  // verify that they are there
  {
    auto tf = db()->getTransferByID(t1->id);
    REQUIRE(t1->brew->id == tf->brew->id);
    REQUIRE(t1->fermenter->id == tf->fermenter->id);
    REQUIRE(t1->transferdate == tf->transferdate);
  }

  // list the transfers
  {
    auto transfers = db()->getTransfers();
    for (auto it: transfers) {
      REQUIRE( (it->id == t1->id || it->id == t2->id) );
      if ( it->id == t1->id ) {
	REQUIRE(t1->fermenter->id == it->fermenter->id);
	REQUIRE(t1->transferdate == it->transferdate);
      } else if ( it->id == t2->id ) {
	REQUIRE(t2->fermenter->id == it->fermenter->id);
	REQUIRE(t2->transferdate == it->transferdate);
      }
    }
  }
}
