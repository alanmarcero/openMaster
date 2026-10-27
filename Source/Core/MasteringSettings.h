#pragma once
#include <array>
namespace analog_master::core {
struct ToneSettings {
    bool enabled = false;
    float lowDb = 0, highDb = 0, tiltDb = 0, lowHz = 100, highHz = 10000;
    bool operator==(const ToneSettings &) const = default;
};
struct CompressorSettings {
    bool enabled = false;
    float thresholdDb = -18, ratio = 2, attackMs = 30, releaseMs = 200;
    float kneeDb = 6, mix = 1, makeupDb = 0, sidechainHz = 80;
    bool operator==(const CompressorSettings &) const = default;
};
struct DynamicBandSettings {
    float frequency = 180, q = 1, thresholdDb = -24, rangeDb = 6;
    float attackMs = 10, releaseMs = 150;
    bool operator==(const DynamicBandSettings &) const = default;
};
struct DynamicEqSettings {
    bool enabled = false;
    float amount = 1;
    int soloBand = -1;
    std::array<DynamicBandSettings, 3> bands{
        {{180, 1, -24, 6, 10, 150}, {2500, 1, -24, 6, 10, 150}, {7000, 1, -24, 6, 10, 150}}};
    bool operator==(const DynamicEqSettings &) const = default;
};
struct SaturationSettings {
    bool enabled = false, autoGain = true;
    int character = 0;
    float driveDb = 6, mix = .5f, outputDb = 0;
    bool operator==(const SaturationSettings &) const = default;
};
struct ClipperSettings {
    bool enabled = false;
    int shape = 0;
    float driveDb = 0, ceilingDb = -1, mix = 1;
    bool operator==(const ClipperSettings &) const = default;
};
struct LimiterSettings {
    bool enabled = false;
    float driveDb = 0, ceilingDb = -1, releaseMs = 100;
    bool operator==(const LimiterSettings &) const = default;
};
} // namespace analog_master::core
