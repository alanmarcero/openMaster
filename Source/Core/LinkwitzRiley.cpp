#include "LinkwitzRiley.h"
#include <algorithm>
#include <numbers>

namespace analog_master::core {

void LinkwitzRiley4::prepare(double sampleRate) noexcept {
    if (sampleRate <= 0.0) {
        return;
    }
    sampleRate_ = sampleRate;
    reset();
    updateCoefficients();
}

void LinkwitzRiley4::reset() noexcept {
    lp1_.reset();
    lp2_.reset();
    hp1_.reset();
    hp2_.reset();
}

void LinkwitzRiley4::setCrossoverFrequency(double freqHz) noexcept {
    if (std::abs(crossoverHz_ - freqHz) < 0.001) {
        return;
    }
    crossoverHz_ = freqHz;
    updateCoefficients();
}

void LinkwitzRiley4::updateCoefficients() noexcept {
    const double maxCutoff = sampleRate_ * 0.49;
    const double safeCutoff = std::clamp(crossoverHz_, 20.0, maxCutoff);

    const double K = std::tan(std::numbers::pi * safeCutoff / sampleRate_);
    const double K2 = K * K;
    constexpr double sqrt2 = std::numbers::sqrt2;

    const double norm = 1.0 / (1.0 + sqrt2 * K + K2);

    // 2nd-order Butterworth LP coefficients
    const double lpB0 = K2 * norm;
    const double lpB1 = 2.0 * lpB0;
    const double lpB2 = lpB0;
    const double a1 = 2.0 * (K2 - 1.0) * norm;
    const double a2 = (1.0 - sqrt2 * K + K2) * norm;

    const BiquadCoeffs lpCoeffs { lpB0, lpB1, lpB2, a1, a2 };
    lp1_.setCoefficients(lpCoeffs);
    lp2_.setCoefficients(lpCoeffs);

    // 2nd-order Butterworth HP coefficients
    const double hpB0 = norm;
    const double hpB1 = -2.0 * norm;
    const double hpB2 = norm;

    const BiquadCoeffs hpCoeffs { hpB0, hpB1, hpB2, a1, a2 };
    hp1_.setCoefficients(hpCoeffs);
    hp2_.setCoefficients(hpCoeffs);
}

CrossoverSplit LinkwitzRiley4::process(float sample) noexcept {
    float low = lp2_.process(lp1_.process(sample));
    float high = hp2_.process(hp1_.process(sample));
    return { low, high };
}

} // namespace analog_master::core
