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
    currentGainLinear_ = targetGainLinear_;
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
    targetGainLinear_ = std::pow(10.0f, masterGainDb_ / 20.0f);
}

void MasterProcessorCore::setMasterBypassed(bool bypassed) noexcept {
    masterBypassed_ = bypassed;
}

void MasterProcessorCore::processBlock(float* leftChannel, float* rightChannel, int numSamples) noexcept {
    if (leftChannel == nullptr || numSamples <= 0) {
        return;
    }

    float currentPeakL = meterPeakL_.load(std::memory_order_relaxed);
    float currentPeakR = meterPeakR_.load(std::memory_order_relaxed);

    if (masterBypassed_) {
        // Transparent bypass: track dry levels
        for (int i = 0; i < numSamples; ++i) {
            float absL = std::abs(leftChannel[i]);
            if (absL > currentPeakL) {
                currentPeakL = absL;
            }
            if (rightChannel != nullptr) {
                float absR = std::abs(rightChannel[i]);
                if (absR > currentPeakR) {
                    currentPeakR = absR;
                }
            }
        }

        // 300ms VU meter exponential decay
        const float decay = std::exp(-static_cast<float>(numSamples) / (0.3f * static_cast<float>(sampleRate_)));
        currentPeakL *= decay;
        currentPeakR *= decay;

        meterPeakL_.store(currentPeakL, std::memory_order_relaxed);
        meterPeakR_.store(currentPeakR, std::memory_order_relaxed);
        return;
    }

    float* channels[2] = { leftChannel, rightChannel ? rightChannel : leftChannel };
    const int numChannels = (rightChannel != nullptr) ? 2 : 1;

    // 1. Ultra-steep Low Cut EQ
    lowCut_.processBlock(channels, numChannels, numSamples);

    // 2. Low-End Stereo Field Shaper
    stereoShaper_.processBlock(leftChannel, rightChannel, numSamples);

    // 3. Ultra-steep High Cut EQ
    highCut_.processBlock(channels, numChannels, numSamples);

    // 4. Master Gain with anti-zipper linear smoothing & VU ballistic tracking
    const float startGain = currentGainLinear_;
    const float endGain = targetGainLinear_;
    const float gainStep = (endGain - startGain) / static_cast<float>(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        currentGainLinear_ += gainStep;

        leftChannel[i] *= currentGainLinear_;
        float absL = std::abs(leftChannel[i]);
        if (absL > currentPeakL) {
            currentPeakL = absL;
        }

        if (rightChannel != nullptr) {
            rightChannel[i] *= currentGainLinear_;
            float absR = std::abs(rightChannel[i]);
            if (absR > currentPeakR) {
                currentPeakR = absR;
            }
        }
    }

    currentGainLinear_ = endGain;

    // 300ms analog VU integration ballistics decay
    const float decay = std::exp(-static_cast<float>(numSamples) / (0.3f * static_cast<float>(sampleRate_)));
    currentPeakL *= decay;
    currentPeakR *= decay;

    meterPeakL_.store(currentPeakL, std::memory_order_relaxed);
    meterPeakR_.store(currentPeakR, std::memory_order_relaxed);
}

} // namespace analog_master::core
