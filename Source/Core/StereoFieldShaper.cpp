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
    currentCrossoverHz_ = crossoverHz_;
    crossoverL_.prepare(sampleRate);
    crossoverR_.prepare(sampleRate);
    crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
    crossoverR_.setCrossoverFrequency(currentCrossoverHz_);
    updateDecorrelator(currentCrossoverHz_);
    currentBypassGain_ = bypassed_ ? 0.0f : 1.0f;
    currentSoloGain_ = soloLow_ ? 1.0f : 0.0f;
    hasProcessedAudio_ = false;
    reset();
}

void StereoFieldShaper::reset() noexcept {
    crossoverL_.reset();
    crossoverR_.reset();
    for (auto& ap : decorrelator_) {
        ap.reset();
    }
    currentCrossoverHz_ = crossoverHz_;
    crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
    crossoverR_.setCrossoverFrequency(currentCrossoverHz_);
    updateDecorrelator(currentCrossoverHz_);
    currentWidth_ = targetWidth_;
    currentBypassGain_ = bypassed_ ? 0.0f : 1.0f;
    currentSoloGain_ = soloLow_ ? 1.0f : 0.0f;
    hasProcessedAudio_ = false;
}

void StereoFieldShaper::updateDecorrelator(double refCrossoverHz) noexcept {
    // 4-stage cascaded 2nd-order allpass phase-dispersion network across the sub/low-bass spectrum.
    // Poles scale gently around the active crossover frequency so phase dispersion stays optimally centered.
    const double scale = std::clamp(refCrossoverHz / 120.0, 0.5, 2.2);
    const double polesHz[4] = {
        38.0 * scale,
        76.0 * scale,
        152.0 * scale,
        304.0 * scale
    };
    constexpr double kQ = 0.7071067811865475;

    for (size_t i = 0; i < 4; ++i) {
        const double safeFc = std::clamp(polesHz[i], 10.0, sampleRate_ * 0.49);
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
    const double clamped = std::clamp(freqHz, 20.0, 500.0);
    if (std::abs(crossoverHz_ - clamped) < 0.001) {
        return;
    }
    crossoverHz_ = clamped;
    if (!hasProcessedAudio_) {
        currentCrossoverHz_ = crossoverHz_;
        crossoverL_.setCrossoverFrequency(currentCrossoverHz_);
        crossoverR_.setCrossoverFrequency(currentCrossoverHz_);
        updateDecorrelator(currentCrossoverHz_);
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
        updateDecorrelator(currentCrossoverHz_);
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
            updateDecorrelator(currentCrossoverHz_);
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

            float decorrMid = midLow;
            for (auto& ap : decorrelator_) {
                decorrMid = ap.process(decorrMid);
            }

            const float enhance = std::max(0.0f, currentWidth_ - 1.0f);
            const float shapedSideLow = sideLow * currentWidth_ + decorrMid * (enhance * 0.45f);

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
