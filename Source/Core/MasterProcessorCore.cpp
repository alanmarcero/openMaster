#include "MasterProcessorCore.h"
#include <algorithm>
#include <cmath>

namespace analog_master::core {

MasterProcessorCore::MasterProcessorCore() {
    // Defaults matching user requirements:
    // Low cut: nominal 20 Hz, steep cutoff (-72 dB/oct)
    lowCut_.setCutoff(20.0);
    lowCut_.setSlope(FilterSlope::Slope72dB);

    // High cut: nominal 20 kHz, steep cutoff (-72 dB/oct)
    highCut_.setCutoff(20000.0);
    highCut_.setSlope(FilterSlope::Slope72dB);

    // Stereo field: nominal 120 Hz crossover, width 0.0 (Mono low-end)
    stereoShaper_.setCrossoverFrequency(120.0);
    stereoShaper_.setWidth(0.0f);
}

void MasterProcessorCore::prepare(double sampleRate) noexcept {
    if (sampleRate <= 0.0) {
        return;
    }
    sampleRate_ = sampleRate;
    currentMasterBypassGain_ = masterBypassed_ ? 0.0f : 1.0f;
    hasProcessedAudio_ = false;
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
    currentMasterBypassGain_ = masterBypassed_ ? 0.0f : 1.0f;
    hasProcessedAudio_ = false;
    meterPeakL_.store(0.0f, std::memory_order_relaxed);
    meterPeakR_.store(0.0f, std::memory_order_relaxed);
    meterVuL_.store(0.0f, std::memory_order_relaxed);
    meterVuR_.store(0.0f, std::memory_order_relaxed);
    meterClipL_.store(false, std::memory_order_relaxed);
    meterClipR_.store(false, std::memory_order_relaxed);
    meterCorrelation_.store(1.0f, std::memory_order_relaxed);
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
    const float clamped = std::clamp(gainDb, -24.0f, 24.0f);
    if (std::abs(masterGainDb_ - clamped) < 1e-4f) {
        return;
    }
    masterGainDb_ = clamped;
    targetGainLinear_ = std::pow(10.0f, masterGainDb_ / 20.0f);
    if (!hasProcessedAudio_) {
        currentGainLinear_ = targetGainLinear_;
    }
}

void MasterProcessorCore::setMasterBypassed(bool bypassed) noexcept {
    masterBypassed_ = bypassed;
    if (!hasProcessedAudio_) {
        currentMasterBypassGain_ = masterBypassed_ ? 0.0f : 1.0f;
    }
}

void MasterProcessorCore::processBlock(float* leftChannel, float* rightChannel, int numSamples) noexcept {
    if (leftChannel == nullptr || numSamples <= 0) {
        return;
    }
    hasProcessedAudio_ = true;

    // A power change crossfades over the block head. On power off, only the head goes through the chain.
    const float targetBypassGain = masterBypassed_ ? 0.0f : 1.0f;
    const bool fading = currentMasterBypassGain_ != targetBypassGain;
    const int fadeSamples = fading ? std::min(numSamples, kBypassFadeSamples) : 0;
    const int wetSamples = masterBypassed_ ? fadeSamples : numSamples;

    std::copy_n(leftChannel, fadeSamples, dryL_.begin());
    if (rightChannel != nullptr) {
        std::copy_n(rightChannel, fadeSamples, dryR_.begin());
    }

    if (wetSamples > 0) {
        float* channels[2] = { leftChannel, rightChannel ? rightChannel : leftChannel };
        const int numChannels = (rightChannel != nullptr) ? 2 : 1;

        // 1. Ultra-steep Low Cut EQ
        lowCut_.processBlock(channels, numChannels, wetSamples);

        // 2. Low-End Stereo Field Shaper
        stereoShaper_.processBlock(leftChannel, rightChannel, wetSamples);

        // 3. Ultra-steep High Cut EQ
        highCut_.processBlock(channels, numChannels, wetSamples);

        // 4. Master Gain with anti-zipper linear smoothing
        const float gainStep = (targetGainLinear_ - currentGainLinear_) / static_cast<float>(wetSamples);
        for (int i = 0; i < wetSamples; ++i) {
            currentGainLinear_ += gainStep;
            leftChannel[i] *= currentGainLinear_;
            if (rightChannel != nullptr) {
                rightChannel[i] *= currentGainLinear_;
            }
        }
        currentGainLinear_ = targetGainLinear_;
    }

    if (fading) {
        const float fadeStep = (targetBypassGain - currentMasterBypassGain_) / static_cast<float>(fadeSamples);
        for (int i = 0; i < fadeSamples; ++i) {
            currentMasterBypassGain_ += fadeStep;
            leftChannel[i] = dryL_[i] + currentMasterBypassGain_ * (leftChannel[i] - dryL_[i]);
            if (rightChannel != nullptr) {
                rightChannel[i] = dryR_[i] + currentMasterBypassGain_ * (rightChannel[i] - dryR_[i]);
            }
        }
        currentMasterBypassGain_ = targetBypassGain;
    }

    float currentPeakL = meterPeakL_.load(std::memory_order_relaxed);
    float currentPeakR = meterPeakR_.load(std::memory_order_relaxed);
    float currentVuL = meterVuL_.load(std::memory_order_relaxed);
    float currentVuR = meterVuR_.load(std::memory_order_relaxed);
    float currentCorr = meterCorrelation_.load(std::memory_order_relaxed);

    float sumAbsL = 0.0f;
    float sumAbsR = 0.0f;
    float blockPeakL = 0.0f;
    float blockPeakR = 0.0f;
    double sumLL = 0.0;
    double sumRR = 0.0;
    double sumLR = 0.0;

    for (int i = 0; i < numSamples; ++i) {
        const float l = leftChannel[i];
        float absL = std::abs(l);
        sumAbsL += absL;
        blockPeakL = std::max(blockPeakL, absL);
        if (absL > currentPeakL) {
            currentPeakL = absL;
        }

        if (rightChannel != nullptr) {
            const float r = rightChannel[i];
            float absR = std::abs(r);
            sumAbsR += absR;
            blockPeakR = std::max(blockPeakR, absR);
            if (absR > currentPeakR) {
                currentPeakR = absR;
            }
            sumLL += static_cast<double>(l) * static_cast<double>(l);
            sumRR += static_cast<double>(r) * static_cast<double>(r);
            sumLR += static_cast<double>(l) * static_cast<double>(r);
        }
    }

    const float peakDecay = std::exp(-static_cast<float>(numSamples) / (0.3f * static_cast<float>(sampleRate_)));
    currentPeakL *= peakDecay;
    currentPeakR *= peakDecay;

    // IEC 60268-17 VU: 99% rise in 300 ms.
    const float vuTau = 0.3f / std::log(100.0f);
    const float vuAlpha = 1.0f - std::exp(-static_cast<float>(numSamples) / (vuTau * static_cast<float>(sampleRate_)));
    const float meanL = sumAbsL / static_cast<float>(numSamples);
    const float meanR = sumAbsR / static_cast<float>(numSamples);
    currentVuL += vuAlpha * (meanL * 1.5708f - currentVuL);
    currentVuR += vuAlpha * (meanR * 1.5708f - currentVuR);

    if (rightChannel != nullptr) {
        const double denom = std::sqrt(sumLL * sumRR);
        if (denom > 1e-9) {
            const float instCorr = std::clamp(static_cast<float>(sumLR / denom), -1.0f, 1.0f);
            currentCorr += std::max(0.25f, vuAlpha) * (instCorr - currentCorr);
        }
    } else {
        currentCorr = 1.0f;
    }

    meterPeakL_.store(currentPeakL, std::memory_order_relaxed);
    meterPeakR_.store(currentPeakR, std::memory_order_relaxed);
    meterVuL_.store(currentVuL, std::memory_order_relaxed);
    meterVuR_.store(currentVuR, std::memory_order_relaxed);
    meterClipL_.store(blockPeakL >= 0.999f, std::memory_order_relaxed);
    meterClipR_.store(blockPeakR >= 0.999f, std::memory_order_relaxed);
    meterCorrelation_.store(currentCorr, std::memory_order_relaxed);
}

} // namespace analog_master::core
