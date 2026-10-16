#include "StereoFieldShaper.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace analog_master::core {

void StereoFieldShaper::prepare(double sampleRate) noexcept {
    if (sampleRate <= 0.0) {
        return;
    }
    sampleRate_ = sampleRate;
    crossoverL_.prepare(sampleRate);
    crossoverR_.prepare(sampleRate);
    updateDecorrelator();
    currentBypassGain_ = bypassed_ ? 0.0f : 1.0f;
    currentSoloGain_ = soloLow_ ? 1.0f : 0.0f;
    reset();
}

void StereoFieldShaper::reset() noexcept {
    crossoverL_.reset();
    crossoverR_.reset();
    for (auto& ap : decorrelator_) {
        ap.reset();
    }
    currentWidth_ = targetWidth_;
}

void StereoFieldShaper::updateDecorrelator() noexcept {
    // 4-stage cascaded 2nd-order allpass phase-dispersion network across the sub/low-bass spectrum.
    // Synthesizes a 100% mono-compatible orthogonal side signal from the mid channel when expanding (> 100%),
    // so even dead-center mono kick/bass instruments bloom into a wide stereo field above 100% width.
    constexpr double kPolesHz[4] = { 38.0, 76.0, 152.0, 304.0 };
    constexpr double kQ = 0.7071067811865475;

    for (size_t i = 0; i < 4; ++i) {
        const double safeFc = std::clamp(kPolesHz[i], 10.0, sampleRate_ * 0.49);
        const double K = std::tan(std::numbers::pi * safeFc / sampleRate_);
        const double K2 = K * K;
        const double norm = 1.0 / (1.0 + (K / kQ) + K2);

        const double b0 = (1.0 - (K / kQ) + K2) * norm;
        const double b1 = 2.0 * (K2 - 1.0) * norm;
        const double b2 = 1.0;
        const double a1 = b1;
        const double a2 = b0;

        decorrelator_[i].setCoefficients({ b0, b1, b2, a1, a2 });
    }
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

    // Pass mid through allpass dispersion network to keep state warm and synthesize stereo expansion above 1.0
    float decorrMid = midLow;
    for (auto& ap : decorrelator_) {
        decorrMid = ap.process(decorrMid);
    }

    const float enhance = std::max(0.0f, targetWidth_ - 1.0f);
    const float shapedSideLow = sideLow * targetWidth_ + decorrMid * (enhance * 0.45f);

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

        float decorrMid = midLow;
        for (auto& ap : decorrelator_) {
            decorrMid = ap.process(decorrMid);
        }

        const float enhance = std::max(0.0f, currentWidth_ - 1.0f);
        const float shapedSideLow = sideLow * currentWidth_ + decorrMid * (enhance * 0.45f);

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
