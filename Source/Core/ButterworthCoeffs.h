#pragma once

#include "BiquadFilter.h"
#include <vector>
#include <cstdint>

namespace analog_master::core {

enum class FilterSlope : uint8_t {
    Slope12dB = 0,
    Slope24dB,
    Slope48dB,
    Slope64dB,
    Slope96dB
};

[[nodiscard]] int slopeToOrder(FilterSlope slope) noexcept;
[[nodiscard]] const char* slopeToString(FilterSlope slope) noexcept;

[[nodiscard]] std::vector<BiquadCoeffs> designButterworthLowPass(
    double sampleRate,
    double cutoffHz,
    int order
) noexcept;

[[nodiscard]] std::vector<BiquadCoeffs> designButterworthHighPass(
    double sampleRate,
    double cutoffHz,
    int order
) noexcept;

} // namespace analog_master::core
