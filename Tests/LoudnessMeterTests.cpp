#include "LoudnessMeter.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <complex>
#include <numbers>
using namespace analog_master::core;
using Catch::Matchers::WithinAbs;
namespace {
void tone(LoudnessMeter &m, double sr, double seconds, double hz, double db, int channels = 2,
          float polarity = 1) {
    const double a = std::pow(10., db / 20.);
    for (int i = 0; i < static_cast<int>(seconds * sr); ++i) {
        const float x = static_cast<float>(a * std::sin(2 * std::numbers::pi * hz * i / sr));
        m.process(x, channels == 2 ? polarity * x : 0.f);
    }
}
// ITU BS.1770-5 Annex 1 Tables 1 and 2, independent published 48 kHz coefficients.
double reference(double hz) {
    const auto z = std::polar(1., -2 * std::numbers::pi * hz / 48000.);
    const auto shelf = (1.53512485958697 - 2.69169618940638 * z + 1.19839281085285 * z * z) /
                       (1. - 1.69065929318241 * z + .73248077421585 * z * z);
    const auto hp = (1. - 2. * z + z * z) / (1. - 1.99004745483398 * z + .99007225036621 * z * z);
    return -.691 + 20 * std::log10(std::abs(shelf * hp));
}
} // namespace
TEST_CASE("LUFS matches published K weighting across frequency", "[loudness]") {
    for (double hz : {31.5, 63., 125., 1000., 4000., 10000.}) {
        LoudnessMeter m;
        m.prepare(48000, 2);
        tone(m, 48000, 4., hz, -23);
        CAPTURE(hz);
        REQUIRE_THAT(m.shortTermLufs(), WithinAbs(-23 + reference(hz), .06));
        REQUIRE_THAT(m.integratedLufs(), WithinAbs(-23 + reference(hz), .06));
    }
}
TEST_CASE("LUFS counts channels without phase cancellation", "[loudness]") {
    for (double sr : {44100., 48000., 96000., 192000.}) {
        LoudnessMeter mono, stereo, anti;
        mono.prepare(sr, 1);
        stereo.prepare(sr, 2);
        anti.prepare(sr, 2);
        tone(mono, sr, 3.2, 997, -23, 1);
        tone(stereo, sr, 3.2, 997, -23);
        tone(anti, sr, 3.2, 997, -23, 2, -1);
        REQUIRE_THAT(mono.integratedLufs(), WithinAbs(-26.01, .1));
        REQUIRE_THAT(stereo.integratedLufs() - mono.integratedLufs(), WithinAbs(3.0103, .001));
        REQUIRE_THAT(stereo.integratedLufs(), WithinAbs(anti.integratedLufs(), .0001));
    }
}
TEST_CASE("LUFS warmup reset silence and absolute gate", "[loudness]") {
    LoudnessMeter m;
    m.prepare(48000, 2);
    tone(m, 48000, .39, 1000, -23);
    REQUIRE_FALSE(m.integratedReady());
    REQUIRE_FALSE(m.shortTermReady());
    tone(m, 48000, .01, 1000, -23);
    REQUIRE(m.integratedReady());
    REQUIRE_FALSE(m.shortTermReady());
    tone(m, 48000, 3., 1000, -23);
    REQUIRE(m.shortTermReady());
    m.reset();
    for (int i = 0; i < 144000; ++i)
        m.process(0, 0);
    REQUIRE(m.shortTermReady());
    REQUIRE(std::isinf(m.shortTermLufs()));
    REQUIRE(std::isinf(m.integratedLufs()));
    tone(m, 48000, 4., 1000, -80);
    REQUIRE(std::isinf(m.integratedLufs()));
    m.process(NAN, INFINITY);
    tone(m, 48000, 4., 1000, -23);
    REQUIRE_THAT(m.shortTermLufs(), WithinAbs(-23., .1));
}
TEST_CASE("LUFS relative gate excludes quiet programme", "[loudness]") {
    LoudnessMeter m;
    m.prepare(48000, 2);
    tone(m, 48000, 20., 1000, -40);
    tone(m, 48000, 20., 1000, -23);
    tone(m, 48000, 20., 1000, -40);
    REQUIRE_THAT(m.integratedLufs(), WithinAbs(-23., .15));
}
TEST_CASE("Integrated gating resolves close energy populations", "[loudness]") {
    // Exact-history reference: libebur128 67b33abe, mode I (not histogram).
    // A stepped three-tone programme exposes coarse relative-gate bins at 13.2 s.
    LoudnessMeter meter;
    meter.prepare(44100, 1);
    for (int block = 0; block < 132; ++block) {
        const double level = block < 30 ? -85. : -55. + static_cast<double>((block / 10 * 17) % 42);
        for (int i = 0; i < 4410; ++i) {
            const double t = (block * 4410 + i) / 44100.;
            const double a = std::pow(10., level / 20.);
            const float x = static_cast<float>(a * (.6 * std::sin(2 * std::numbers::pi * 997 * t) +
                                                    .2 * std::sin(2 * std::numbers::pi * 53 * t) +
                                                    .2 * std::sin(2 * std::numbers::pi * 8000 * t)));
            meter.process(x, 0);
        }
    }
    REQUIRE_THAT(meter.integratedLufs(), WithinAbs(-29.507931587, .005));
}
