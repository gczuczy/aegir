/*
  Config tests
 */

#include "Config.hh"

#include <catch2/catch_test_macros.hpp>

#define CFG_TEST_FILE "tests/data/aegir-brewd.yaml"


TEST_CASE("Config", "[Config]") {
  auto cfg = aegir::Config::getInstance();

  {
    auto cfg2 = aegir::Config::getInstance();
    REQUIRE(cfg == cfg2);
  }

  cfg->load(CFG_TEST_FILE);
  INFO("Config loaded");

  REQUIRE(cfg->getSPIDevice() == "/dev/spigen0.0");

  aegir::pinlayout_t pinlayout;
  cfg->getPinConfig(pinlayout);
  REQUIRE(pinlayout["bkpump"] == 24);
  REQUIRE(pinlayout["cs0"] == 7);
  REQUIRE(pinlayout["mtpump"] == 23);

  REQUIRE(cfg->getHeatOverhead() == 2.5f);

  aegir::Config::tcids tcs;
  tcs = cfg->getThermocouples();
  REQUIRE(tcs.tcs[aegir::ThermoCouple::MT] == 1);
  REQUIRE(tcs.tcs[aegir::ThermoCouple::BK] == 3);
  REQUIRE(tcs.tcs[aegir::ThermoCouple::HLT] == 2);
  REQUIRE(tcs.tcs[aegir::ThermoCouple::HERMS] == 0);
}
