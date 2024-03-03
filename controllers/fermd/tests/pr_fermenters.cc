
#include "fermd/tests/prbase.hh"

TEST_CASE_METHOD(PRFixture, "pr_getFermenters", "[fermd][pr]") {
  std::string cmd("{\"command\": \"getFermenters\"}");

  auto msg = send(cmd);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // now verify these
  auto dbfs = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getFermenters();

  auto indata = c4::to_csubstr((char*)msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();

  std::set<int> ids;
  for (ryml::ConstNodeRef node: root["data"].children()) {
    aegir::fermd::DB::fermenter f;
    int ftid;
    node["id"] >> f.id;
    node["name"] >> f.name;
    node["type"]["id"] >> ftid;
    ids.insert(f.id);

    bool found{false};
    for (auto& dbit: dbfs) {
      if ( dbit->id == f.id ) {
	found = true;
	break;
      }
    }
    REQUIRE(found);
  }
  for (auto& dbit: dbfs) {
    INFO("Checking id: " << dbit->id);
    REQUIRE( ids.find(dbit->id) != ids.end() );
  }
}

TEST_CASE_METHOD(PRFixture, "pr_addFermenter", "[fermd][pr]") {
  std::string name{"unittest"};
  auto db = aegir::ServiceManager::get<aegir::fermd::DB::Connection>();
  int ftid = db->getFermenterTypes().front()->id;

  char buff[512];
  size_t bufflen;
  bufflen = snprintf(buff, sizeof(buff)-1,
		     "{\"command\": \"addFermenter\","
		     "\"data\": {\"name\": \"%s\","
		     "\"type\": {\"id\": %i}}}",
		     name.c_str(), ftid);
  std::string cmd(buff, bufflen);
  auto msg = send(cmd);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // verify we have the new tuple returned
  auto indata = c4::to_csubstr((char*)msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();
  ryml::ConstNodeRef node = root["data"];

  REQUIRE( node.has_child("id") );
  REQUIRE( node.has_child("name") );
  REQUIRE( node.has_child("type") );
  REQUIRE( node["type"].has_child("id") );

  int fid;
  {
    int nftid;
    std::string nname;

    node["id"] >> fid;
    node["name"] >> nname;
    node["type"]["id"] >> nftid;

    REQUIRE( nname == name );
    REQUIRE( nftid == ftid );
  }

  // now verify the new one in the DB
  auto dbf = db->getFermenterByID(fid);

  REQUIRE( dbf->name == name );
  REQUIRE( dbf->fermenter_type->id == ftid );
}

TEST_CASE_METHOD(PRFixture, "pr_updateFermenter", "[fermd][pr]") {
  auto db = aegir::ServiceManager::get<aegir::fermd::DB::Connection>();
  auto dbfts = db->getFermenterTypes();
  std::string newname{"unittest"};

  auto f = *db->getFermenters().front();
  int ftid;
  for (const auto& it: dbfts) {
    if ( it->id != f.fermenter_type->id ) {
      ftid = it->id;
      break;
    }
  }

  char buff[512];
  size_t bufflen;
  bufflen = snprintf(buff, sizeof(buff)-1,
		     "{\"command\": \"updateFermenter\","
		     "\"data\": {\"name\": \"%s\", \"id\": %i, "
		     "\"type\": {\"id\": %i}}}",
		     newname.c_str(), f.id, ftid);
  std::string cmd(buff, bufflen);

  auto msg = send(cmd);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // verify the update
  auto uf = db->getFermenterByID(f.id);
  REQUIRE( uf->name == newname );
  REQUIRE( uf->fermenter_type->id == ftid);
}
