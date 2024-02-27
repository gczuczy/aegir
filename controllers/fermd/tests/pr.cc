/*
  PR thread/pool testing
 */

#include "common/ThreadManager.hh"
#include "fermd/PRThread.hh"
#include "fermd/ZMQConfig.hh"

#include <stdio.h>

#include <chrono>
#include <filesystem>
#include <functional>

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
