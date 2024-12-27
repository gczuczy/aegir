#include "fermd/tests/dbbase.hh"

#include <list>
#include <ctime>

TEST_CASE_METHOD(DBFixture, "db_fermentationlog", "[fermd][db][fermentationlog]") {

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

    auto newbrew = db()->txn().addBrew(b);
    REQUIRE(newbrew->name == b.name);
    REQUIRE(newbrew->yeast->id == b.yeast->id);
    REQUIRE(newbrew->brewdate == b.brewdate);
    REQUIRE_THAT(newbrew->sgoffset, WithinRel(b.sgoffset));
    REQUIRE_THAT(newbrew->originalsg.value(), WithinRel(b.originalsg.value()));
    REQUIRE(newbrew->finished == b.finished);
    REQUIRE(newbrew->metadata.value() == b.metadata.value());
    b.id = newbrew->id;
  }

  aegir::fermd::DB::fermentationlog fl;

  auto now = std::time(0);
  fl.brew = db()->getBrewByID(b.id);
  fl.timestamp = now;
  fl.sg = 1.060f;
  fl.temperature = 18.0f;

  std::list<aegir::fermd::DB::fermentationlog::cptr> fls;
  for (int i=0; i<10; ++i) {
    fl.timestamp += 1;
    fl.sg -= 0.001f;
    auto ret = db()->txn().addFermentationlog(fl);
    REQUIRE(ret->brew->id == fl.brew->id);
    REQUIRE(ret->timestamp == fl.timestamp);
    REQUIRE_THAT(ret->sg, WithinRel(fl.sg));
    REQUIRE_THAT(ret->temperature, WithinRel(fl.temperature));
    fls.push_back(ret);
  }

  // Read them back
  auto dbfls = db()->getFermentationlogsByBrew(b);
  REQUIRE(dbfls.size() == fls.size());
  for (auto it: dbfls) {
    bool found(false);
    for (auto it2: fls) {
      if (it2->id != it->id) continue;
      found = true;
      REQUIRE(it->brew->id == it2->brew->id);
      REQUIRE(it->timestamp == it2->timestamp);
      REQUIRE_THAT(it->sg, WithinRel(it2->sg));
      REQUIRE_THAT(it->temperature, WithinRel(it2->temperature));
      break;
    }
    REQUIRE(found);
  }
}
