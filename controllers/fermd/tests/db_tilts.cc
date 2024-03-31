#include "fermd/tests/dbbase.hh"

TEST_CASE_METHOD(DBFixture, "db_tilts", "[fermd][db][tilts]") {
  auto txn = db()->txn();
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
