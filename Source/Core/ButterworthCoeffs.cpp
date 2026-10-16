#include "ButterworthCoeffs.h"
#include <algorithm>
#include <numbers>

namespace analog_master::core {

int slopeToOrder(FilterSlope slope) noexcept {
    switch (slope) {
        case FilterSlope::Slope12dB: return 2;
        case FilterSlope::Slope24dB: return 4;
        case FilterSlope::Slope48dB: return 8;
        case FilterSlope::Slope64dB: return 12;
        case FilterSlope::Slope96dB: return 16;
    }
    return 4;
}

const char* slopeToString(FilterSlope slope) noexcept {
    switch (slope) {
        case FilterSlope::Slope12dB: return "12 dB/oct";
        case FilterSlope::Slope24dB: return "24 dB/oct";
        case FilterSlope::Slope48dB: return "48 dB/oct";
        case FilterSlope::Slope64dB: return "64 dB/oct";
        case FilterSlope::Slope96dB: return "96 dB/oct";
    }
    return "24 dB/oct";
}

std::vector<BiquadCoeffs> designButterworthLowPass(
    double sampleRate,
    double cutoffHz,
    int order
) noexcept {
    if (sampleRate <= 0.0) {
        return {};
    }

    const int safeOrder = std::clamp((order / 2) * 2, 2, 16);
    const double maxCutoff = sampleRate * 0.49;
    const double safeCutoff = std::clamp(cutoffHz, 10.0, maxCutoff);

    const double K = std::tan(std::numbers::pi * safeCutoff / sampleRate);
    const double K2 = K * K;
    const int numSections = safeOrder / 2;

    std::vector<BiquadCoeffs> sections;
    sections.reserve(numSections);

    for (int k = 0; k < numSections; ++k) {
        const double theta = (2.0 * k + 1.0) * std::numbers::pi / (2.0 * safeOrder);
        const double Q = 1.0 / (2.0 * std::cos(theta));

        const double norm = 1.0 / (1.0 + (K / Q) + K2);
        const float b0 = static_cast<float>(K2 * norm);
        const float b1 = static_cast<float>(2.0 * b0);
        const float b2 = b0;
        const float a1 = static_cast<float>(2.0 * (K2 - 1.0) * norm);
        const float a2 = static_cast<float>((1.0 - (K / Q) + K2) * norm);

        sections.push_back({ b0, b1, b2, a1, a2 });
    }

    return sections;
}

std::vector<BiquadCoeffs> designButterworthHighPass(
    double sampleRate,
    double cutoffHz,
    int order
) noexcept {
    if (sampleRate <= 0.0) {
        return {};
    }

    const int safeOrder = std::clamp((order / 2) * 2, 2, 16);
    const double maxCutoff = sampleRate * 0.49;
    const double safeCutoff = std::clamp(cutoffHz, 5.0, maxCutoff);

    const double K = std::tan(std::numbers::pi * safeCutoff / sampleRate);
    const double K2 = K * K;
    const int numSections = safeOrder / 2;

    std::vector<BiquadCoeffs> sections;
    sections.reserve(numSections);

    for (int k = 0; k < numSections; ++k) {
        const double theta = (2.0 * k + 1.0) * std::numbers::pi / (2.0 * safeOrder);
        const double Q = 1.0 / (2.0 * std::cos(theta));

        const double norm = 1.0 / (1.0 + (K / Q) + K2);
        const float b0 = static_cast<float>(norm);
        const float b1 = static_cast<float>(-2.0 * norm);
        const float b2 = static_cast<float>(norm);
        const float a1 = static_cast<float>(2.0 * (K2 - 1.0) * norm);
        const float a2 = static_cast<float>((1.0 - (K / Q) + K2) * norm);

        sections.push_back({ b0, b1, b2, a1, a2 });
    }

    return sections;
}

} // namespace analog_master::core
