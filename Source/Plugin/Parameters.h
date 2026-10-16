#pragma once

#include "ButterworthCoeffs.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace analog_master::params {

inline constexpr const char* kLowCutFreq = "low_cut_freq";
inline constexpr const char* kLowCutSlope = "low_cut_slope";
inline constexpr const char* kLowCutBypass = "low_cut_bypass";

inline constexpr const char* kStereoFreq = "stereo_freq";
inline constexpr const char* kStereoWidth = "stereo_width";
inline constexpr const char* kStereoBypass = "stereo_bypass";
inline constexpr const char* kStereoSoloLow = "stereo_solo_low";

inline constexpr const char* kHighCutFreq = "high_cut_freq";
inline constexpr const char* kHighCutSlope = "high_cut_slope";
inline constexpr const char* kHighCutBypass = "high_cut_bypass";

inline constexpr const char* kMasterGain = "master_gain";
inline constexpr const char* kMasterBypass = "master_bypass";

inline constexpr const char* kTheme = "theme";

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace analog_master::params
