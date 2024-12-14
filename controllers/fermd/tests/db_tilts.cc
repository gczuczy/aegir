
#include <stdio.h>
#include <stdlib.h>

#include <limits>
#include <random>

#include "fermd/tests/dbbase.hh"

#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>

using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinULP;

TEST_CASE_METHOD(DBFixture, "db_tilts", "[fermd][db][tilts]") {
  auto txn = db()->txn();
  auto tilts = txn->getTilthydrometers();
  REQUIRE(tilts.size() == 8);

  auto& cth = *tilts.begin();
  aegir::fermd::DB::tilthydrometer th = *cth;
  th.enabled = true;
  txn.setTilthydrometer(th);

  // now verify
  auto th2 = txn->getTilthydrometerByUUID(th.uuid);
  REQUIRE(th2->enabled == th.enabled);
}

static float randrange(float _range) {
	return _range*(1.0f*arc4random())/static_cast<float>(std::numeric_limits<uint32_t>::max());

}

TEST_CASE("TiltAdjustNone", "[fermd][db][tilts][adjust]") {
	aegir::fermd::DB::tilthydrometer th;
	float sg = GENERATE(take(5, random(1.000f, 1.120f)));
	REQUIRE_THAT(th.adjust(sg), Catch::Matchers::WithinAbs(sg, 0.0001));
}

TEST_CASE("TiltAdjustNull", "[fermd][db][tilts][adjust]") {
	aegir::fermd::DB::tilthydrometer th;
	float offset = GENERATE(take(5, random(0.000f, 0.020f)));
	float measured = GENERATE(take(5, random(1.000f, 1.120f)));
	float expected = measured + offset;

	th.setZero(1.000f+offset);
	float adjusted = th.adjust(measured);

	printf("measured:%.4f(%.4f) offset:%0.4f adjusted:%.4f\n", measured, adjusted,
				 offset, adjusted);
	REQUIRE_THAT(adjusted,
							 WithinAbs(expected, 0.01f)
							 || WithinRel(expected, 0.005f) );
}
