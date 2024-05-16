
#include "fermd/tests/prbase.hh"

#include <time.h>
#include <math.h>

#include <ctime>
#include <iostream>

class PRBrewFixture: public PRFixture {
public:
  PRBrewFixture(): PRFixture() {
  }
  virtual ~PRBrewFixture() {};
protected:
  aegir::RawMessage::ptr addBrew(std::string _name,
				 aegir::fermd::DB::yeast::cptr _yeast = nullptr) {
    auto yeast = _yeast;
    if ( !yeast ) {
      yeast = *aegir::ServiceManager::get<aegir::fermd::DB::Connection>()
	->getYeasts().begin();
    }
    return send("{\"command\": \"addBrew\","
		"\"data\": {"
		"\"name\": \"%s\","
		"\"yeast\": {\"id\": %i}"
		"}}",
		_name.c_str(),
		yeast->id);
  }
};

TEST_CASE_METHOD(PRBrewFixture, "pr_addBrew", "[fermd][pr][brews]") {
  std::string name{"test-1"}, currdate;
  auto msg = addBrew(name);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );

  {
    std::tm tm;
    time_t now = std::time(nullptr);
    gmtime_r(&now, &tm);
    char buff[32];
    auto len = std::strftime(buff, 31, "%Y-%m-%d", &tm);
    currdate = std::string(buff, len);
  }

  int brewid;
  {
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    bool finished;
    data["finished"] >> finished;
    data["id"] >> brewid;
    REQUIRE( data["name"].val() == name );
    REQUIRE( data["brewdate"].val() == currdate );
    REQUIRE( finished == false );
  }

  // fetch it from the DB
  {
    auto dbbrew = aegir::ServiceManager::get<aegir::fermd::DB::Connection>()
      ->getBrewByID(brewid);
    REQUIRE( dbbrew->name == name );
    REQUIRE( dbbrew->brewdate == currdate );
    REQUIRE( dbbrew->finished == false );
  }
}

TEST_CASE_METHOD(PRBrewFixture, "pr_getBrews", "[fermd][pr][brews]") {
  {
    std::string name{"test-1"};
    auto msg = addBrew(name);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
  }

  {
    std::string name{"test-2"};
    auto msg = addBrew(name);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
  }

  {
    auto msg = send("{\"command\": \"getBrews\","
		    "\"data\": {}}");
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    REQUIRE( data.is_seq() );

    int nbrews{0};
    for (ryml::ConstNodeRef node: data.children()) {
      ++nbrews;
    }
    REQUIRE( nbrews == 2 );
  }
}

TEST_CASE_METHOD(PRBrewFixture, "pr_getBrew", "[fermd][pr][brews]") {
  std::string currdate, name1{"test-1"}, name2{"test-2"};
  int id1, id2;
  {
    auto msg = addBrew(name1);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    data["id"] >> id1;
  }

  {
    auto msg = addBrew(name2);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    data["id"] >> id2;
  }

  {
    std::tm tm;
    time_t now = std::time(nullptr);
    gmtime_r(&now, &tm);
    char buff[32];
    auto len = std::strftime(buff, 31, "%Y-%m-%d", &tm);
    currdate = std::string(buff, len);
  }

  {
    auto msg = send("{\"command\": \"getBrew\","
			"\"data\": {\"id\": %i}}",
			id1);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    REQUIRE( data.is_map() );

    int id;
    data["id"] >> id;

    REQUIRE( id == id1 );
    REQUIRE( data["name"].val() == name1 );
    REQUIRE( data["brewdate"].val() == currdate );
  }
}

TEST_CASE_METHOD(PRBrewFixture, "pr_updateBrew", "[fermd][pr][brews]") {
  std::string currdate, name1{"test-1"};
  int id1, id2;
  {
    auto msg = addBrew(name1);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    data["id"] >> id1;
  }

  {
    auto yeasts = aegir::ServiceManager::get<aegir::fermd::DB::Connection>()
      ->getYeasts();
    auto newyeast = *(++yeasts.begin());
    float newsgoffset(0.010), neworiginalsg(1.040);
    std::string newname{"NewName-42"};
    auto msg = send("{\"command\": \"updateBrew\","
			"\"data\": {"
			"\"id\": %i,"
			"\"name\": \"%s\","
			"\"finished\": true,"
			"\"sgoffset\": %.3f,"
			"\"originalsg\": %.3f,"
			"\"yeast\": {\"id\": %i}"
			"}}",
			id1, newname.c_str(), newsgoffset,
			neworiginalsg, newyeast->id);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    REQUIRE( data.is_map() );

    int id;
    data["id"] >> id;
    bool finished;
    data["finished"] >> finished;
    float sgoffset, originalsg;
    data["sgoffset"] >> sgoffset;
    data["originalsg"] >> originalsg;

    REQUIRE( id == id1 );
    REQUIRE( data["name"].val() == newname );
    REQUIRE( finished == true );
    REQUIRE_THAT( sgoffset, WithinRel(newsgoffset) );
    REQUIRE_THAT( originalsg, WithinRel(neworiginalsg) );
  }
}

TEST_CASE_METHOD(PRBrewFixture, "pr_deleteBrew", "[fermd][pr][brews]") {
  std::string name{"test-1"}, currdate;
  int brewid;
  {
    auto msg = addBrew(name);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    data["id"] >> brewid;
  }

  auto msg = send("{\"command\": \"deleteBrew\", \"data\": "
		  "\{\"id\": %i}}", brewid);
  REQUIRE( msg );
  REQUIRE( !isError(msg) );
}

TEST_CASE_METHOD(PRBrewFixture, "pr_transferBrew", "[fermd][pr][brews]") {
  std::string name{"test-1"}, currdate;
  int brewid;
  {
    auto msg = addBrew(name);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    data["id"] >> brewid;
  }


  auto fermenters = aegir::ServiceManager::get<aegir::fermd::DB::Connection>()
    ->getFermenters();
  int fid1 = (*fermenters.begin())->id;
  // transfer it to a fermenter
  int tfid1, tfid2;
  {
    auto msg = send("{\"command\": \"transferBrew\", \"data\": {"
		    "\"brew\": {\"id\": %i},"
		    "\"fermenter\": {\"id\": %i}"
		    "}}", brewid, fid1);
    REQUIRE( (msg && !isError(msg)) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    data["id"] >> tfid1;
  }
  // check we have this transfer
  {
    auto tf = aegir::ServiceManager::get<aegir::fermd::DB::Connection>()
      ->getTransferByID(tfid1);
    REQUIRE( tf );
    REQUIRE( tf->fermenter->id == fid1 );
    REQUIRE( tf->brew->id == brewid );
  }

  // another transfer to the other fermenter
  int fid2 = (*++fermenters.begin())->id;
  {
    auto msg = send("{\"command\": \"transferBrew\", \"data\": {"
		    "\"brew\": {\"id\": %i},"
		    "\"fermenter\": {\"id\": %i}"
		    "}}", brewid, fid2);
    REQUIRE( (msg && !isError(msg)) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    data["id"] >> tfid2;
  }
  // check we have this transfer
  {
    auto tf = aegir::ServiceManager::get<aegir::fermd::DB::Connection>()
      ->getTransferByID(tfid2);
    REQUIRE( tf );
    REQUIRE( tf->fermenter->id == fid2 );
    REQUIRE( tf->brew->id == brewid );
  }

  // check the transfers in the getBrews
  {
    auto msg = send("{\"command\": \"getBrew\","
		    "\"data\": {\"id\": %i}}",
		    brewid);
    REQUIRE( (msg && !isError(msg)) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];

    for (ryml::ConstNodeRef node: data["transfers"].children()) {
      int id;
      node["id"] >> id;
      REQUIRE( (id == tfid1 || id == tfid2) );
    }
  }
}

TEST_CASE_METHOD(PRBrewFixture, "pr_getBrew_fermlog", "[fermd][pr][brews]") {
  std::string name{"test-1"}, currdate;
  int brewid;
  {
    auto msg = addBrew(name);
    REQUIRE( msg );
    REQUIRE( !isError(msg) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];
    data["id"] >> brewid;
  }

  auto db = aegir::ServiceManager::get<aegir::fermd::DB::Connection>();
  auto brew = db->getBrewByID(brewid);
  std::list<aegir::fermd::DB::fermentationlog::cptr> dbflog;
  {
    auto txn = db->txn();
    aegir::fermd::DB::fermentationlog fl;
    auto now = std::time(0);
    now = now - (now%3600); // floor 1 hour

    float basesg = 1.044f;
    float basetemp = 18.0f;
    fl.brew = brew;
    for (int i=0; i<16; ++i) {
      fl.timestamp = now + 3600*i;
      fl.temperature = basetemp + sin(i)*1.2;
      fl.sg = i==0 ? basesg : basesg - atan(i)/100;
      CAPTURE(i, fl.timestamp, fl.temperature, fl.sg);
      dbflog.push_back(txn.addFermentationlog(fl));
    }
  }

  // now query it from PR
  // check the transfers in the getBrews
  {
    auto msg = send("{\"command\": \"getBrew\","
		    "\"data\": {\"id\": %i}}",
		    brewid);
    REQUIRE( (msg && !isError(msg)) );
    auto indata = c4::to_csubstr((char*)msg->data());
    ryml::Tree tree = ryml::parse_in_arena(indata);
    ryml::NodeRef data = tree.rootref()["data"];

    int entries=0;
    for (ryml::ConstNodeRef node: data["fermentationlog"].children()) {
      ++entries;
      int id;
      float sg, temp;
      CAPTURE(node);
      node["id"] >> id;
      node["sg"] >> sg;
      node["temperature"] >> temp;
      bool found = false;
      CAPTURE(id, sg, temp);
      for (auto& it: dbflog) {
	if ( id != it->id ) continue;
	found = true;
	REQUIRE_THAT( it->sg, Catch::Matchers::WithinAbs(sg, 0.001) );
	REQUIRE_THAT( it->temperature, Catch::Matchers::WithinAbs(temp, 0.1) );
	break;
      }
      CHECK( found );
    }
    REQUIRE( dbflog.size() == entries);
  }
}
