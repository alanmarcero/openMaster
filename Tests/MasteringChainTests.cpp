#include "MasterProcessorCore.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <numbers>
#include <vector>
using namespace analog_master::core;
using Catch::Matchers::WithinAbs;
namespace {
void neutral(MasterProcessorCore &p) {
    p.setLowCutBypassed(true);
    p.setHighCutBypassed(true);
    p.setStereoFieldBypassed(true);
}
std::vector<float> render(MasterProcessorCore &p, int count, int block, float amplitude = .1f) {
    std::vector<float> l(static_cast<size_t>(count)), r(l.size());
    for (int i = 0; i < count; ++i)
        l[static_cast<size_t>(i)] =
            amplitude * static_cast<float>(std::sin(2 * std::numbers::pi * 997 * i / 48000));
    r = l;
    for (int i = 0; i < count; i += block)
        p.processBlock(l.data() + i, r.data() + i, std::min(block, count - i));
    return l;
}
} // namespace
TEST_CASE("New stages off preserve samples through fixed latency", "[chain]") {
    MasterProcessorCore p;
    neutral(p);
    p.prepare(48000);
    const auto audio = render(p, 4096, 127);
    REQUIRE(p.latencySamples() > 0);
    for (int i = 0; i < static_cast<int>(audio.size()); ++i) {
        const float expected = i < p.latencySamples()
                                   ? 0.f
                                   : .1f * static_cast<float>(std::sin(2 * std::numbers::pi * 997 *
                                                                       (i - p.latencySamples()) / 48000));
        REQUIRE_THAT(audio[static_cast<size_t>(i)], WithinAbs(expected, 1e-6));
    }
}
TEST_CASE("Chain processing is invariant to host partition with settled settings", "[chain]") {
    MasterProcessorCore a, b;
    neutral(a);
    neutral(b);
    CompressorSettings c;
    c.enabled = true;
    c.thresholdDb = -24;
    a.setCompressor(c);
    b.setCompressor(c);
    a.prepare(48000);
    b.prepare(48000);
    const auto x = render(a, 12000, 17), y = render(b, 12000, 4096);
    for (size_t i = 0; i < x.size(); ++i)
        REQUIRE_THAT(x[i], WithinAbs(y[i], 1e-7));
}
TEST_CASE("LUFS matched bypass attenuates louder branch and measures emitted audio", "[chain]") {
    MasterProcessorCore p;
    neutral(p);
    p.setMasterGainDb(6);
    p.setMatchBypass(true);
    p.prepare(48000);
    render(p, 48000 * 7, 512);
    auto levels = p.meterLevels();
    REQUIRE(levels.matchReady);
    REQUIRE_THAT(levels.matchDb, WithinAbs(-6., .15));
    REQUIRE_THAT(levels.shortTermLufs, WithinAbs(-20., .2));
    p.setMasterBypassed(true);
    render(p, 48000 * 4, 127);
    levels = p.meterLevels();
    REQUIRE_THAT(levels.shortTermLufs, WithinAbs(-20., .1));
    p.requestMeterReset();
    float zero = 0;
    p.processBlock(&zero, nullptr, 1);
    REQUIRE(std::isinf(p.meterLevels().integratedLufs));
}
TEST_CASE("Host bypass uses the same aligned dry path", "[chain]") {
    MasterProcessorCore p;
    p.setHostBypassed(true);
    p.prepare(48000);
    const auto x = render(p, 4096, 64);
    for (int i = p.latencySamples(); i < 4096; ++i)
        REQUIRE_THAT(
            x[static_cast<size_t>(i)],
            WithinAbs(.1 * std::sin(2 * std::numbers::pi * 997 * (i - p.latencySamples()) / 48000), 1e-6));
}
TEST_CASE("Meter reset excludes true peaks emitted before the new measurement", "[chain]") {
    MasterProcessorCore p;
    neutral(p);
    p.prepare(48000);
    const int frames = p.latencySamples() + 1;
    std::vector<float> left(static_cast<size_t>(frames)), right(left.size());
    left.front() = right.front() = 1.f;
    p.processBlock(left.data(), right.data(), frames);
    REQUIRE(left.back() == 1.f);
    REQUIRE(right.back() == 1.f);
    REQUIRE(std::isfinite(p.meterLevels().truePeakDbtp));

    p.requestMeterReset();
    left.assign(256, 0.f);
    right.assign(256, 0.f);
    p.processBlock(left.data(), right.data(), 256);
    for (size_t i = 0; i < left.size(); ++i) {
        REQUIRE(left[i] == 0.f);
        REQUIRE(right[i] == 0.f);
    }
    REQUIRE(p.meterLevels().truePeakDbtp == -INFINITY);
}
TEST_CASE("Nonfinite input does not poison full chain loudness or filters", "[chain]") {
    MasterProcessorCore p;
    p.prepare(48000);
    float l = NAN, r = INFINITY;
    p.processBlock(&l, &r, 1);
    auto x = render(p, 4096, 64);
    for (const float v : x)
        REQUIRE(std::isfinite(v));
}
