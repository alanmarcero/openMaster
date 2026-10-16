#include "StereoFieldShaper.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <numbers>
#include <vector>

using namespace analog_master::core;
using Catch::Matchers::WithinAbs;

TEST_CASE("StereoFieldShaper collapses low frequencies to mono when width is 0", "[stereo]") {
    const double fs = 48000.0;
    StereoFieldShaper shaper;
    shaper.prepare(fs);
    shaper.setCrossoverFrequency(200.0);
    shaper.setWidth(0.0f); // 100% Mono for low end

    // Generate a 50 Hz out-of-phase low frequency tone: L = sin, R = -sin (pure side component)
    const double freq = 50.0;
    const int numSamples = 2000;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float val = static_cast<float>(std::sin(2.0 * std::numbers::pi * freq * i / fs));
        left[i] = val;
        right[i] = -val;
    }

    shaper.processBlock(left.data(), right.data(), numSamples);

    // After settling, since width is 0 and input had pure side (mid = 0),
    // the output low end should be suppressed to near zero.
    float residualEnergy = 0.0f;
    for (int i = 1000; i < numSamples; ++i) {
        residualEnergy += std::abs(left[i] - right[i]);
    }
    float avgDiff = residualEnergy / 1000.0f;

    // Difference between L and R in the low band should be essentially zero (mono)
    REQUIRE(avgDiff < 0.05f);
}

TEST_CASE("StereoFieldShaper preserves high frequencies above crossover", "[stereo]") {
    const double fs = 48000.0;
    StereoFieldShaper shaper;
    shaper.prepare(fs);
    shaper.setCrossoverFrequency(100.0);
    shaper.setWidth(0.0f); // Low-end mono

    // Generate high frequency 5000 Hz tone: L = 1, R = 0 (high frequency stereo difference)
    const double freq = 5000.0;
    const int numSamples = 2000;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float val = static_cast<float>(std::sin(2.0 * std::numbers::pi * freq * i / fs));
        left[i] = val;
        right[i] = 0.0f;
    }

    shaper.processBlock(left.data(), right.data(), numSamples);

    // At 5000 Hz (way above 100 Hz), left should still have full signal, right should remain near 0
    float leftPeak = 0.0f;
    float rightPeak = 0.0f;
    for (int i = 1500; i < numSamples; ++i) {
        leftPeak = std::max(leftPeak, std::abs(left[i]));
        rightPeak = std::max(rightPeak, std::abs(right[i]));
    }

    REQUIRE(leftPeak > 0.9f);
    REQUIRE(rightPeak < 0.05f);
}

TEST_CASE("StereoFieldShaper passthrough when width is 1.0", "[stereo]") {
    const double fs = 48000.0;
    StereoFieldShaper shaper;
    shaper.prepare(fs);
    shaper.setCrossoverFrequency(150.0);
    shaper.setWidth(1.0f); // Unity width

    // Crossover LR4 sums to flat allpass unity magnitude
    const double freq = 150.0;
    const int numSamples = 2000;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        left[i] = static_cast<float>(std::sin(2.0 * std::numbers::pi * freq * i / fs));
        right[i] = left[i];
    }

    shaper.processBlock(left.data(), right.data(), numSamples);

    // Output amplitude should remain 1.0
    float peak = 0.0f;
    for (int i = 1500; i < numSamples; ++i) {
        peak = std::max(peak, std::abs(left[i]));
    }
    REQUIRE_THAT(peak, WithinAbs(1.0f, 0.02f));
}

TEST_CASE("StereoFieldShaper enhances stereo width when width is 2.0", "[stereo]") {
    const double fs = 48000.0;
    StereoFieldShaper shaper;
    shaper.prepare(fs);
    shaper.setCrossoverFrequency(200.0);
    shaper.setWidth(2.0f); // 200% Enhanced stereo

    const double freq = 60.0;
    const int numSamples = 2000;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float val = static_cast<float>(std::sin(2.0 * std::numbers::pi * freq * i / fs));
        left[i] = val * 0.5f;
        right[i] = -val * 0.5f; // Pure side signal with amplitude 0.5
    }

    shaper.processBlock(left.data(), right.data(), numSamples);

    // With width 2.0, side component is doubled: amplitude should reach ~1.0
    float peak = 0.0f;
    for (int i = 1000; i < numSamples; ++i) {
        peak = std::max(peak, std::abs(left[i]));
    }
    REQUIRE(peak > 0.90f);
}

TEST_CASE("StereoFieldShaper solo low band mutes high frequencies", "[stereo]") {
    const double fs = 48000.0;
    StereoFieldShaper shaper;
    shaper.prepare(fs);
    shaper.setCrossoverFrequency(150.0);
    shaper.setSoloLow(true);

    const int numSamples = 2000;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    // 5000 Hz high frequency tone
    for (int i = 0; i < numSamples; ++i) {
        float val = static_cast<float>(std::sin(2.0 * std::numbers::pi * 5000.0 * i / fs));
        left[i] = val;
        right[i] = val;
    }

    shaper.processBlock(left.data(), right.data(), numSamples);

    // Under Solo Low, high frequencies must be heavily attenuated (> 40 dB cut)
    float highPeak = 0.0f;
    for (int i = 1000; i < numSamples; ++i) {
        highPeak = std::max(highPeak, std::abs(left[i]));
    }
    REQUIRE(highPeak < 0.01f);
}

TEST_CASE("StereoFieldShaper expands mono/center-panned low frequencies when width > 1.0 while remaining 100% mono-compatible", "[stereo]") {
    const double fs = 48000.0;
    StereoFieldShaper shaper;
    shaper.prepare(fs);
    shaper.setCrossoverFrequency(150.0);
    shaper.setWidth(2.0f); // 200% Enhanced stereo

    const double freq = 60.0;
    const int numSamples = 4000;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    // Pure mono / center-panned 60 Hz sub-bass (L == R)
    for (int i = 0; i < numSamples; ++i) {
        float val = static_cast<float>(std::sin(2.0 * std::numbers::pi * freq * i / fs));
        left[i] = val;
        right[i] = val;
    }

    shaper.processBlock(left.data(), right.data(), numSamples);

    float maxStereoDiff = 0.0f;
    float maxMonoPeak = 0.0f;
    for (int i = 2000; i < numSamples; ++i) {
        maxStereoDiff = std::max(maxStereoDiff, std::abs(left[i] - right[i]));
        maxMonoPeak = std::max(maxMonoPeak, std::abs(0.5f * (left[i] + right[i])));
    }

    // Must synthesize substantial stereo width (L != R) even when the low-end input was 100% mono
    REQUIRE(maxStereoDiff > 0.7f);
    // Summing L+R back to mono must cancel the synthesized side component with zero comb filtering
    REQUIRE_THAT(maxMonoPeak, WithinAbs(1.0f, 0.02f));
}
