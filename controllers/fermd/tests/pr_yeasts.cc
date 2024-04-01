
#include <iostream>

#include "fermd/tests/prbase.hh"

TEST_CASE_METHOD(PRFixture, "pr_getYeasts", "[fermd][pr][yeasts]") {
  std::string cmd("{\"command\": \"getYeasts\"}");
  auto msg = send(cmd);

  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // now verify these
  auto dbys = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getYeasts();

  auto indata = c4::to_csubstr((char*)msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();
  REQUIRE( root.has_child("data") );

  std::set<int> ids;
  for (ryml::ConstNodeRef node: root["data"].children()) {
    aegir::fermd::DB::yeast y;
    node["id"] >> y.id;
    node["name"] >> y.name;
    node["abv"] >> y.abv;
    node["attenuation"] >> y.attenuation;
    node["mintemp"] >> y.mintemp;
    node["maxtemp"] >> y.maxtemp;
    ids.insert(y.id);

    bool found{false};
    for (auto& dbit: dbys) {
      if ( dbit->id == y.id ) {
	found = true;
	break;
      }
    }
    REQUIRE(found);
  }
  for (auto& dbit: dbys) {
    INFO("Checking id: " << dbit->id);
    REQUIRE( ids.find(dbit->id) != ids.end() );
  }

}

TEST_CASE_METHOD(PRFixture, "pr_addYeast", "[fermd][pr][yeasts]") {
  std::string name{"unityeast"};
  float attenuation = 60;
  float abv = 13;
  float mintemp = 15;
  float maxtemp = 17;

  char buff[512];
  size_t bufflen;
  bufflen = snprintf(buff, sizeof(buff)-1,
		     "{\"command\": \"addYeast\","
		     "\"data\": {\"name\": \"%s\","
		     " \"attenuation\": \"%.1f\","
		     " \"abv\": \"%.1f\","
		     " \"mintemp\": \"%.1f\","
		     " \"maxtemp\": \"%.1f\","
		     "}}",
		     name.c_str(), attenuation,
		     abv, mintemp, maxtemp);
  std::string cmd(buff, bufflen);
  auto msg = send(cmd);

  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // verify we have the new tuple returned
  auto indata = c4::to_csubstr((char*)msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();
  REQUIRE( root.has_child("data") );
  ryml::ConstNodeRef node = root["data"];

  REQUIRE( node.has_child("id") );
  REQUIRE( node.has_child("name") );
  REQUIRE( node.has_child("attenuation") );
  REQUIRE( node.has_child("abv") );
  REQUIRE( node.has_child("mintemp") );
  REQUIRE( node.has_child("maxtemp") );

  int yid;
  {
    std::string nname;
    float nattenuation, nabv, nmintemp, nmaxtemp;
    node["id"] >> yid;
    node["name"] >> nname;
    node["attenuation"] >> nattenuation;
    node["abv"] >> nabv;
    node["mintemp"] >> nmintemp;
    node["maxtemp"] >> nmaxtemp;

    REQUIRE( nname == name );
    REQUIRE_THAT(nattenuation, WithinRel(attenuation));
    REQUIRE_THAT(nabv, WithinRel(abv));
    REQUIRE_THAT(nmintemp, WithinRel(mintemp));
    REQUIRE_THAT(nmaxtemp, WithinRel(maxtemp));
  }

  // now verify the new one in the DB
  auto dby = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getYeastByID(yid);

  REQUIRE( dby->name == name );
  REQUIRE_THAT(dby->attenuation, WithinRel(attenuation));
  REQUIRE_THAT(dby->abv, WithinRel(abv));
  REQUIRE_THAT(dby->mintemp, WithinRel(mintemp));
  REQUIRE_THAT(dby->maxtemp, WithinRel(maxtemp));
}

TEST_CASE_METHOD(PRFixture, "pr_updateYeast", "[fermd][pr][yeasts]") {
  std::string newname{"unityeast"};
  float newattenuation = 60;
  float newabv = 13;
  float newmintemp = 15;
  float newmaxtemp = 17;

  auto dbys = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getYeasts();

  auto y = *dbys.front();

  char buff[512];
  size_t bufflen;
  bufflen = snprintf(buff, sizeof(buff)-1,
		     "{\"command\": \"updateYeast\","
		     "\"data\": {\"id\": %i,"
		     "\"name\": \"%s\","
		     " \"attenuation\": \"%.1f\","
		     " \"abv\": \"%.1f\","
		     " \"mintemp\": \"%.1f\","
		     " \"maxtemp\": \"%.1f\","
		     "}}", y.id,
		     newname.c_str(), newattenuation,
		     newabv, newmintemp, newmaxtemp
		     );
  std::string cmd(buff, bufflen);

  auto msg = send(cmd);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // verify we have the new tuple returned
  auto indata = c4::to_csubstr((char*)msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();
  REQUIRE( root.has_child("data") );
  ryml::ConstNodeRef node = root["data"];

  REQUIRE( node.has_child("id") );
  REQUIRE( node.has_child("name") );
  REQUIRE( node.has_child("attenuation") );
  REQUIRE( node.has_child("abv") );
  REQUIRE( node.has_child("mintemp") );
  REQUIRE( node.has_child("maxtemp") );
  {
    std::string nname;
    float nattenuation, nabv, nmintemp, nmaxtemp;
    node["name"] >> nname;
    node["attenuation"] >> nattenuation;
    node["abv"] >> nabv;
    node["mintemp"] >> nmintemp;
    node["maxtemp"] >> nmaxtemp;

    REQUIRE( nname == newname );
    REQUIRE_THAT(nattenuation, WithinRel(newattenuation));
    REQUIRE_THAT(nabv, WithinRel(newabv));
    REQUIRE_THAT(nmintemp, WithinRel(newmintemp));
    REQUIRE_THAT(nmaxtemp, WithinRel(newmaxtemp));
  }

  // now verify the new one in the DB
  auto dby = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getYeastByID(y.id);

  REQUIRE( dby->name == newname );
  REQUIRE_THAT(dby->attenuation, WithinRel(newattenuation));
  REQUIRE_THAT(dby->abv, WithinRel(newabv));
  REQUIRE_THAT(dby->mintemp, WithinRel(newmintemp));
  REQUIRE_THAT(dby->maxtemp, WithinRel(newmaxtemp));
}

TEST_CASE_METHOD(PRFixture, "pr_deleteYeast", "[fermd][pr][yeasts]") {
  std::string newname{"unityeast"};
  float newattenuation = 60;
  float newabv = 13;
  float newmintemp = 15;
  float newmaxtemp = 17;

  auto dbys = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getYeasts();

  auto y = *dbys.front();

  char buff[512];
  size_t bufflen;
  bufflen = snprintf(buff, sizeof(buff)-1,
		     "{\"command\": \"deleteYeast\","
		     "\"data\": {\"id\": %i,}}", y.id);
  std::string cmd(buff, bufflen);

  auto msg = send(cmd);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // now verify the new one in the DB
  auto dby = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getYeastByID(y.id);

  REQUIRE( dby == nullptr );
}
