#include "StereoFieldShaper.h"
#include <algorithm>

namespace analog_master::core {

void StereoFieldShaper::prepare(double sampleRate) noexcept {
    if (sampleRate <= 0.0) {
        return;
    }
    crossoverL_.prepare(sampleRate);
    crossoverR_.prepare(sampleRate);
    currentBypassGain_ = bypassed_ ? 0.0f : 1.0f;
    currentSoloGain_ = soloLow_ ? 1.0f : 0.0f;
    reset();
}

void StereoFieldShaper::reset() noexcept {
    crossoverL_.reset();
    crossoverR_.reset();
    currentWidth_ = targetWidth_;
}

void StereoFieldShaper::setCrossoverFrequency(double freqHz) noexcept {
    if (std::abs(crossoverHz_ - freqHz) < 0.001) {
        return;
    }
    crossoverHz_ = freqHz;
    crossoverL_.setCrossoverFrequency(freqHz);
    crossoverR_.setCrossoverFrequency(freqHz);
}

void StereoFieldShaper::setWidth(float width) noexcept {
    targetWidth_ = std::clamp(width, 0.0f, 2.0f);
}

void StereoFieldShaper::setBypassed(bool bypassed) noexcept {
    bypassed_ = bypassed;
}

void StereoFieldShaper::setSoloLow(bool solo) noexcept {
    soloLow_ = solo;
}

void StereoFieldShaper::processSample(float inL, float inR, float& outL, float& outR) noexcept {
    if (bypassed_) {
        outL = inL;
        outR = inR;
        return;
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
    if (bypassed_ || leftChannel == nullptr || numSamples <= 0) {
        return;
    }

    const float startWidth = currentWidth_;
    const float endWidth = targetWidth_;
    const float widthStep = (endWidth - startWidth) / static_cast<float>(numSamples);

    if (rightChannel == nullptr) {
        // Mono channel processing
        for (int i = 0; i < numSamples; ++i) {
            const auto split = crossoverL_.process(leftChannel[i]);
            leftChannel[i] = soloLow_ ? split.low : (split.low + split.high);
        }
        currentWidth_ = endWidth;
        return;
    }

    // Stereo channel processing with click-free width interpolation
    for (int i = 0; i < numSamples; ++i) {
        currentWidth_ += widthStep;

        const float dryL = leftChannel[i];
        const float dryR = rightChannel[i];

        const auto splitL = crossoverL_.process(dryL);
        const auto splitR = crossoverR_.process(dryR);

        const float midLow = 0.5f * (splitL.low + splitR.low);
        const float sideLow = 0.5f * (splitL.low - splitR.low);

        const float shapedSideLow = sideLow * currentWidth_;

        const float shapedLowL = midLow + shapedSideLow;
        const float shapedLowR = midLow - shapedSideLow;

        if (soloLow_) {
            leftChannel[i] = shapedLowL;
            rightChannel[i] = shapedLowR;
        } else {
            leftChannel[i] = shapedLowL + splitL.high;
            rightChannel[i] = shapedLowR + splitR.high;
        }
    }

    currentWidth_ = endWidth;
}

} // namespace analog_master::core
