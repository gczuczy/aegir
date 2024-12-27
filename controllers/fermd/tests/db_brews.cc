#include "fermd/tests/dbbase.hh"

TEST_CASE_METHOD(DBFixture, "db_brews", "[fermd][db][brews]") {

  aegir::fermd::DB::brew b;
  b.name = "test";
  b.yeast = *db()->getYeasts().begin();
  b.brewdate = "2024-04-27";
  b.sgoffset = 0;
  b.originalsg = 1.044f;
  b.finished = false;
  b.metadata = "{\"meta\": \"data\"}";
  // add a brew first
  int newid;
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
    newid = newbrew->id;
  }

  // verify it from byid
  {
    auto newbrew = db()->getBrewByID(newid);
    REQUIRE(newbrew->name == b.name);
    REQUIRE(newbrew->yeast->id == b.yeast->id);
    REQUIRE(newbrew->brewdate == b.brewdate);
    REQUIRE_THAT(newbrew->sgoffset, WithinRel(b.sgoffset));
    REQUIRE_THAT(newbrew->originalsg.value(), WithinRel(b.originalsg.value()));
    REQUIRE(newbrew->finished == b.finished);
    REQUIRE(newbrew->metadata.value() == b.metadata.value());
  }

  // verify it from getBrews
  {
    auto brews = db()->getBrews();
    REQUIRE(brews.size() == 1);
    auto newbrew = *brews.begin();
    REQUIRE(newbrew->name == b.name);
    REQUIRE(newbrew->yeast->id == b.yeast->id);
    REQUIRE(newbrew->brewdate == b.brewdate);
    REQUIRE_THAT(newbrew->sgoffset, WithinRel(b.sgoffset));
    REQUIRE_THAT(newbrew->originalsg.value(), WithinRel(b.originalsg.value()));
    REQUIRE(newbrew->finished == b.finished);
    REQUIRE(newbrew->metadata.value() == b.metadata.value());
  }

  // Update it
  b.id = newid;
  b.name = "test2";
  b.yeast = *(++(db()->getYeasts().begin()));
  b.brewdate = "2024-04-21";
  b.sgoffset = 0.010f;
  b.originalsg = 1.040f;
  b.finished = true;
  b.metadata = "{\"meta2\": \"data2\"}";
  {
    db()->txn().updateBrew(b);
    auto newbrew = db()->getBrewByID(newid);
    REQUIRE(newbrew->name == b.name);
    REQUIRE(newbrew->yeast->id == b.yeast->id);
    REQUIRE(newbrew->brewdate == b.brewdate);
    REQUIRE_THAT(newbrew->sgoffset, WithinRel(b.sgoffset));
    REQUIRE_THAT(newbrew->originalsg.value(), WithinRel(b.originalsg.value()));
    REQUIRE(newbrew->finished == b.finished);
    REQUIRE(newbrew->metadata.value() == b.metadata.value());
  }

  // and finally delete it
  {
    db()->txn().deleteBrew(newid);
    REQUIRE(db()->getBrews().size() == 0);
  }
}
