/*
  PR thread/pool testing
 */

#include "common/ThreadManager.hh"
#include "fermd/PRThread.hh"
#include "fermd/ZMQConfig.hh"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <set>

#include "common/ServiceManager.hh"
#include "DBConnection.hh"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

using Catch::Matchers::WithinRel;

class PRManager: public aegir::ThreadManager {

  friend aegir::ServiceManager;
protected:
  PRManager(): aegir::ThreadManager() {
    registerHandler<aegir::fermd::PRThread>("PR");
  };

public:
  virtual ~PRManager() {};
};

class PRTestSM: public aegir::ServiceManager {
public:
  PRTestSM() {
    add<aegir::fermd::ZMQConfig>();
    add<aegir::fermd::PRThread>();
    add<aegir::fermd::DB::Connection>();
    add<PRManager>();
  };
  virtual ~PRTestSM() {};
};

class DBFileGuard {
public:
  DBFileGuard() {
    char *x;
    x = tempnam(0, "tests.fermd.");
    if ( x ) {
      c_filename = std::string(x);
      free(x);
    } else {
      throw std::runtime_error("Unable to allocate tempfile name");
    }
  };
  ~DBFileGuard() {
    std::filesystem::remove(c_filename);
  }
  inline std::string getFilename() const {return c_filename;};
private:
  std::string c_filename;
};

static void runManager() {
  auto tm = aegir::ServiceManager::get<PRManager>();

  tm->run();
}

void doTest(std::function<void(aegir::zmqsocket_type)> _tests) {
  DBFileGuard dbfg;
  PRTestSM sm;
  auto db = sm.get<aegir::fermd::DB::Connection>();
  db->setConnectionFile(dbfg.getFilename());
  db->init();
  auto tm = sm.get<PRManager>();

  std::thread tmt(runManager);

  auto csock = sm.get<aegir::fermd::ZMQConfig>()
    ->srcSocket("prpublic");
  csock->brrr();

  std::chrono::milliseconds s(10);
  while ( !tm->isRunning() )
    std::this_thread::sleep_for(s);

  try {
    _tests(csock);
  }
  catch (aegir::Exception& e) {
    FAIL("Got exception: " << e.what());
  }
  catch (std::exception& e) {
    FAIL("Got exception: " << e.what());
  }
  catch (...) {
    FAIL("Got unknown exception");
  }
  tm->stop();
  tmt.join();
}

bool isError(aegir::RawMessage::ptr& msg) {
  auto indata = c4::to_csubstr((char*)msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();
  return root["status"] == "error";
}

TEST_CASE("pr_nocmd", "[fermd][pr]") {

  doTest([](aegir::zmqsocket_type csock) {
    ryml::Tree tree;
    ryml::NodeRef root = tree.rootref();
    root |= ryml::MAP;
    root["command"] = "bullShit";
    root["data"] |= ryml::MAP;
    std::string output = ryml::emitrs_yaml<std::string>(tree);
    csock->send(output, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << ((char*)msg->data()));
    REQUIRE( isError(msg) );
  });
}

TEST_CASE("pr_randomdata", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
    std::string output("foo");
      output.resize(32);

      for (int i=0; i<32; i += sizeof(int) ) {
	int rnd = rand();
	memcpy((void*)(output.data()+i), &rnd, sizeof(int));
      }

      csock->send(output, true);

      auto msg = csock->recvRaw(true);
      REQUIRE( msg );
      INFO("Response: " << ((char*)msg->data()));
      REQUIRE( isError(msg) );
  });
}

TEST_CASE("pr_getFermenterTypes", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
    std::string cmd("{\"command\": \"getFermenterTypes\"}");
    INFO("Request: " << cmd);

    csock->send(cmd, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << ((char*)msg->data()));
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

  });
}

TEST_CASE("pr_addFermenterTypes", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
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
    INFO("Request: " << cmd);

    csock->send(cmd, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << ((char*)msg->data()));
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
  });
}

TEST_CASE("pr_updateFermenterTypes", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
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
    INFO("Request: " << cmd);

    csock->send(cmd, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << ((char*)msg->data()));
    REQUIRE( !isError(msg) );

    // now verify the new one in the DB
    auto dbft = aegir::ServiceManager
      ::get<aegir::fermd::DB::Connection>()->getFermenterTypeByID(ft.id);

    REQUIRE( dbft->capacity == newcapacity );
    REQUIRE( dbft->name == newname );
    REQUIRE( dbft->imageurl == newimageurl );
  });
}

TEST_CASE("pr_getFermenters", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
    std::string cmd("{\"command\": \"getFermenters\"}");
    INFO("Request: " << cmd);

    csock->send(cmd, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << ((char*)msg->data()));
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

  });
}

TEST_CASE("pr_addFermenter", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
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
    INFO("Request: " << cmd);

    csock->send(cmd, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << ((char*)msg->data()));
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
  });
}

TEST_CASE("pr_updateFermenter", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
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
    INFO("Request: " << cmd);

    csock->send(cmd, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << ((char*)msg->data()));
    REQUIRE( !isError(msg) );

    // verify the update
    auto uf = db->getFermenterByID(f.id);
    REQUIRE( uf->name == newname );
    REQUIRE( uf->fermenter_type->id == ftid);
  });
}

TEST_CASE("pr_getTilthydrometers", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
    std::string cmd("{\"command\": \"getTilthydrometers\"}");
    INFO("Request: " << cmd);

    csock->send(cmd, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << ((char*)msg->data()));
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

  });
}

TEST_CASE("pr_updateTilthydrometer", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
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
    INFO("Request: " << cmd);

    csock->send(cmd, true);

    auto msg = csock->recvRaw(true);
    REQUIRE( msg );
    INFO("Response: " << (char*)msg->data());
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
  });
}
