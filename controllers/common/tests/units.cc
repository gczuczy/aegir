
#include <stdio.h>
#include <stdlib.h>

#include <limits>

#include "common/units.hh"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinULP;

#define SAMPLES 4

// brix,sg
static float testdata[SAMPLES][2] =
	{
		{0.0f, 1.000f},
		{10.0f, 1.040f},
		{11.0f, 1.044f},
		{22.0f, 1.092f}
	};

TEST_CASE("sg2brix", "[common][units]") {
	for (int i=0; i<SAMPLES; ++i) {
		REQUIRE_THAT(aegir::sg2brix(testdata[i][1]),
								 WithinAbs(testdata[i][0], 0.01f)
								 || WithinRel(testdata[i][0], 0.005f) );
	}
}

TEST_CASE("brix2sg", "[common][units]") {
	for (int i=0; i<SAMPLES; ++i) {
		REQUIRE_THAT(aegir::brix2sg(testdata[i][0]),
								 WithinAbs(testdata[i][1], 0.001f)
								 || WithinRel(testdata[i][1], 0.0005f) );
	}
}

TEST_CASE("RoundTripConversion", "[common][units]") {
	for (int i=0; i<32; ++i) {
		float brix = (1.0f*arc4random())/static_cast<float>(std::numeric_limits<uint32_t>::max());
		float rebrix = aegir::sg2brix(aegir::brix2sg(brix));
		REQUIRE_THAT(brix,
								 WithinAbs(rebrix, 0.01f)
								 || WithinRel(rebrix, 0.005f) );
	}
}
