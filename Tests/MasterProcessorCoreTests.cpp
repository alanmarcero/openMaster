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
    // Low cut: 20 Hz, slope 72 dB/oct
    REQUIRE(processor.lowCutFrequency() == 20.0);
    REQUIRE(processor.lowCutSlope() == FilterSlope::Slope72dB);
    REQUIRE_FALSE(processor.isLowCutBypassed());

    // High cut: 20000 Hz, slope 72 dB/oct
    REQUIRE(processor.highCutFrequency() == 20000.0);
    REQUIRE(processor.highCutSlope() == FilterSlope::Slope72dB);
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
    processor.setLowCutSlope(FilterSlope::Slope72dB);

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

    // Sub-audible 5 Hz (2 octaves below 20Hz at 72 dB/oct) should be heavily attenuated
    REQUIRE(peakRemaining < 0.01f);
}

TEST_CASE("MasterProcessorCore cuts ultrasonic noise above 20kHz", "[master]") {
    const double fs = 96000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setHighCutFrequency(20000.0);
    processor.setHighCutSlope(FilterSlope::Slope72dB);

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

TEST_CASE("MasterProcessorCore bypass passes delayed audio untouched", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setMasterBypassed(true);

    const int numSamples = processor.latencySamples() + 64;
    std::vector<float> left(numSamples, 0.75f);
    std::vector<float> right(numSamples, -0.42f);

    processor.processBlock(left.data(), right.data(), numSamples);

    for (int i = 0; i < numSamples; ++i) {
        REQUIRE_THAT(left[i], WithinAbs(i < processor.latencySamples() ? 0.f : 0.75f, 1e-6));
        REQUIRE_THAT(right[i], WithinAbs(i < processor.latencySamples() ? 0.f : -0.42f, 1e-6));
    }
}

TEST_CASE("MasterProcessorCore handles mono channels seamlessly", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);

    const int numSamples = processor.latencySamples() + 128;
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

    const int numSamples = processor.latencySamples() + 2400;
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
    // The 5 ms smoother settles over time, independently of the host block length.
    REQUIRE_THAT(leftRamp[numSamples - 1], WithinAbs(2.0f, 0.05f));
}

TEST_CASE("MasterProcessorCore tracks peak transients with exponential decay", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setHighCutBypassed(true);

    // Impulse transient
    std::vector<float> impulse(processor.latencySamples() + 64, 0.0f);
    impulse[0] = 1.0f;
    std::vector<float> rightImpulse(processor.latencySamples() + 64, 0.0f);
    rightImpulse[0] = 0.8f;

    processor.processBlock(impulse.data(), rightImpulse.data(), static_cast<int>(impulse.size()));

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

TEST_CASE("MasterProcessorCore slope changes during playback remain stable", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);

    const int numSamples = 256;
    std::vector<float> left(numSamples, 0.4f);
    std::vector<float> right(numSamples, 0.4f);

    const FilterSlope slopes[] = {
        FilterSlope::Slope12dB,
        FilterSlope::Slope96dB,
        FilterSlope::Slope24dB,
        FilterSlope::Slope72dB,
        FilterSlope::Slope48dB
    };

    for (auto s : slopes) {
        processor.setLowCutSlope(s);
        processor.setHighCutSlope(s);
        processor.processBlock(left.data(), right.data(), numSamples);

        for (int i = 0; i < numSamples; ++i) {
            REQUIRE(std::isfinite(left[i]));
            REQUIRE(std::isfinite(right[i]));
        }
    }
}

TEST_CASE("MasterProcessorCore detects overload clip when signal reaches 0 dBFS", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setHighCutBypassed(true);
    processor.setStereoFieldBypassed(true);

    // Signal at 0.5 (safe, no clip)
    std::vector<float> safeBlock(processor.latencySamples() + 128, 0.5f);
    processor.processBlock(safeBlock.data(), safeBlock.data(), processor.latencySamples() + 128);
    auto safeLevels = processor.meterLevels();
    REQUIRE_FALSE(safeLevels.clipL);

    // Overload pulse at 1.05 (> 0 dBFS clip)
    std::vector<float> clipBlock(processor.latencySamples() + 128, 1.05f);
    processor.processBlock(clipBlock.data(), clipBlock.data(), processor.latencySamples() + 128);
    auto clipLevels = processor.meterLevels();
    REQUIRE(clipLevels.clipL);
    REQUIRE(clipLevels.clipR);
}

TEST_CASE("MasterProcessorCore computes steady IEC VU levels", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setHighCutBypassed(true);
    processor.setStereoFieldBypassed(true);

    // Feed 1 kHz sine wave for 1.5 s (5x tau for 99.3% steady-state VU integration)
    const int numSamples = 72000;
    std::vector<float> sineL(numSamples);
    std::vector<float> sineR(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float val = static_cast<float>(std::sin(2.0 * std::numbers::pi * 1000.0 * i / fs));
        sineL[i] = val;
        sineR[i] = val;
    }

    processor.processBlock(sineL.data(), sineR.data(), numSamples);

    auto levels = processor.meterLevels();
    // A VU reads a sine's RMS
    REQUIRE_THAT(levels.vuL, WithinAbs(std::numbers::sqrt2 / 2.0, 0.005));
    REQUIRE_THAT(levels.vuR, WithinAbs(std::numbers::sqrt2 / 2.0, 0.005));
}

TEST_CASE("MasterProcessorCore High Cut remains spike-free during abrupt 16-pole cutoff modulation", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setStereoFieldBypassed(true);
    processor.setHighCutSlope(FilterSlope::Slope96dB); // 16-pole brickwall (max Q = 5.10)
    processor.setHighCutFrequency(8000.0);

    const int numSamples = 1024;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    // Pump 1 kHz sine wave with amplitude 0.5 at 8 kHz cutoff
    for (int i = 0; i < numSamples; ++i) {
        float s = 0.5f * static_cast<float>(std::sin(2.0 * std::numbers::pi * 1000.0 * i / fs));
        left[i] = s;
        right[i] = s;
    }
    processor.processBlock(left.data(), right.data(), numSamples);

    // Abruptly jump cutoff from 8 kHz to 24 kHz while states are active
    processor.setHighCutFrequency(24000.0);
    for (int i = 0; i < numSamples; ++i) {
        float s = 0.5f * static_cast<float>(std::sin(2.0 * std::numbers::pi * 1000.0 * (i + numSamples) / fs));
        left[i] = s;
        right[i] = s;
    }
    processor.processBlock(left.data(), right.data(), numSamples);

    float maxPeak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        maxPeak = std::max(maxPeak, std::abs(left[i]));
    }

    // TPT SVF must prevent the >10x (+20 dB) transient explosion that occurs in Direct Form II biquads
    REQUIRE(maxPeak < 0.65f);
}

TEST_CASE("MasterProcessorCore tracks stereo phase correlation accurately", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setHighCutBypassed(true);
    processor.setStereoFieldBypassed(true);

    const int numSamples = 4096;
    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    // 1. In-phase mono stereo signal (L == R) -> correlation approaches +1.0
    for (int i = 0; i < numSamples; ++i) {
        const float s = 0.5f * static_cast<float>(std::sin(2.0 * std::numbers::pi * 440.0 * i / fs));
        left[i] = s;
        right[i] = s;
    }
    const auto inPhase = left;
    for (int b = 0; b < 8; ++b) {
        left = inPhase;
        right = inPhase;
        processor.processBlock(left.data(), right.data(), numSamples);
    }
    REQUIRE(processor.meterLevels().correlation > 0.95f);

    // 2. Out-of-phase stereo signal (L == -R) -> correlation approaches -1.0
    for (int i = 0; i < numSamples; ++i) {
        const float s = 0.5f * static_cast<float>(std::sin(2.0 * std::numbers::pi * 440.0 * i / fs));
        left[i] = s;
        right[i] = -s;
    }
    const auto outOfPhase = right;
    for (int b = 0; b < 12; ++b) {
        // Supply fresh input: reusing rendered buffers recirculates delayed old-phase audio.
        left = inPhase;
        right = outOfPhase;
        processor.processBlock(left.data(), right.data(), numSamples);
    }
    REQUIRE(processor.meterLevels().correlation < -0.90f);
}


TEST_CASE("MasterProcessorCore crossfades master bypass without a step click", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setStereoFieldWidth(1.0f);

    const int blockSize = 512;
    const int numBlocks = 12;
    std::vector<float> left(blockSize * numBlocks);
    std::vector<float> right(blockSize * numBlocks);
    for (size_t i = 0; i < left.size(); ++i) {
        left[i] = 0.5f * static_cast<float>(std::sin(2.0 * std::numbers::pi * 100.0 * static_cast<double>(i) / fs));
        right[i] = left[i];
    }
    const std::vector<float> dry = left;

    for (int b = 0; b < numBlocks; ++b) {
        processor.setMasterBypassed(b >= 4 && b < 8);
        processor.processBlock(left.data() + b * blockSize, right.data() + b * blockSize, blockSize);
    }

    // The clean sine changes at most 0.0066 per sample.
    float maxStep = 0.0f;
    for (int i = blockSize; i < blockSize * numBlocks; ++i) {
        maxStep = std::max(maxStep, std::abs(left[i] - left[i - 1]));
    }
    REQUIRE(maxStep < 0.01f);

    // After the fade-out, the bypassed samples are dry.
    for (int i = 5 * blockSize; i < 8 * blockSize; ++i) {
        REQUIRE(left[i] == dry[static_cast<size_t>(i - processor.latencySamples())]);
    }
}

TEST_CASE("MasterProcessorCore VU reaches 99 percent of a step within 300 ms", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setHighCutBypassed(true);
    processor.setStereoFieldBypassed(true);

    auto runSine = [&](int numSamples) {
        std::vector<float> left(numSamples);
        std::vector<float> right(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            left[i] = static_cast<float>(std::sin(2.0 * std::numbers::pi * 1000.0 * i / fs));
            right[i] = left[i];
        }
        processor.processBlock(left.data(), right.data(), numSamples);
        return processor.meterLevels().vuL;
    };

    // 250 ms: below 99%. 300 ms: at 99%.
    constexpr float kSineRms = std::numbers::sqrt2_v<float> / 2.0f;
    REQUIRE(runSine(12000) < 0.99f * kSineRms);
    REQUIRE(runSine(2400) >= 0.985f * kSineRms);
}

TEST_CASE("MasterProcessorCore holds a clip until the meter reads it", "[master]") {
    const double fs = 48000.0;
    MasterProcessorCore processor;
    processor.prepare(fs);
    processor.setLowCutBypassed(true);
    processor.setHighCutBypassed(true);
    processor.setStereoFieldBypassed(true);

    std::vector<float> clipBlock(processor.latencySamples() + 128, 1.05f);
    processor.processBlock(clipBlock.data(), clipBlock.data(), processor.latencySamples() + 128);
    std::vector<float> safeBlock(processor.latencySamples() + 128, 0.5f);
    processor.processBlock(safeBlock.data(), safeBlock.data(), processor.latencySamples() + 128);

    const auto firstRead = processor.takeMeterLevels();
    const auto secondRead = processor.takeMeterLevels();

    REQUIRE(firstRead.clipL);
    REQUIRE(firstRead.clipR);
    REQUIRE_FALSE(secondRead.clipL);
    REQUIRE_FALSE(secondRead.clipR);
}

TEST_CASE("MasterProcessorCore correlation speed does not depend on the block size", "[master]") {
    const double fs = 48000.0;
    const auto correlationAfterPhaseFlip = [fs](int blockSize) {
        MasterProcessorCore processor;
        processor.prepare(fs);
        processor.setLowCutBypassed(true);
        processor.setHighCutBypassed(true);
        processor.setStereoFieldBypassed(true);
        const auto run = [&](float rightSign) {
            std::vector<float> left(4096);
            std::vector<float> right(4096);
            for (int i = 0; i < 4096; ++i) {
                left[i] = 0.5f * static_cast<float>(std::sin(2.0 * std::numbers::pi * 440.0 * i / fs));
                right[i] = rightSign * left[i];
            }
            for (int offset = 0; offset < 4096; offset += blockSize) {
                processor.processBlock(left.data() + offset, right.data() + offset, blockSize);
            }
        };
        run(1.0f);
        run(1.0f);
        run(-1.0f);
        return processor.meterLevels().correlation;
    };

    REQUIRE_THAT(correlationAfterPhaseFlip(64), WithinAbs(correlationAfterPhaseFlip(4096), 0.02));
}
