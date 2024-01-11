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

#include "ryml.hh"

#include <catch2/catch_test_macros.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"

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
  auto cfg = aegir::fermd::FermdConfig::getInstance();

  TempFileGuard fg;

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

  cfg->load(fg.getFilename());
  REQUIRE(cfg->getLogLevel() == blt::severity_level::error);
}
