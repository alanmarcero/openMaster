#pragma once

#include "MultiPoleFilter.h"
#include "StereoFieldShaper.h"
#include "MasteringStages.h"
#include "LoudnessMeter.h"
#include <vector>
#include <array>
#include <atomic>

namespace analog_master::core {
static_assert(std::atomic<float>::is_always_lock_free && std::atomic<bool>::is_always_lock_free);

struct MeterLevels {
    float peakL = 0.0f;
    float peakR = 0.0f;
    float vuL = 0.0f; // sine RMS calibration: a full-scale sine reads 0.707
    float vuR = 0.0f;
    bool clipL = false;
    bool clipR = false;
    float correlation = 1.0f; // -1.0 (out of phase) .. 0.0 (wide/uncorrelated) .. +1.0 (mono/in-phase)
    float shortTermLufs = -std::numeric_limits<float>::infinity();
    float integratedLufs = -std::numeric_limits<float>::infinity();
    float truePeakDbtp = -std::numeric_limits<float>::infinity();
    float compressorGr = 0, dynamicEqGr = 0, clipperGr = 0, limiterGr = 0;
    float matchDb = 0;
    bool matchReady = false;
};

class MasterProcessorCore {
public:
    MasterProcessorCore();

    void prepare(double sampleRate, int channels = 2);
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

    void setTone(const ToneSettings &s) noexcept { tone_.setSettings(s); }
    void setCompressor(const CompressorSettings &s) noexcept { compressor_.setSettings(s); }
    void setDynamicEq(const DynamicEqSettings &s) noexcept { dynamicEq_.setSettings(s); }
    void setSaturation(const SaturationSettings &s) noexcept { saturation_.setSettings(s); }
    void setClipper(const ClipperSettings &s) noexcept { clipper_.setSettings(s); }
    void setLimiter(const LimiterSettings &s) noexcept { limiter_.setSettings(s); }
    void setMatchBypass(bool enabled) noexcept { matchBypass_ = enabled; }
    void setHostBypassed(bool enabled) noexcept { hostBypassed_ = enabled; }
    void requestMeterReset() noexcept { resetMetersRequested_.store(true, std::memory_order_relaxed); }
    [[nodiscard]] int latencySamples() const noexcept { return latency_; }

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
        MeterLevels m;
        m.peakL = meterPeakL_.load(std::memory_order_relaxed);
        m.peakR = meterPeakR_.load(std::memory_order_relaxed);
        m.vuL = meterVuL_.load(std::memory_order_relaxed);
        m.vuR = meterVuR_.load(std::memory_order_relaxed);
        m.clipL = meterClipL_.load(std::memory_order_relaxed);
        m.clipR = meterClipR_.load(std::memory_order_relaxed);
        m.correlation = meterCorrelation_.load(std::memory_order_relaxed);
        m.shortTermLufs = shortTermLufs_.load(std::memory_order_relaxed);
        m.integratedLufs = integratedLufs_.load(std::memory_order_relaxed);
        m.truePeakDbtp = truePeakDbtp_.load(std::memory_order_relaxed);
        m.compressorGr = compressorGr_.load(std::memory_order_relaxed);
        m.dynamicEqGr = dynamicEqGr_.load(std::memory_order_relaxed);
        m.clipperGr = clipperGr_.load(std::memory_order_relaxed);
        m.limiterGr = limiterGr_.load(std::memory_order_relaxed);
        m.matchDb = matchDb_.load(std::memory_order_relaxed);
        m.matchReady = matchReady_.load(std::memory_order_relaxed);
        return m;
    }

    // Reads the levels and clears the clip latches
    [[nodiscard]] MeterLevels takeMeterLevels() noexcept {
        auto levels = meterLevels();
        levels.clipL = meterClipL_.exchange(false, std::memory_order_relaxed);
        levels.clipR = meterClipR_.exchange(false, std::memory_order_relaxed);
        return levels;
    }

    void processBlock(float *leftChannel, float *rightChannel, int numSamples) noexcept;

private:
    static constexpr int kChunkSize = 256;
    void resetMeters() noexcept;
    ToneStage tone_;
    CompressorStage compressor_;
    DynamicEqStage dynamicEq_;
    SaturationStage saturation_;
    ClipperStage clipper_;
    LimiterStage limiter_;
    TruePeakDetector truePeak_;
    LoudnessMeter outputLoudness_, dryLoudness_, wetLoudness_;
    std::vector<std::array<float, 2>> dryDelay_;
    int latency_ = 0, dryCursor_ = 0;
    bool matchBypass_ = false, hostBypassed_ = false;
    float wetMatch_ = 1, dryMatch_ = 1, maximumTruePeak_ = 0;
    float bypassStep_ = 1.f / 512.f, gainAlpha_ = .001f, matchAlpha_ = .00002f;
    std::atomic<bool> resetMetersRequested_{false}, matchReady_{false};
    std::atomic<float> shortTermLufs_{-INFINITY}, integratedLufs_{-INFINITY}, truePeakDbtp_{-INFINITY};
    std::atomic<float> compressorGr_{0}, dynamicEqGr_{0}, clipperGr_{0}, limiterGr_{0}, matchDb_{0};

    double sampleRate_ = 44100.0;
    float masterGainDb_ = 0.0f;
    float targetGainLinear_ = 1.0f;
    float currentGainLinear_ = 1.0f;
    bool masterBypassed_ = false;
    float currentMasterBypassGain_ = 1.0f; // 1.0 = power ON, 0.0 = bypassed
    bool hasProcessedAudio_ = false;

    MultiPoleFilter lowCut_{FilterType::HighPass};
    MultiPoleFilter highCut_{FilterType::LowPass};
    StereoFieldShaper stereoShaper_{};
    std::array<float, kChunkSize> dryL_{};
    std::array<float, kChunkSize> dryR_{};

    std::atomic<float> meterPeakL_{0.0f};
    std::atomic<float> meterPeakR_{0.0f};
    std::atomic<float> meterVuL_{0.0f};
    std::atomic<float> meterVuR_{0.0f};
    std::atomic<bool> meterClipL_{false};
    std::atomic<bool> meterClipR_{false};
    std::atomic<float> meterCorrelation_{1.0f};
    double correlationLL_ = 0.0;
    double correlationRR_ = 0.0;
    double correlationLR_ = 0.0;
};

} // namespace analog_master::core
