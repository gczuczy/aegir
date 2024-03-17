
#include "fermd/tests/prbase.hh"

TEST_CASE_METHOD(PRFixture, "pr_getFermenterTypes", "[fermd][pr]") {
  std::string cmd("{\"command\": \"getFermenterTypes\"}");
  auto msg = send(cmd);

  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // now verify these
  auto dbfts = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getFermenterTypes();

  auto indata = c4::to_csubstr((char*)msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();

  std::set<int> ids;
  for (ryml::ConstNodeRef node: root["data"].children()) {
    aegir::fermd::DB::fermenter_types ft;
    node["id"] >> ft.id;
    node["capacity"] >> ft.capacity;
    node["name"] >> ft.name;
    node["imageurl"] >> ft.imageurl;
    ids.insert(ft.id);

    bool found{false};
    for (auto& dbit: dbfts) {
      if ( dbit->id == ft.id ) {
	found = true;
	break;
      }
    }
    REQUIRE(found);
  }
  for (auto& dbit: dbfts) {
    INFO("Checking id: " << dbit->id);
    REQUIRE( ids.find(dbit->id) != ids.end() );
  }

}

TEST_CASE_METHOD(PRFixture, "pr_addFermenterTypes", "[fermd][pr]") {
  int capacity{42};
  std::string name{"unittest"};
  std::string imageurl{"http://dev.null/"};

  char buff[512];
  size_t bufflen;
  bufflen = snprintf(buff, sizeof(buff)-1,
		     "{\"command\": \"addFermenterTypes\","
		     "\"data\": {\"capacity\": %i, \"name\": \"%s\","
		     "\"imageurl\": \"%s\"}}", capacity,
		     name.c_str(), imageurl.c_str());
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
  REQUIRE( node.has_child("capacity") );
  REQUIRE( node.has_child("name") );
  REQUIRE( node.has_child("imageurl") );

  int ftid;
  {
    int ncapacity;
    std::string nname, nimageurl;
    node["id"] >> ftid;
    node["capacity"] >> ncapacity;
    node["name"] >> nname;
    node["imageurl"] >> nimageurl;

    REQUIRE( ncapacity == capacity );
    REQUIRE( nname == name );
    REQUIRE( nimageurl == imageurl );
  }

  // now verify the new one in the DB
  auto dbft = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getFermenterTypeByID(ftid);

  REQUIRE( dbft->capacity == capacity );
  REQUIRE( dbft->name == name );
  REQUIRE( dbft->imageurl == imageurl );
}

TEST_CASE_METHOD(PRFixture, "pr_updateFermenterTypes", "[fermd][pr]") {
  int newcapacity{42};
  std::string newname{"unittest"};
  std::string newimageurl{"http://dev.null/"};

  auto dbfts = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getFermenterTypes();

  auto ft = *dbfts.front();

  char buff[512];
  size_t bufflen;
  bufflen = snprintf(buff, sizeof(buff)-1,
		     "{\"command\": \"updateFermenterTypes\","
		     "\"data\": {\"capacity\": %i, \"name\": \"%s\","
		     "\"imageurl\": \"%s\", \"id\": %i}}",
		     newcapacity, newname.c_str(),
		     newimageurl.c_str(), ft.id);
  std::string cmd(buff, bufflen);

  auto msg = send(cmd);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // now verify the new one in the DB
  auto dbft = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getFermenterTypeByID(ft.id);

  REQUIRE( dbft->capacity == newcapacity );
  REQUIRE( dbft->name == newname );
  REQUIRE( dbft->imageurl == newimageurl );
}
