#pragma once

#include "ButterworthCoeffs.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace analog_master::params {

inline constexpr const char* kLowCutFreq = "low_cut_freq";
inline constexpr const char* kLowCutSlope = "low_cut_slope";
inline constexpr const char* kLowCutIn = "low_cut_in";

inline constexpr const char* kStereoFreq = "stereo_freq";
inline constexpr const char* kStereoWidth = "stereo_width";
inline constexpr const char* kStereoIn = "stereo_in";
inline constexpr const char* kStereoSoloLow = "stereo_solo_low";

inline constexpr const char* kHighCutFreq = "high_cut_freq";
inline constexpr const char* kHighCutSlope = "high_cut_slope";
inline constexpr const char* kHighCutIn = "high_cut_in";

inline constexpr const char* kMasterGain = "master_gain";
inline constexpr const char* kMasterPower = "master_power";

inline constexpr const char* kTheme = "theme";

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace analog_master::params
