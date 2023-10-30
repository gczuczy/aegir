/*
  types for types-related functions
 */


#include "types.hh"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ThermoCouple", "[types]") {
  REQUIRE_NOTHROW(aegir::ThermoCouple("MT") == aegir::ThermoCouple::MT);
  REQUIRE_NOTHROW(aegir::ThermoCouple("HERMS") == aegir::ThermoCouple::HERMS);
  REQUIRE_NOTHROW(aegir::ThermoCouple("HERMS") == aegir::ThermoCouple::RIMS);
  REQUIRE_NOTHROW(aegir::ThermoCouple("RIMS") == aegir::ThermoCouple::HERMS);
  REQUIRE_NOTHROW(aegir::ThermoCouple("RIMS") == aegir::ThermoCouple::RIMS);
  REQUIRE_NOTHROW(aegir::ThermoCouple("BK") == aegir::ThermoCouple::BK);
  REQUIRE_NOTHROW(aegir::ThermoCouple("HLT") == aegir::ThermoCouple::HLT);

  REQUIRE_NOTHROW(aegir::ThermoCouple("HERMS") != aegir::ThermoCouple::HLT);

  REQUIRE_THROWS(aegir::ThermoCouple("foobar"));
  REQUIRE_THROWS(aegir::ThermoCouple("_SIZE"));
}

TEST_CASE("ThermoCouple::toStr", "[types]") {
  REQUIRE_NOTHROW(std::strcmp(aegir::ThermoCouple(aegir::ThermoCouple::MT).toStr(),
			      "MT") == 0);
  REQUIRE_NOTHROW(std::strcmp(aegir::ThermoCouple(aegir::ThermoCouple::HERMS).toStr(),
			      "HERMS") == 0);
  REQUIRE_NOTHROW(std::strcmp(aegir::ThermoCouple(aegir::ThermoCouple::BK).toStr(),
			      "BK") == 0);
  REQUIRE_NOTHROW(std::strcmp(aegir::ThermoCouple(aegir::ThermoCouple::HLT).toStr(),
			      "HLT") == 0);

  REQUIRE_THROWS(aegir::ThermoCouple(aegir::ThermoCouple::_SIZE).toStr());
}
