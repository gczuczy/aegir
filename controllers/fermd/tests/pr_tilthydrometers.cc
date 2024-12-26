#include "fermd/tests/prbase.hh"

TEST_CASE_METHOD(PRFixture, "pr_getTilthydrometers", "[fermd][pr]") {
  std::string cmd("{\"command\": \"getTilthydrometers\"}");

  auto msg = send(cmd);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // now verify these
  auto dbth = aegir::ServiceManager
    ::get<aegir::fermd::DB::Connection>()->getTilthydrometers();

  auto indata = c4::to_csubstr((char*)msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();

  std::set<int> ids;
  for (ryml::ConstNodeRef node: root["data"].children()) {
    aegir::fermd::DB::tilthydrometer th;
    node["id"] >> th.id;
    ids.insert(th.id);

    bool found{false};
    for (auto& dbit: dbth) {
      if ( dbit->id == th.id ) {
	found = true;
	break;
      }
    }
    REQUIRE(found);
  }
  for (auto& dbit: dbth) {
    INFO("Checking id: " << dbit->id);
    REQUIRE( ids.find(dbit->id) != ids.end() );
  }
}

TEST_CASE_METHOD(PRFixture, "pr_updateTilthydrometer", "[fermd][pr]") {
  auto db = aegir::ServiceManager::get<aegir::fermd::DB::Connection>();
  int thid = db->getTilthydrometers().front()->id;
  int fid = db->getFermenters().front()->id;
  bool enabled = true;
  float calibr_null=1.002,
    calibr_sg = 1.042, calibr_at = 1.069;

  char buff[512];
  size_t bufflen;
  bufflen = snprintf(buff, sizeof(buff)-1,
		     "{\"command\": \"updateTilthydrometer\","
		     "\"data\": {\"id\": %i, \"enabled\": %s, "
		     "\"fermenter\": {\"id\": %i},"
		     "\"calibr_null\": %.3f, "
		     "\"calibr_sg\": %.3f, \"calibr_at\": %.3f}}",
		     thid, enabled?"true":"false", fid,
		     calibr_null, calibr_sg, calibr_at);
  std::string cmd(buff, bufflen);

  auto msg = send(cmd);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  // verify the update
  auto th = db->getTilthydrometerByID(thid);
  REQUIRE( th->enabled == enabled );
  REQUIRE( th->fermenter );
  REQUIRE( th->fermenter->id == fid);
  REQUIRE( th->calibr_null );
  REQUIRE( th->calibr_sg );
  REQUIRE_THAT( th->calibr_null->sg, WithinRel(calibr_null) );
  REQUIRE_THAT( th->calibr_sg->at, WithinRel(calibr_at) );
  REQUIRE_THAT( th->calibr_sg->sg, WithinRel(calibr_sg) );
}
