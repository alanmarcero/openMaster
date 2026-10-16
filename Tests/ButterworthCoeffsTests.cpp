#include "ButterworthCoeffs.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>

using namespace analog_master::core;

TEST_CASE("Slope to order mapping is correct", "[butterworth]") {
    REQUIRE(slopeToOrder(FilterSlope::Slope12dB) == 2);
    REQUIRE(slopeToOrder(FilterSlope::Slope24dB) == 4);
    REQUIRE(slopeToOrder(FilterSlope::Slope48dB) == 8);
    REQUIRE(slopeToOrder(FilterSlope::Slope64dB) == 12);
    REQUIRE(slopeToOrder(FilterSlope::Slope96dB) == 16);
}

TEST_CASE("Butterworth design produces correct number of biquad sections", "[butterworth]") {
    const double fs = 48000.0;
    auto lpSections = designButterworthLowPass(fs, 1000.0, 16);
    REQUIRE(lpSections.size() == 8);

    auto hpSections = designButterworthHighPass(fs, 1000.0, 12);
    REQUIRE(hpSections.size() == 6);
}

TEST_CASE("Butterworth Low-Pass frequency response", "[butterworth]") {
    const double fs = 48000.0;
    const double fc = 1000.0;
    const int order = 4; // 24 dB/oct
    auto sections = designButterworthLowPass(fs, fc, order);

    // Compute frequency response at DC (omega = 0)
    // H(z) at z=1: sum(b) / (1 + sum(a))
    double totalGainAtDC = 1.0;
    for (const auto& s : sections) {
        double num = s.b0 + s.b1 + s.b2;
        double den = 1.0 + s.a1 + s.a2;
        totalGainAtDC *= (num / den);
    }
    REQUIRE(std::abs(totalGainAtDC - 1.0) < 1e-4);

    // Compute frequency response at Nyquist (z = -1)
    double totalGainAtNyquist = 1.0;
    for (const auto& s : sections) {
        double num = s.b0 - s.b1 + s.b2;
        double den = 1.0 - s.a1 + s.a2;
        totalGainAtNyquist *= (num / den);
    }
    // Lowpass at Nyquist should be virtually zero
    REQUIRE(std::abs(totalGainAtNyquist) < 1e-4);
}

TEST_CASE("Butterworth High-Pass frequency response", "[butterworth]") {
    const double fs = 48000.0;
    const double fc = 100.0;
    const int order = 4; // 24 dB/oct
    auto sections = designButterworthHighPass(fs, fc, order);

    // Compute frequency response at DC (z = 1)
    double totalGainAtDC = 1.0;
    for (const auto& s : sections) {
        double num = s.b0 + s.b1 + s.b2;
        double den = 1.0 + s.a1 + s.a2;
        totalGainAtDC *= (num / den);
    }
    // Highpass at DC should be completely blocked
    REQUIRE(std::abs(totalGainAtDC) < 1e-4);
}
