#include "StereoFieldShaper.h"
#include <algorithm>
#include <cmath>

namespace analog_master::core {

void StereoFieldShaper::prepare(double sampleRate) noexcept {
    if (sampleRate <= 0.0) {
        return;
    }
    sampleRate_ = sampleRate;
    currentCrossoverHz_ = crossoverHz_;
    crossoverL_.prepare(sampleRate);
    crossoverR_.prepare(sampleRate);
    crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
    crossoverR_.setCrossoverFrequency(currentCrossoverHz_);
    currentBypassGain_ = bypassed_ ? 0.0f : 1.0f;
    currentSoloGain_ = soloLow_ ? 1.0f : 0.0f;
    hasProcessedAudio_ = false;
    reset();
}

void StereoFieldShaper::reset() noexcept {
    crossoverL_.reset();
    crossoverR_.reset();
    currentCrossoverHz_ = crossoverHz_;
    crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
    crossoverR_.setCrossoverFrequency(currentCrossoverHz_);
    currentWidth_ = targetWidth_;
    currentBypassGain_ = bypassed_ ? 0.0f : 1.0f;
    currentSoloGain_ = soloLow_ ? 1.0f : 0.0f;
    hasProcessedAudio_ = false;
}

void StereoFieldShaper::setCrossoverFrequency(double freqHz) noexcept {
    const double clamped = std::clamp(freqHz, 20.0, 500.0);
    if (std::abs(crossoverHz_ - clamped) < 0.001) {
        return;
    }
    crossoverHz_ = clamped;
    if (!hasProcessedAudio_) {
        currentCrossoverHz_ = crossoverHz_;
        crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
        crossoverR_.setCrossoverFrequency(currentCrossoverHz_);
    }
}

void StereoFieldShaper::setWidth(float width) noexcept {
    targetWidth_ = std::clamp(width, 0.0f, 2.0f);
    if (!hasProcessedAudio_) {
        currentWidth_ = targetWidth_;
    }
}

void StereoFieldShaper::setBypassed(bool bypassed) noexcept {
    bypassed_ = bypassed;
    if (!hasProcessedAudio_) {
        currentBypassGain_ = bypassed_ ? 0.0f : 1.0f;
    }
}

void StereoFieldShaper::setSoloLow(bool solo) noexcept {
    soloLow_ = solo;
    if (!hasProcessedAudio_) {
        currentSoloGain_ = soloLow_ ? 1.0f : 0.0f;
    }
}

void StereoFieldShaper::processSample(float inL, float inR, float& outL, float& outR) noexcept {
    hasProcessedAudio_ = true;
    if (bypassed_) {
        outL = inL;
        outR = inR;
        return;
    }

    if (std::abs(currentCrossoverHz_ - crossoverHz_) > 1e-3) {
        currentCrossoverHz_ = crossoverHz_;
        crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
        crossoverR_.setCrossoverFrequency(currentCrossoverHz_);
    }

    const auto splitL = crossoverL_.process(inL);
    const auto splitR = crossoverR_.process(inR);

    // Mid/Side matrix of low frequencies (< crossover)
    const float midLow = 0.5f * (splitL.low + splitR.low);
    const float sideLow = 0.5f * (splitL.low - splitR.low);

    const float shapedSideLow = sideLow * targetWidth_;

    const float shapedLowL = midLow + shapedSideLow;
    const float shapedLowR = midLow - shapedSideLow;

    if (soloLow_) {
        outL = shapedLowL;
        outR = shapedLowR;
        return;
    }

    // Recombine with untouched high frequencies (> crossover)
    outL = shapedLowL + splitL.high;
    outR = shapedLowR + splitR.high;
}

void StereoFieldShaper::processBlock(float* leftChannel, float* rightChannel, int numSamples) noexcept {
    if (leftChannel == nullptr || numSamples <= 0) {
        return;
    }
    hasProcessedAudio_ = true;

    const float targetBypassGain = bypassed_ ? 0.0f : 1.0f;
    if (bypassed_ && currentBypassGain_ <= 1e-5f) {
        currentBypassGain_ = 0.0f;
        currentWidth_ = targetWidth_;
        currentCrossoverHz_ = crossoverHz_;
        return;
    }

    const float startWidth = currentWidth_;
    const float endWidth = targetWidth_;
    const float widthStep = (endWidth - startWidth) / static_cast<float>(numSamples);

    const float targetSoloGain = soloLow_ ? 1.0f : 0.0f;
    const float soloStep = (targetSoloGain - currentSoloGain_) / static_cast<float>(numSamples);
    const float bypassStep = (targetBypassGain - currentBypassGain_) / static_cast<float>(numSamples);

    if (rightChannel == nullptr) {
        // Mono channel processing
        if ( std::abs(currentCrossoverHz_ - crossoverHz_) > 1e-3 ) {
            currentCrossoverHz_ = crossoverHz_;
            crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
        }
        for (int i = 0; i < numSamples; ++i) {
            currentSoloGain_ += soloStep;
            currentBypassGain_ += bypassStep;
            const float dry = leftChannel[i];
            const auto split = crossoverL_.process(dry);
            const float wet = split.low + (1.0f - currentSoloGain_) * split.high;
            leftChannel[i] = dry + currentBypassGain_ * (wet - dry);
        }
        currentWidth_ = endWidth;
        currentSoloGain_ = targetSoloGain;
        currentBypassGain_ = targetBypassGain;
        return;
    }

    // Stereo channel processing with sub-block crossover smoothing & click-free width/solo/bypass interpolation
    constexpr int kSubBlockSize = 16;
    const double smoothingCoeff = 1.0 - std::exp(-static_cast<double>(kSubBlockSize) / (0.006 * sampleRate_));

    int sampleIdx = 0;
    while (sampleIdx < numSamples) {
        const int chunk = std::min(kSubBlockSize, numSamples - sampleIdx);

        if (std::abs(currentCrossoverHz_ - crossoverHz_) > 1e-3) {
            const double logCurr = std::log(std::max(20.0, currentCrossoverHz_));
            const double logTarget = std::log(std::max(20.0, crossoverHz_));
            currentCrossoverHz_ = std::exp(logCurr + smoothingCoeff * (logTarget - logCurr));
            if (std::abs(currentCrossoverHz_ - crossoverHz_) < 0.02) {
                currentCrossoverHz_ = crossoverHz_;
            }
            crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
            crossoverR_.setCrossoverFrequency(currentCrossoverHz_);
        }

        for (int i = sampleIdx; i < sampleIdx + chunk; ++i) {
            currentWidth_ += widthStep;
            currentSoloGain_ += soloStep;
            currentBypassGain_ += bypassStep;

            const float dryL = leftChannel[i];
            const float dryR = rightChannel[i];

            const auto splitL = crossoverL_.process(dryL);
            const auto splitR = crossoverR_.process(dryR);

            const float midLow = 0.5f * (splitL.low + splitR.low);
            const float sideLow = 0.5f * (splitL.low - splitR.low);

            const float shapedSideLow = sideLow * currentWidth_;

            const float shapedLowL = midLow + shapedSideLow;
            const float shapedLowR = midLow - shapedSideLow;

            const float highMix = 1.0f - currentSoloGain_;
            const float wetL = shapedLowL + highMix * splitL.high;
            const float wetR = shapedLowR + highMix * splitR.high;

            leftChannel[i] = dryL + currentBypassGain_ * (wetL - dryL);
            rightChannel[i] = dryR + currentBypassGain_ * (wetR - dryR);
        }

        sampleIdx += chunk;
    }

    currentWidth_ = endWidth;
    currentSoloGain_ = targetSoloGain;
    currentBypassGain_ = targetBypassGain;
}

} // namespace analog_master::core
