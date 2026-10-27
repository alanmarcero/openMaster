#include "MasterProcessorCore.h"
#include <algorithm>
#include <cmath>
#include <numbers>

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

void MasterProcessorCore::prepare(double sampleRate, int channels) {
    if (!std::isfinite(sampleRate) || sampleRate < 8000.0 || sampleRate > 384000.0) {
        return;
    }
    sampleRate_ = sampleRate;
    currentMasterBypassGain_ = masterBypassed_ ? 0.0f : 1.0f;
    hasProcessedAudio_ = false;
    lowCut_.prepare(sampleRate);
    stereoShaper_.prepare(sampleRate);
    highCut_.prepare(sampleRate);
    tone_.prepare(sampleRate);
    compressor_.prepare(sampleRate);
    dynamicEq_.prepare(sampleRate);
    saturation_.prepare(sampleRate);
    clipper_.prepare(sampleRate);
    limiter_.prepare(sampleRate);
    truePeak_.prepare(sampleRate);
    outputLoudness_.prepare(sampleRate, channels);
    dryLoudness_.prepare(sampleRate, channels, false);
    wetLoudness_.prepare(sampleRate, channels, false);
    latency_ = saturation_.latencySamples() + clipper_.latencySamples() + limiter_.latencySamples();
    dryDelay_.assign(static_cast<size_t>(std::max(1, latency_)), {});
    bypassStep_ = static_cast<float>(1. / (.01 * sampleRate));
    gainAlpha_ = static_cast<float>(1. - std::exp(-1. / (.005 * sampleRate)));
    matchAlpha_ = static_cast<float>(1. - std::exp(-1. / (.5 * sampleRate)));
    reset();
}

void MasterProcessorCore::reset() noexcept {
    lowCut_.reset();
    stereoShaper_.reset();
    highCut_.reset();
    currentGainLinear_ = targetGainLinear_;
    currentMasterBypassGain_ = masterBypassed_ ? 0.0f : 1.0f;
    hasProcessedAudio_ = false;
    tone_.reset();
    compressor_.reset();
    dynamicEq_.reset();
    saturation_.reset();
    clipper_.reset();
    limiter_.reset();
    dryLoudness_.reset();
    wetLoudness_.reset();
    std::fill(dryDelay_.begin(), dryDelay_.end(), std::array<float, 2>{});
    dryCursor_ = 0;
    wetMatch_ = dryMatch_ = 1;
    resetMeters();
}

void MasterProcessorCore::resetMeters() noexcept {
    outputLoudness_.reset();
    truePeak_.reset();
    maximumTruePeak_ = 0;
    shortTermLufs_.store(-INFINITY, std::memory_order_relaxed);
    integratedLufs_.store(-INFINITY, std::memory_order_relaxed);
    truePeakDbtp_.store(-INFINITY, std::memory_order_relaxed);
    compressorGr_.store(0);
    dynamicEqGr_.store(0);
    clipperGr_.store(0);
    limiterGr_.store(0);
    matchDb_.store(0);
    matchReady_.store(false);
    meterPeakL_.store(0.0f, std::memory_order_relaxed);
    meterPeakR_.store(0.0f, std::memory_order_relaxed);
    meterVuL_.store(0.0f, std::memory_order_relaxed);
    meterVuR_.store(0.0f, std::memory_order_relaxed);
    meterClipL_.store(false, std::memory_order_relaxed);
    meterClipR_.store(false, std::memory_order_relaxed);
    meterCorrelation_.store(1.0f, std::memory_order_relaxed);
    correlationLL_ = 0.0;
    correlationRR_ = 0.0;
    correlationLR_ = 0.0;
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

void MasterProcessorCore::processBlock(float *leftChannel, float *rightChannel, int numSamples) noexcept {
    if (!leftChannel || numSamples <= 0 || dryDelay_.empty())
        return;
    if (resetMetersRequested_.exchange(false, std::memory_order_relaxed))
        resetMeters();
    const bool stereo = rightChannel != nullptr;
    if (!hasProcessedAudio_)
        currentMasterBypassGain_ = (masterBypassed_ || hostBypassed_) ? 0.f : 1.f;
    hasProcessedAudio_ = true;
    const float targetBypass = (masterBypassed_ || hostBypassed_) ? 0.f : 1.f;
    const float peakDecay = static_cast<float>(std::exp(-1. / (.3 * sampleRate_)));
    const float vuAlpha = static_cast<float>(1. - std::exp(-std::log(100.) / (.3 * sampleRate_)));
    constexpr float vuCalibration = std::numbers::pi_v<float> / (2.f * std::numbers::sqrt2_v<float>);
    float peakL = meterPeakL_.load(), peakR = meterPeakR_.load();
    float vuL = meterVuL_.load(), vuR = meterVuR_.load();
    bool clipL = false, clipR = false;
    float compGr = 0, dynGr = 0, clipGr = 0, limitGr = 0;
    for (int offset = 0; offset < numSamples; offset += kChunkSize) {
        const int count = std::min(kChunkSize, numSamples - offset);
        std::array<float, kChunkSize> wetL{}, wetR{};
        for (int i = 0; i < count; ++i) {
            const float l = leftChannel[offset + i], r = stereo ? rightChannel[offset + i] : l;
            wetL[i] = std::isfinite(l) ? l : 0;
            wetR[i] = std::isfinite(r) ? r : 0;
            const auto delayed = dryDelay_[static_cast<size_t>(dryCursor_)];
            dryDelay_[static_cast<size_t>(dryCursor_)] = {wetL[i], wetR[i]};
            dryCursor_ = (dryCursor_ + 1) % static_cast<int>(dryDelay_.size());
            dryL_[i] = delayed[0];
            dryR_[i] = delayed[1];
        }
        float *channels[]{wetL.data(), wetR.data()};
        lowCut_.processBlock(channels, stereo ? 2 : 1, count);
        stereoShaper_.processBlock(wetL.data(), stereo ? wetR.data() : nullptr, count);
        highCut_.processBlock(channels, stereo ? 2 : 1, count);
        for (int i = 0; i < count; ++i) {
            float l = wetL[i], r = stereo ? wetR[i] : l;
            tone_.process(l, r);
            compressor_.process(l, r);
            dynamicEq_.process(l, r);
            saturation_.process(l, r);
            currentGainLinear_ += gainAlpha_ * (targetGainLinear_ - currentGainLinear_);
            l *= currentGainLinear_;
            r *= currentGainLinear_;
            clipper_.process(l, r);
            limiter_.process(l, r);
            compGr = std::max(compGr, compressor_.gainReductionDb());
            dynGr = std::max(dynGr, dynamicEq_.gainReductionDb());
            clipGr = std::max(clipGr, clipper_.gainReductionDb());
            limitGr = std::max(limitGr, limiter_.gainReductionDb());
            dryLoudness_.process(dryL_[i], stereo ? dryR_[i] : 0);
            wetLoudness_.process(l, stereo ? r : 0);
            const float dryLu = dryLoudness_.shortTermLufs(), wetLu = wetLoudness_.shortTermLufs();
            const bool ready = dryLoudness_.shortTermReady() && wetLoudness_.shortTermReady() &&
                               std::isfinite(dryLu) && std::isfinite(wetLu) && dryLu > -60 && wetLu > -60;
            float wetTarget = 1, dryTarget = 1;
            if (matchBypass_ && ready) {
                const float difference = std::clamp(dryLu - wetLu, -12.f, 12.f);
                // Attenuate the louder branch; comparison never boosts past the limiter ceiling.
                wetTarget = std::pow(10.f, std::min(0.f, difference) / 20.f);
                dryTarget = std::pow(10.f, std::min(0.f, -difference) / 20.f);
            } else if (matchBypass_) {
                wetTarget = wetMatch_;
                dryTarget = dryMatch_;
            }
            wetMatch_ += matchAlpha_ * (wetTarget - wetMatch_);
            dryMatch_ += matchAlpha_ * (dryTarget - dryMatch_);
            currentMasterBypassGain_ +=
                std::clamp(targetBypass - currentMasterBypassGain_, -bypassStep_, bypassStep_);
            const float mix = currentMasterBypassGain_;
            l = mix * l * wetMatch_ + (1.f - mix) * dryL_[i] * dryMatch_;
            r = mix * r * wetMatch_ + (1.f - mix) * dryR_[i] * dryMatch_;
            leftChannel[offset + i] = l;
            if (stereo)
                rightChannel[offset + i] = r;
            outputLoudness_.process(l, stereo ? r : 0);
            maximumTruePeak_ = std::max(maximumTruePeak_, truePeak_.process(l, stereo ? r : l));
            peakL = std::max(std::abs(l), peakL * peakDecay);
            peakR = stereo ? std::max(std::abs(r), peakR * peakDecay) : 0;
            vuL += vuAlpha * (std::abs(l) * vuCalibration - vuL);
            vuR = stereo ? vuR + vuAlpha * (std::abs(r) * vuCalibration - vuR) : 0;
            clipL |= std::abs(l) >= .999f;
            clipR |= stereo && std::abs(r) >= .999f;
            correlationLL_ += vuAlpha * (static_cast<double>(l) * l - correlationLL_);
            correlationRR_ += vuAlpha * (static_cast<double>(r) * r - correlationRR_);
            correlationLR_ += vuAlpha * (static_cast<double>(l) * r - correlationLR_);
        }
    }
    const double denom = std::sqrt(correlationLL_ * correlationRR_);
    meterCorrelation_.store(
        stereo && denom > 1e-12 ? std::clamp(static_cast<float>(correlationLR_ / denom), -1.f, 1.f) : 1.f);
    meterPeakL_.store(peakL);
    meterPeakR_.store(peakR);
    meterVuL_.store(vuL);
    meterVuR_.store(vuR);
    if (clipL)
        meterClipL_.store(true);
    if (clipR)
        meterClipR_.store(true);
    shortTermLufs_.store(outputLoudness_.shortTermLufs());
    integratedLufs_.store(outputLoudness_.integratedLufs());
    truePeakDbtp_.store(maximumTruePeak_ > 1e-12f ? 20.f * std::log10(maximumTruePeak_) : -INFINITY);
    compressorGr_.store(compGr);
    dynamicEqGr_.store(dynGr);
    clipperGr_.store(clipGr);
    limiterGr_.store(limitGr);
    matchDb_.store(20.f * std::log10(wetMatch_ / dryMatch_));
    matchReady_.store(dryLoudness_.shortTermReady() && wetLoudness_.shortTermReady() &&
                      dryLoudness_.shortTermLufs() > -60 && wetLoudness_.shortTermLufs() > -60);
}

} // namespace analog_master::core
