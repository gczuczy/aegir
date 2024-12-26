
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

TEST_CASE("TiltAdjustNone", "[fermd][tilts][adjust]") {
	aegir::fermd::DB::tilthydrometer th;
	float sg = GENERATE(take(5, random(1.000f, 1.120f)));
	REQUIRE_THAT(th.adjust(sg), Catch::Matchers::WithinAbs(sg, 0.0001));
}

TEST_CASE("TiltAdjustNull", "[fermd][tilts][adjust]") {
	aegir::fermd::DB::tilthydrometer th;
	float offset = GENERATE(take(5, random(-0.005f, 0.010f)));
	float measured = GENERATE(take(5, random(1.000f, 1.120f)));
	float expected = measured - offset;

	th.setZero(1.000f+offset);
	float adjusted = th.adjust(measured);

	REQUIRE_THAT(adjusted,
							 WithinAbs(expected, 0.0001f)
							 || WithinRel(expected, 0.0005f) );
}

TEST_CASE("TiltAdjustHigh", "[fermd][tilts][adjust]") {
	aegir::fermd::DB::tilthydrometer th;
	float offset = GENERATE(take(5, random(-0.005f, 0.010f)));
	float measured = GENERATE(take(5, random(1.000f, 1.120f)));
	float expected = measured - offset;
	INFO("offset "<<offset);
	INFO("measured "<<measured);
	INFO("expected "<<expected);

	th.setHigh(measured, expected);
	float adjusted = th.adjust(measured);
	INFO("adjusted "<<adjusted);

	REQUIRE_THAT(adjusted,
							 WithinAbs(expected, 0.0001f)
							 || WithinRel(expected, 0.0005f) );
}

TEST_CASE("TiltAdjustFull", "[fermd][tilts][adjust]") {
	aegir::fermd::DB::tilthydrometer th;
	float nulloffset = GENERATE(take(5, random(-0.005f, 0.010f)));
	float highoffset = GENERATE(take(5, random(-0.005f, 0.010f)));
	float measurement = GENERATE(take(5, random(1.040f, 1.120f)));
	INFO("nulloffset " << nulloffset);
	INFO("highoffset " << highoffset);
	INFO("measurement " << measurement);

	float highexpected = measurement - highoffset;
	INFO("highexpected " << highexpected);

	float nullmeasurement = 1.000f + nulloffset;
	INFO("nullmeasurement " << nullmeasurement);

	// sanity checks
#if 0
	CHECK(measurement > nullmeasurement);
	CHECK(highexpected > 1.030f);
#endif

	th.setZero(nullmeasurement).setHigh(measurement, highexpected);

	// test null
	float nulladjusted = th.adjust(nullmeasurement);
	INFO("nulladjusted " << nulladjusted);
	REQUIRE_THAT(1.000f,
							 WithinAbs(nulladjusted, 0.002f)
							 || WithinRel(nulladjusted, 0.002f) );
	// test high
	float highadjusted = th.adjust(measurement);
	INFO("highadjusted " << highadjusted);
	REQUIRE_THAT(highadjusted,
							 WithinAbs(highexpected, 0.0001f)
							 || WithinRel(highexpected, 0.0005f) );
}
