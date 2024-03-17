/*
  Config tests
 */

#include "FermdConfig.hh"

#include <stdio.h>
#include <unistd.h>

#include <exception>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "common/ServiceManager.hh"
#include "PRThread.hh"
#include "ZMQConfig.hh"
#include "MainLoop.hh"
#include "Bluetooth.hh"
#include "DBConnection.hh"
#include "SensorProxy.hh"

#include <catch2/catch_test_macros.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

class FermdConfigTestSM: public aegir::ServiceManager {
public:
  FermdConfigTestSM(): aegir::ServiceManager() {
    try {
      add<aegir::MessageFactory>();
      add<aegir::fermd::ZMQConfig>();
      add<aegir::fermd::SensorProxy>();
      add<aegir::fermd::PRThread>();
      add<aegir::fermd::Bluetooth>();
      add<aegir::fermd::MainLoop>();
      add<aegir::fermd::DB::Connection>();
      add<aegir::fermd::FermdConfig>();
    }
    catch (aegir::Exception& e) {
      printf("FermdConfigTestSM excepton: %s\n", e.what());
      throw e;
    }
  };
  virtual ~FermdConfigTestSM() {
  };
};

class TempFileGuard {
public:
  TempFileGuard() {
    char *x;
    x = tempnam(0, "tests.fermd.");
    if ( x ) {
      c_filename = std::string(x);
      free(x);
    } else {
      throw std::runtime_error("Unable to allocate tempfile name");
    }
  };
  ~TempFileGuard() {
    std::filesystem::remove(c_filename);
  }
  inline std::string getFilename() const {return c_filename;};
private:
  std::string c_filename;
};

void readFile(const std::string& _fname, ryml::Tree &_tree) {

  std::ifstream f(_fname, std::ios::in | std::ios::binary);

  const auto size = std::filesystem::file_size(_fname);

  std::string contents(size, '\0');

  f.read(contents.data(), size);

  ryml::csubstr csrcview(contents.c_str(), contents.size());
  _tree = ryml::parse_in_arena(csrcview);
  f.close();
}

void writeYAML(const ryml::Tree& _tree, const std::string& _fname) {
  std::ofstream f(_fname, std::ios::out | std::ios::trunc | std::ios::binary);
  f << _tree;
  f.flush();
  f.close();
}

TEST_CASE("FermdConfig", "[fermd]") {
  FermdConfigTestSM sm;
  auto cfg = sm.get<aegir::fermd::FermdConfig>();
  cfg->autosave(false);

  TempFileGuard fg;

  //  std::cout << "saving config " << fg.getFilename() << std::endl;
  cfg->save(fg.getFilename());
  ryml::Tree tree;
  readFile(fg.getFilename(), tree);
  ryml::NodeRef root = tree.rootref();

  // verify a few values
  REQUIRE(root["loglevel"].val() == "info");

  // modify the config
  root["loglevel"] = "error";

  // change something and read it back
  writeYAML(tree, fg.getFilename());
  //std::cout << "YAML:" << std::endl << tree << "---" << std::endl;

  try {
    cfg->load(fg.getFilename());
  }
  catch (aegir::Exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
    throw e;
  }
  // FermdConfig values
  REQUIRE(cfg->getLogLevel() == blt::severity_level::error);
}
