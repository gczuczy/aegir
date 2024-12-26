#include "fermd/tests/dbbase.hh"

TEST_CASE_METHOD(DBFixture, "db_yeasts", "[fermd][db][yeasts]") {
  auto txn = db()->txn();
  auto yeasts = txn->getYeasts();
  REQUIRE(yeasts.size() == 13);

  auto& cy = *yeasts.begin();
  aegir::fermd::DB::yeast y = *cy;
  float abv = 15;
  float mintemp = 8;
  float maxtemp = 29;
  float attenuation = 99;
  y.abv = abv;
  y.mintemp = mintemp;
  y.maxtemp = maxtemp;
  y.attenuation = attenuation;
  txn.updateYeast(y);

  // now verify
  auto y2 = txn->getYeastByID(y.id);
  REQUIRE_THAT(y2->abv, WithinRel(abv));
  REQUIRE_THAT(y2->mintemp, WithinRel(mintemp));
  REQUIRE_THAT(y2->maxtemp, WithinRel(maxtemp));
  REQUIRE_THAT(y2->attenuation, WithinRel(attenuation));
}

TEST_CASE_METHOD(DBFixture, "db_yeasts_add", "[fermd][db][yeasts]") {
  float abv = 15;
  float mintemp = 8;
  float maxtemp = 29;
  float attenuation = 99;

  aegir::fermd::DB::yeast y;
  y.name = "Test Yeast";
  y.abv = abv;
  y.mintemp = mintemp;
  y.maxtemp = maxtemp;
  y.attenuation = attenuation;
  int yid;
  {
    auto txn = db()->txn();
    auto y2 = txn.addYeast(y);
    yid = y2->id;
    REQUIRE(y2->name == y.name);
    REQUIRE_THAT(y2->abv, WithinRel(abv));
    REQUIRE_THAT(y2->mintemp, WithinRel(mintemp));
    REQUIRE_THAT(y2->maxtemp, WithinRel(maxtemp));
    REQUIRE_THAT(y2->attenuation, WithinRel(attenuation));
  }

  auto yeasts = db()->getYeasts();
  bool found(false);
  for (auto& it: yeasts) {
    if ( it->id == yid ) {
      found = true;
      REQUIRE(it->name == y.name);
      REQUIRE_THAT(it->abv, WithinRel(abv));
      REQUIRE_THAT(it->mintemp, WithinRel(mintemp));
      REQUIRE_THAT(it->maxtemp, WithinRel(maxtemp));
      REQUIRE_THAT(it->attenuation, WithinRel(attenuation));
      break;
    }
  }
  REQUIRE(found);

  auto y2 = db()->getYeastByID(yid);
  REQUIRE(y2->name == y.name);
  REQUIRE_THAT(y2->abv, WithinRel(abv));
  REQUIRE_THAT(y2->mintemp, WithinRel(mintemp));
  REQUIRE_THAT(y2->maxtemp, WithinRel(maxtemp));
  REQUIRE_THAT(y2->attenuation, WithinRel(attenuation));
}
