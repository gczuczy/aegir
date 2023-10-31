/*
  Message formats
 */

#include "Message.hh"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("TSDB::ThermoReadingMessage", "[Message]") {
  aegir::ThermoReadings in, out;

  for (int i=0; i<aegir::ThermoCouple::_SIZE; ++i)
    in[i] = 0.5f * (1+i);

  auto srcmsg = aegir::ThermoReadingMessage(in, 42);
  auto dstmsg = aegir::ThermoReadingMessage(srcmsg.serialize());

  out = dstmsg.getTemps();
  for (int i=0; i<aegir::ThermoCouple::_SIZE; ++i) {
    REQUIRE(out[i] == Catch::Approx(in[i]));
  }
}
