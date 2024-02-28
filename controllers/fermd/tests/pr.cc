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

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

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
      CHECK( msg );
      CHECK( isError(msg) );
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
      CHECK( msg );
      CHECK( isError(msg) );
  });
}

TEST_CASE("pr_getFermenterTypes", "[fermd][pr]") {
  doTest([](aegir::zmqsocket_type csock) {
    std::string cmd("{\"command\": \"getFermenterTypes\"}");

      csock->send(cmd, true);

      auto msg = csock->recvRaw(true);
      CHECK( msg );
      CHECK( !isError(msg) );
      INFO("Response: " << ((char*)msg->data()));

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
