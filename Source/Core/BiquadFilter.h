#pragma once

#include <cmath>

namespace analog_master::core {

struct BiquadCoeffs {
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
};

// Transposed Direct Form II Biquad Section
class BiquadFilter {
public:
    BiquadFilter() = default;

    void setCoefficients(const BiquadCoeffs& coeffs) noexcept {
        coeffs_ = coeffs;
    }

    void reset() noexcept {
        s1_ = 0.0f;
        s2_ = 0.0f;
    }

    [[nodiscard]] float process(float sample) noexcept {
        float out = coeffs_.b0 * sample + s1_;
        s1_ = coeffs_.b1 * sample - coeffs_.a1 * out + s2_;
        s2_ = coeffs_.b2 * sample - coeffs_.a2 * out;
        
        // Prevent denormals
        if (std::abs(s1_) < 1e-15f) s1_ = 0.0f;
        if (std::abs(s2_) < 1e-15f) s2_ = 0.0f;
        
        return out;
    }

    [[nodiscard]] const BiquadCoeffs& coefficients() const noexcept {
        return coeffs_;
    }

private:
    BiquadCoeffs coeffs_{};
    float s1_ = 0.0f;
    float s2_ = 0.0f;
};

} // namespace analog_master::core
