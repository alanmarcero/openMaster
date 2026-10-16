#pragma once

#include "BiquadFilter.h"
#include <array>

namespace analog_master::core {

struct CrossoverSplit {
    float low = 0.0f;
    float high = 0.0f;
};

// 4th-order Linkwitz-Riley (LR4) crossover filter for one audio channel.
// Low-pass and High-pass outputs sum to flat unity magnitude.
class LinkwitzRiley4 {
public:
    LinkwitzRiley4() = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setCrossoverFrequency(double freqHz) noexcept;

    [[nodiscard]] double crossoverFrequency() const noexcept { return crossoverHz_; }

    [[nodiscard]] CrossoverSplit process(float sample) noexcept;

private:
    void updateCoefficients() noexcept;

    double sampleRate_ = 44100.0;
    double crossoverHz_ = 120.0;

    // Two cascaded Butterworth sections for LP
    BiquadFilter lp1_{};
    BiquadFilter lp2_{};

    // Two cascaded Butterworth sections for HP
    BiquadFilter hp1_{};
    BiquadFilter hp2_{};
};

} // namespace analog_master::core
