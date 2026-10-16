#include "MasterProcessorCore.h"
#include <algorithm>
#include <cmath>

namespace analog_master::core {

MasterProcessorCore::MasterProcessorCore() {
    // Defaults matching user requirements:
    // Low cut: nominal 20 Hz, steep cutoff (-64 dB/oct)
    lowCut_.setCutoff(20.0);
    lowCut_.setSlope(FilterSlope::Slope64dB);

    // High cut: nominal 20 kHz, steep cutoff (-64 dB/oct)
    highCut_.setCutoff(20000.0);
    highCut_.setSlope(FilterSlope::Slope64dB);

    // Stereo field: nominal 120 Hz crossover, width 0.0 (Mono low-end)
    stereoShaper_.setCrossoverFrequency(120.0);
    stereoShaper_.setWidth(0.0f);
}

void MasterProcessorCore::prepare(double sampleRate) noexcept {
    if (sampleRate <= 0.0) {
        return;
    }
    sampleRate_ = sampleRate;
    lowCut_.prepare(sampleRate);
    stereoShaper_.prepare(sampleRate);
    highCut_.prepare(sampleRate);
    reset();
}

void MasterProcessorCore::reset() noexcept {
    lowCut_.reset();
    stereoShaper_.reset();
    highCut_.reset();
    meterPeakL_.store(0.0f, std::memory_order_relaxed);
    meterPeakR_.store(0.0f, std::memory_order_relaxed);
}

void MasterProcessorCore::setLowCutFrequency(double freqHz) noexcept {
    lowCut_.setCutoff(freqHz);
}

void MasterProcessorCore::setLowCutSlope(FilterSlope slope) noexcept {
    lowCut_.setSlope(slope);
}

void MasterProcessorCore::setLowCutBypassed(bool bypassed) noexcept {
    lowCut_.setBypassed(bypassed);
}

void MasterProcessorCore::setHighCutFrequency(double freqHz) noexcept {
    highCut_.setCutoff(freqHz);
}

void MasterProcessorCore::setHighCutSlope(FilterSlope slope) noexcept {
    highCut_.setSlope(slope);
}

void MasterProcessorCore::setHighCutBypassed(bool bypassed) noexcept {
    highCut_.setBypassed(bypassed);
}

void MasterProcessorCore::setStereoFieldFrequency(double freqHz) noexcept {
    stereoShaper_.setCrossoverFrequency(freqHz);
}

void MasterProcessorCore::setStereoFieldWidth(float width) noexcept {
    stereoShaper_.setWidth(width);
}

void MasterProcessorCore::setStereoFieldBypassed(bool bypassed) noexcept {
    stereoShaper_.setBypassed(bypassed);
}

void MasterProcessorCore::setStereoFieldSoloLow(bool solo) noexcept {
    stereoShaper_.setSoloLow(solo);
}

void MasterProcessorCore::setMasterGainDb(float gainDb) noexcept {
    masterGainDb_ = std::clamp(gainDb, -24.0f, 24.0f);
    masterGainLinear_ = std::pow(10.0f, masterGainDb_ / 20.0f);
}

void MasterProcessorCore::setMasterBypassed(bool bypassed) noexcept {
    masterBypassed_ = bypassed;
}

void MasterProcessorCore::processBlock(float* leftChannel, float* rightChannel, int numSamples) noexcept {
    if (leftChannel == nullptr || numSamples <= 0) {
        return;
    }

    float blockPeakL = 0.0f;
    float blockPeakR = 0.0f;

    if (masterBypassed_) {
        for (int i = 0; i < numSamples; ++i) {
            blockPeakL = std::max(blockPeakL, std::abs(leftChannel[i]));
            if (rightChannel != nullptr) {
                blockPeakR = std::max(blockPeakR, std::abs(rightChannel[i]));
            }
        }
        meterPeakL_.store(blockPeakL, std::memory_order_relaxed);
        meterPeakR_.store(blockPeakR, std::memory_order_relaxed);
        return;
    }

    float* channels[2] = { leftChannel, rightChannel ? rightChannel : leftChannel };

    // 1. Ultra-steep Low Cut EQ
    lowCut_.processBlock(channels, rightChannel ? 2 : 1, numSamples);

    // 2. Low-End Stereo Field Shaper
    if (rightChannel != nullptr) {
        stereoShaper_.processBlock(leftChannel, rightChannel, numSamples);
    }

    // 3. Ultra-steep High Cut EQ
    highCut_.processBlock(channels, rightChannel ? 2 : 1, numSamples);

    // 4. Master Gain & Peak Tracking
    for (int i = 0; i < numSamples; ++i) {
        leftChannel[i] *= masterGainLinear_;
        blockPeakL = std::max(blockPeakL, std::abs(leftChannel[i]));

        if (rightChannel != nullptr) {
            rightChannel[i] *= masterGainLinear_;
            blockPeakR = std::max(blockPeakR, std::abs(rightChannel[i]));
        }
    }

    meterPeakL_.store(blockPeakL, std::memory_order_relaxed);
    meterPeakR_.store(blockPeakR, std::memory_order_relaxed);
}

} // namespace analog_master::core
