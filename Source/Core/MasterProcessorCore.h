#pragma once

#include "MultiPoleFilter.h"
#include "StereoFieldShaper.h"
#include <array>
#include <atomic>

namespace analog_master::core {

struct MeterLevels {
    float peakL = 0.0f;
    float peakR = 0.0f;
    float vuL = 0.0f;
    float vuR = 0.0f;
    bool clipL = false;
    bool clipR = false;
    float correlation = 1.0f; // -1.0 (out of phase) .. 0.0 (wide/uncorrelated) .. +1.0 (mono/in-phase)
};

class MasterProcessorCore {
public:
    MasterProcessorCore();

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    // Low Cut Filter
    void setLowCutFrequency(double freqHz) noexcept;
    void setLowCutSlope(FilterSlope slope) noexcept;
    void setLowCutBypassed(bool bypassed) noexcept;

    // High Cut Filter
    void setHighCutFrequency(double freqHz) noexcept;
    void setHighCutSlope(FilterSlope slope) noexcept;
    void setHighCutBypassed(bool bypassed) noexcept;

    // Low-end Stereo Field Shaper
    void setStereoFieldFrequency(double freqHz) noexcept;
    void setStereoFieldWidth(float width) noexcept; // 0.0 (mono) .. 1.0 (normal) .. 2.0 (enhanced)
    void setStereoFieldBypassed(bool bypassed) noexcept;
    void setStereoFieldSoloLow(bool solo) noexcept;

    // Master Output & Global Bypass
    void setMasterGainDb(float gainDb) noexcept;
    void setMasterBypassed(bool bypassed) noexcept;

    [[nodiscard]] double lowCutFrequency() const noexcept { return lowCut_.cutoff(); }
    [[nodiscard]] FilterSlope lowCutSlope() const noexcept { return lowCut_.slope(); }
    [[nodiscard]] bool isLowCutBypassed() const noexcept { return lowCut_.isBypassed(); }

    [[nodiscard]] double highCutFrequency() const noexcept { return highCut_.cutoff(); }
    [[nodiscard]] FilterSlope highCutSlope() const noexcept { return highCut_.slope(); }
    [[nodiscard]] bool isHighCutBypassed() const noexcept { return highCut_.isBypassed(); }

    [[nodiscard]] double stereoFieldFrequency() const noexcept { return stereoShaper_.crossoverFrequency(); }
    [[nodiscard]] float stereoFieldWidth() const noexcept { return stereoShaper_.width(); }
    [[nodiscard]] bool isStereoFieldBypassed() const noexcept { return stereoShaper_.isBypassed(); }
    [[nodiscard]] bool isStereoFieldSoloLow() const noexcept { return stereoShaper_.isSoloLow(); }

    [[nodiscard]] float masterGainDb() const noexcept { return masterGainDb_; }
    [[nodiscard]] bool isMasterBypassed() const noexcept { return masterBypassed_; }

    [[nodiscard]] MeterLevels meterLevels() const noexcept {
        return {
            meterPeakL_.load(std::memory_order_relaxed),
            meterPeakR_.load(std::memory_order_relaxed),
            meterVuL_.load(std::memory_order_relaxed),
            meterVuR_.load(std::memory_order_relaxed),
            meterClipL_.load(std::memory_order_relaxed),
            meterClipR_.load(std::memory_order_relaxed),
            meterCorrelation_.load(std::memory_order_relaxed)
        };
    }

    void processBlock(float* leftChannel, float* rightChannel, int numSamples) noexcept;

private:
    static constexpr int kBypassFadeSamples = 512;

    double sampleRate_ = 44100.0;
    float masterGainDb_ = 0.0f;
    float targetGainLinear_ = 1.0f;
    float currentGainLinear_ = 1.0f;
    bool masterBypassed_ = false;
    float currentMasterBypassGain_ = 1.0f; // 1.0 = power ON, 0.0 = bypassed
    bool hasProcessedAudio_ = false;

    MultiPoleFilter lowCut_{ FilterType::HighPass };
    MultiPoleFilter highCut_{ FilterType::LowPass };
    StereoFieldShaper stereoShaper_{};
    std::array<float, kBypassFadeSamples> dryL_{};
    std::array<float, kBypassFadeSamples> dryR_{};

    std::atomic<float> meterPeakL_{ 0.0f };
    std::atomic<float> meterPeakR_{ 0.0f };
    std::atomic<float> meterVuL_{ 0.0f };
    std::atomic<float> meterVuR_{ 0.0f };
    std::atomic<bool> meterClipL_{ false };
    std::atomic<bool> meterClipR_{ false };
    std::atomic<float> meterCorrelation_{ 1.0f };
};

} // namespace analog_master::core
