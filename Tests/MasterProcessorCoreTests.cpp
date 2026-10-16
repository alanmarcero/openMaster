#include "MasterProcessorCore.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <numbers>
#include <vector>

using namespace analog_master::core;
using Catch::Matchers::WithinAbs;

TEST_CASE("MasterProcessorCore defaults match mastering specifications", "[master]") {
    MasterProcessorCore processor;
    // Low cut: 20 Hz, slope 64 dB/oct
    REQUIRE(processor.lowCutFrequency() == 20.0);
    REQUIRE(processor.lowCutSlope() == FilterSlope::Slope64dB);
    REQUIRE_FALSE(processor.isLowCutBypassed());

    // High cut: 20000 Hz, slope 64 dB/oct
    REQUIRE(processor.highCutFrequency() == 20000.0);
    REQUIRE(processor.highCutSlope() == FilterSlope::Slope64dB);
    REQUIRE_FALSE(processor.isHighCutBypassed());

    // Low-end stereo: 120 Hz crossover, width 0.0 (Mono)
    REQUIRE(processor.stereoFieldFrequency() == 120.0);
    REQUIRE(processor.stereoFieldWidth() == 0.0f);
    REQUIRE_FALSE(processor.isStereoFieldBypassed());
}

TEST_CASE("MasterProcessorCore cuts sub-audible DC rumble", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutFrequency(20.0);
    processor.setLowCutSlope(FilterSlope::Slope64dB);

    // Feed 5 Hz sub-rumble tone for multiple full cycles
    const int numSamples = 24000; // 0.5s, 2.5 complete cycles of 5 Hz
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float s = static_cast<float>(std::sin(2.0 * std::numbers::pi * 5.0 * i / fs));
        left[i] = s;
        right[i] = s;
    }

    processor.processBlock(left.data(), right.data(), numSamples);

    // Measure after filter settling
    float peakRemaining = 0.0f;
    for (int i = 14400; i < numSamples; ++i) {
        peakRemaining = std::max(peakRemaining, std::abs(left[i]));
    }

    // Sub-audible 5 Hz (2 octaves below 20Hz at 64 dB/oct) should be heavily attenuated
    REQUIRE(peakRemaining < 0.01f);
}

TEST_CASE("MasterProcessorCore cuts ultrasonic noise above 20kHz", "[master]") {
    const double fs = 96000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setHighCutFrequency(20000.0);
    processor.setHighCutSlope(FilterSlope::Slope64dB);

    // Feed 35 kHz ultrasonic tone
    const int numSamples = 4800;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float s = static_cast<float>(std::sin(2.0 * std::numbers::pi * 35000.0 * i / fs));
        left[i] = s;
        right[i] = s;
    }

    processor.processBlock(left.data(), right.data(), numSamples);

    float peakRemaining = 0.0f;
    for (int i = 2400; i < numSamples; ++i) {
        peakRemaining = std::max(peakRemaining, std::abs(left[i]));
    }

    REQUIRE(peakRemaining < 0.02f);
}

TEST_CASE("MasterProcessorCore bypass passes audio untouched", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setMasterBypassed(true);

    const int numSamples = 64;
    std::vector<float> left(numSamples, 0.75f);
    std::vector<float> right(numSamples, -0.42f);

    processor.processBlock(left.data(), right.data(), numSamples);

    for (int i = 0; i < numSamples; ++i) {
        REQUIRE_THAT(left[i], WithinAbs(0.75f, 1e-6));
        REQUIRE_THAT(right[i], WithinAbs(-0.42f, 1e-6));
    }
}

TEST_CASE("MasterProcessorCore handles mono channels seamlessly", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);

    const int numSamples = 128;
    std::vector<float> mono(numSamples, 0.5f);

    // Pass nullptr for rightChannel to simulate mono track
    processor.processBlock(mono.data(), nullptr, numSamples);

    for (int i = 0; i < numSamples; ++i) {
        REQUIRE(std::isfinite(mono[i]));
    }

    auto levels = processor.meterLevels();
    REQUIRE(levels.peakL > 0.0f);
}

TEST_CASE("MasterProcessorCore smooths gain changes without step clicks", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setHighCutBypassed(true);
    processor.setStereoFieldBypassed(true);
    processor.setMasterGainDb(0.0f);

    const int numSamples = 64;
    std::vector<float> left(numSamples, 1.0f);
    std::vector<float> right(numSamples, 1.0f);

    // Initial block at 0 dB (unity gain)
    processor.processBlock(left.data(), right.data(), numSamples);

    // Abrupt step to +6 dB (gain = 2.0)
    processor.setMasterGainDb(6.0206f);

    std::vector<float> leftRamp(numSamples, 1.0f);
    std::vector<float> rightRamp(numSamples, 1.0f);
    processor.processBlock(leftRamp.data(), rightRamp.data(), numSamples);

    // First sample of the ramp should start near 1.0, not instantaneously jump to 2.0
    REQUIRE(leftRamp[0] < 1.15f);
    // Last sample should reach ~2.0
    REQUIRE_THAT(leftRamp[numSamples - 1], WithinAbs(2.0f, 0.05f));
}

TEST_CASE("MasterProcessorCore tracks peak transients with exponential decay", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setHighCutBypassed(true);

    // Impulse transient
    std::vector<float> impulse(64, 0.0f);
    impulse[0] = 1.0f;
    std::vector<float> rightImpulse(64, 0.0f);
    rightImpulse[0] = 0.8f;

    processor.processBlock(impulse.data(), rightImpulse.data(), 64);

    auto levels = processor.meterLevels();
    REQUIRE(levels.peakL > 0.9f);
    REQUIRE(levels.peakR > 0.7f);

    // Now feed silence and observe exponential decay
    std::vector<float> silence(4800, 0.0f); // 100 ms
    processor.processBlock(silence.data(), silence.data(), 4800);

    auto decayedLevels = processor.meterLevels();
    REQUIRE(decayedLevels.peakL < levels.peakL);
    REQUIRE(decayedLevels.peakL > 0.0f);
}

TEST_CASE("BiquadFilter gracefully handles NaN and non-finite inputs", "[biquad]") {
    BiquadFilter filter;
    float nanVal = std::numeric_limits<float>::quiet_NaN();
    float out = filter.process(nanVal);

    // Should flush to zero without exploding
    REQUIRE(out == 0.0f);
    REQUIRE(filter.process(0.5f) != 0.0f);
}

