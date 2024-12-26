#include "fermd/tests/dbbase.hh"

TEST_CASE_METHOD(DBFixture, "db_fermenter_types", "[fermd][db][fermenter_types]") {
  auto txn = db()->txn();
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
