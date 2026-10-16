#include "StereoFieldShaper.h"
#include <algorithm>

namespace analog_master::core {

void StereoFieldShaper::prepare(double sampleRate) noexcept {
    if (sampleRate <= 0.0) {
        return;
    }
    crossoverL_.prepare(sampleRate);
    crossoverR_.prepare(sampleRate);
    reset();
}

void StereoFieldShaper::reset() noexcept {
    crossoverL_.reset();
    crossoverR_.reset();
}

void StereoFieldShaper::setCrossoverFrequency(double freqHz) noexcept {
    crossoverHz_ = freqHz;
    crossoverL_.setCrossoverFrequency(freqHz);
    crossoverR_.setCrossoverFrequency(freqHz);
}

void StereoFieldShaper::setWidth(float width) noexcept {
    width_ = std::clamp(width, 0.0f, 2.0f);
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

    // Mid/Side matrix of the low frequencies
    const float midLow = 0.5f * (splitL.low + splitR.low);
    const float sideLow = 0.5f * (splitL.low - splitR.low);

    // Adjust stereo field only for the low frequencies below the crossover knob
    const float shapedSideLow = sideLow * width_;

    const float shapedLowL = midLow + shapedSideLow;
    const float shapedLowR = midLow - shapedSideLow;

    if (soloLow_) {
        outL = shapedLowL;
        outR = shapedLowR;
        return;
    }

    // Recombine with untouched high frequencies
    outL = shapedLowL + splitL.high;
    outR = shapedLowR + splitR.high;
}

void StereoFieldShaper::processBlock(float* leftChannel, float* rightChannel, int numSamples) noexcept {
    if (bypassed_ || leftChannel == nullptr || rightChannel == nullptr) {
        return;
    }

    for (int i = 0; i < numSamples; ++i) {
        processSample(leftChannel[i], rightChannel[i], leftChannel[i], rightChannel[i]);
    }
}

} // namespace analog_master::core
