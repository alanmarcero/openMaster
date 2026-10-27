#pragma once
#include "MasteringSettings.h"
#include <memory>

namespace analog_master::core {
// Audio-thread objects. prepare() owns all allocation; the other methods are bounded
// and allocation-free. Enable and parameter changes ramp over 10 ms. Delayed stages
// keep their prepared latency even when disabled.
#define OPEN_MASTER_STAGE_DECL(Name, Settings)                                                               \
    class Name {                                                                                             \
    public:                                                                                                  \
        Name();                                                                                              \
        ~Name();                                                                                             \
        Name(const Name &) = delete;                                                                         \
        Name &operator=(const Name &) = delete;                                                              \
        void prepare(double sampleRate);                                                                     \
        void reset() noexcept;                                                                               \
        void setSettings(const Settings &) noexcept;                                                         \
        void process(float &left, float &right) noexcept;                                                    \
        [[nodiscard]] int latencySamples() const noexcept;                                                   \
        [[nodiscard]] float gainReductionDb() const noexcept;                                                \
                                                                                                             \
    private:                                                                                                 \
        struct Impl;                                                                                         \
        std::unique_ptr<Impl> impl_;                                                                         \
        Settings settings_{};                                                                                \
    };
OPEN_MASTER_STAGE_DECL(ToneStage, ToneSettings)
OPEN_MASTER_STAGE_DECL(CompressorStage, CompressorSettings)
OPEN_MASTER_STAGE_DECL(DynamicEqStage, DynamicEqSettings)
OPEN_MASTER_STAGE_DECL(SaturationStage, SaturationSettings)
OPEN_MASTER_STAGE_DECL(ClipperStage, ClipperSettings)
OPEN_MASTER_STAGE_DECL(LimiterStage, LimiterSettings)
#undef OPEN_MASTER_STAGE_DECL

// 16-phase windowed-sinc reconstruction; process returns the maximum for the
// current reconstructed frame, including both channels and the raw sample peak.
class TruePeakDetector {
public:
    TruePeakDetector();
    ~TruePeakDetector();
    TruePeakDetector(const TruePeakDetector &) = delete;
    TruePeakDetector &operator=(const TruePeakDetector &) = delete;
    void prepare(double sampleRate);
    void reset() noexcept;
    [[nodiscard]] float process(float left, float right) noexcept;
    [[nodiscard]] int latencySamples() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace analog_master::core
