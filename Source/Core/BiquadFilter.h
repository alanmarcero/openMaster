#pragma once

#include <cmath>

namespace analog_master::core {

struct BiquadCoeffs {
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

// Transposed Direct Form II Biquad Section with 64-bit double precision accumulation
class BiquadFilter {
public:
    BiquadFilter() = default;

    void setCoefficients(const BiquadCoeffs& coeffs) noexcept {
        coeffs_ = coeffs;
    }

    void reset() noexcept {
        s1_ = 0.0;
        s2_ = 0.0;
    }

    [[nodiscard]] float process(float sample) noexcept {
        // Sanitize non-finite inputs (NaN / Inf protection)
        if (!std::isfinite(sample)) {
            reset();
            return 0.0f;
        }

        const double in = static_cast<double>(sample);
        const double out = coeffs_.b0 * in + s1_;
        s1_ = coeffs_.b1 * in - coeffs_.a1 * out + s2_;
        s2_ = coeffs_.b2 * in - coeffs_.a2 * out;

        // Catch non-finite filter divergence / instability
        if (!std::isfinite(out)) {
            reset();
            return 0.0f;
        }

        // Flush denormals
        if (std::abs(s1_) < 1e-25) {
            s1_ = 0.0;
        }
        if (std::abs(s2_) < 1e-25) {
            s2_ = 0.0;
        }

        return static_cast<float>(out);
    }

    [[nodiscard]] const BiquadCoeffs& coefficients() const noexcept {
        return coeffs_;
    }

private:
    BiquadCoeffs coeffs_{};
    double s1_ = 0.0;
    double s2_ = 0.0;
};

} // namespace analog_master::core
