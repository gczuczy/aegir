#include "fermd/tests/dbbase.hh"

TEST_CASE_METHOD(DBFixture, "db_fermenters", "[fermd][db][fermenters]") {
  auto txn = db()->txn();
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
