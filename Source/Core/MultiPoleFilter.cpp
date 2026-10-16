#include "MultiPoleFilter.h"
#include <algorithm>

namespace analog_master::core {

MultiPoleFilter::MultiPoleFilter(FilterType type)
    : type_(type) {
}

void MultiPoleFilter::prepare(double sampleRate) noexcept {
    if (sampleRate <= 0.0) {
        return;
    }
    sampleRate_ = sampleRate;
    currentCutoffHz_ = targetCutoffHz_;
    currentBypassGain_ = bypassed_ ? 0.0f : 1.0f;
    reset();
    updateCoefficients();
}

void MultiPoleFilter::reset() noexcept {
    for (auto& chFilters : filters_) {
        for (auto& section : chFilters) {
            section.reset();
        }
    }
}

void MultiPoleFilter::setType(FilterType type) noexcept {
    if (type_ == type) {
        return;
    }
    type_ = type;
    updateCoefficients();
}

void MultiPoleFilter::setCutoff(double cutoffHz) noexcept {
    if (std::abs(targetCutoffHz_ - cutoffHz) < 0.001) {
        return;
    }
    targetCutoffHz_ = cutoffHz;
    // Immediate update if not prepared or ready
    if (std::abs(currentCutoffHz_ - targetCutoffHz_) > 0.001) {
        updateCoefficients();
    }
}

void MultiPoleFilter::setSlope(FilterSlope slope) noexcept {
    if (slope_ == slope) {
        return;
    }
    slope_ = slope;
    updateCoefficients();
}

void MultiPoleFilter::setBypassed(bool bypassed) noexcept {
    bypassed_ = bypassed;
}

void MultiPoleFilter::updateCoefficients() noexcept {
    const int order = slopeToOrder(slope_);
    const FilterDesign design = (type_ == FilterType::LowPass)
        ? designButterworthLowPass(sampleRate_, targetCutoffHz_, order)
        : designButterworthHighPass(sampleRate_, targetCutoffHz_, order);

    const int newActiveSections = design.count;

    // Reset unused / newly activated sections to prevent clicks from historical stale states
    if (newActiveSections != activeSections_) {
        for (int ch = 0; ch < 2; ++ch) {
            for (int s = std::min(activeSections_, newActiveSections); s < kMaxSections; ++s) {
                filters_[ch][s].reset();
            }
        }
    }

    activeSections_ = newActiveSections;
    currentCutoffHz_ = targetCutoffHz_;

    for (int s = 0; s < activeSections_; ++s) {
        filters_[0][s].setCoefficients(design.sections[s]);
        filters_[1][s].setCoefficients(design.sections[s]);
    }
}

float MultiPoleFilter::processSample(int channel, float sample) noexcept {
    if (bypassed_ || channel < 0 || channel >= 2) {
        return sample;
    }

    float current = sample;
    for (int s = 0; s < activeSections_; ++s) {
        current = filters_[channel][s].process(current);
    }
    return current;
}

void MultiPoleFilter::processBlock(float* const* channelData, int numChannels, int numSamples) noexcept {
    if (bypassed_ || channelData == nullptr || numSamples <= 0) {
        return;
    }

    const int channelsToProcess = std::min(numChannels, 2);
    for (int ch = 0; ch < channelsToProcess; ++ch) {
        float* data = channelData[ch];
        if (data == nullptr) {
            continue;
        }

        for (int i = 0; i < numSamples; ++i) {
            float s = data[i];
            for (int sec = 0; sec < activeSections_; ++sec) {
                s = filters_[ch][sec].process(s);
            }
            data[i] = s;
        }
    }
}

} // namespace analog_master::core
