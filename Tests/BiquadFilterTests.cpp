#include "BiquadFilter.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace analog_master::core;
using Catch::Matchers::WithinAbs;

TEST_CASE("BiquadFilter initializes with pass-through coefficients", "[biquad]") {
    BiquadFilter filter;
    REQUIRE_THAT(filter.process(1.0f), WithinAbs(1.0f, 1e-6));
    REQUIRE_THAT(filter.process(0.5f), WithinAbs(0.5f, 1e-6));
}

TEST_CASE("BiquadFilter reset clears state registers", "[biquad]") {
    BiquadFilter filter;
    // Set simple lowpass-like smoothing coefficients
    filter.setCoefficients({ 0.1f, 0.2f, 0.1f, -0.8f, 0.2f });
    
    // Pump non-zero samples
    for (int i = 0; i < 50; ++i) {
        (void)filter.process(1.0f);
    }

    filter.reset();
    // After reset, input 0 should produce 0 output
    REQUIRE_THAT(filter.process(0.0f), WithinAbs(0.0f, 1e-6));
}
