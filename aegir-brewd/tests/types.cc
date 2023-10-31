/*
  types for types-related functions
 */


#include "types.hh"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ThermoCouple", "[types]") {
  REQUIRE(aegir::ThermoCouple((uint8_t)0) == aegir::ThermoCouple::MT);
  REQUIRE(aegir::ThermoCouple((uint8_t)1) == aegir::ThermoCouple::HERMS);
  REQUIRE(aegir::ThermoCouple((uint8_t)2) == aegir::ThermoCouple::BK);
  REQUIRE(aegir::ThermoCouple((uint8_t)3) == aegir::ThermoCouple::HLT);

  REQUIRE(aegir::ThermoCouple("MT") == aegir::ThermoCouple::MT);
  REQUIRE(aegir::ThermoCouple("MashTun") == aegir::ThermoCouple::MT);
  REQUIRE(aegir::ThermoCouple("HERMS") == aegir::ThermoCouple::HERMS);
  REQUIRE(aegir::ThermoCouple("HERMS") == aegir::ThermoCouple::RIMS);
  REQUIRE(aegir::ThermoCouple("RIMS") == aegir::ThermoCouple::HERMS);
  REQUIRE(aegir::ThermoCouple("RIMS") == aegir::ThermoCouple::RIMS);
  REQUIRE(aegir::ThermoCouple("BK") == aegir::ThermoCouple::BK);
  REQUIRE(aegir::ThermoCouple("BoilKettle") == aegir::ThermoCouple::BK);
  REQUIRE(aegir::ThermoCouple("HLT") == aegir::ThermoCouple::HLT);
  REQUIRE(aegir::ThermoCouple("HotLiquorTank") == aegir::ThermoCouple::HLT);

  REQUIRE(aegir::ThermoCouple(std::string("MT")) == aegir::ThermoCouple::MT);
  REQUIRE(aegir::ThermoCouple(std::string("MashTun")) == aegir::ThermoCouple::MT);
  REQUIRE(aegir::ThermoCouple(std::string("HERMS")) == aegir::ThermoCouple::HERMS);
  REQUIRE(aegir::ThermoCouple(std::string("HERMS")) == aegir::ThermoCouple::RIMS);
  REQUIRE(aegir::ThermoCouple(std::string("RIMS")) == aegir::ThermoCouple::HERMS);
  REQUIRE(aegir::ThermoCouple(std::string("RIMS")) == aegir::ThermoCouple::RIMS);
  REQUIRE(aegir::ThermoCouple(std::string("BK")) == aegir::ThermoCouple::BK);
  REQUIRE(aegir::ThermoCouple(std::string("BoilKettle")) == aegir::ThermoCouple::BK);
  REQUIRE(aegir::ThermoCouple(std::string("HotLiquorTank")) == aegir::ThermoCouple::HLT);
  REQUIRE(aegir::ThermoCouple(std::string("HotLiquorTank")) == aegir::ThermoCouple::HLT);

  REQUIRE(aegir::ThermoCouple("HERMS") != aegir::ThermoCouple::HLT);

  REQUIRE_THROWS(aegir::ThermoCouple("foobar"));
  REQUIRE_THROWS(aegir::ThermoCouple("_SIZE"));
}

TEST_CASE("ThermoCouple::toStr", "[types]") {
  REQUIRE(std::strcmp(aegir::ThermoCouple(aegir::ThermoCouple::MT).toStr(),
			      "MashTun") == 0);
  REQUIRE(std::strcmp(aegir::ThermoCouple(aegir::ThermoCouple::HERMS).toStr(),
			      "HERMS") == 0);
  REQUIRE(std::strcmp(aegir::ThermoCouple(aegir::ThermoCouple::BK).toStr(),
			      "BoilKettle") == 0);
  REQUIRE(std::strcmp(aegir::ThermoCouple(aegir::ThermoCouple::HLT).toStr(),
			      "HotLiquorTank") == 0);

  REQUIRE_THROWS(aegir::ThermoCouple(aegir::ThermoCouple::_SIZE).toStr());
}
