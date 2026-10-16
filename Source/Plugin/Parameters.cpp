#include "Parameters.h"

namespace analog_master::params {

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::StringArray slopeChoices {
        "12 dB/oct",
        "24 dB/oct",
        "48 dB/oct",
        "64 dB/oct (Hard)",
        "96 dB/oct (Brickwall)"
    };

    // 1. Low Cut
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kLowCutFreq, 1 },
        "Low Cut Frequency",
        juce::NormalisableRange<float>(10.0f, 200.0f, 0.1f, 0.35f),
        20.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kLowCutSlope, 1 },
        "Low Cut Slope",
        slopeChoices,
        3 // Default to 64 dB/oct (Hard)
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kLowCutBypass, 1 },
        "Low Cut Bypass",
        false
    ));

    // 2. Low-End Stereo Field
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kStereoFreq, 1 },
        "Stereo Crossover",
        juce::NormalisableRange<float>(40.0f, 500.0f, 1.0f, 0.45f),
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kStereoWidth, 1 },
        "Low Stereo Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f),
        0.0f, // Default 0% = Mono low-end
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kStereoBypass, 1 },
        "Stereo Bypass",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kStereoSoloLow, 1 },
        "Solo Low Band",
        false
    ));

    // 3. High Cut
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kHighCutFreq, 1 },
        "High Cut Frequency",
        juce::NormalisableRange<float>(8000.0f, 24000.0f, 10.0f, 0.5f),
        20000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kHighCutSlope, 1 },
        "High Cut Slope",
        slopeChoices,
        3 // Default to 64 dB/oct (Hard)
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kHighCutBypass, 1 },
        "High Cut Bypass",
        false
    ));

    // 4. Master Output & Bypass
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kMasterGain, 1 },
        "Master Trim",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kMasterBypass, 1 },
        "Master Bypass",
        false
    ));

    // 5. Visual Faceplate Theme (4 Looks)
    const juce::StringArray themeChoices {
        "Classic Charcoal",
        "Silverface",
        "British Console",
        "Midnight Slate"
    };

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kTheme, 1 },
        "Faceplate Theme",
        themeChoices,
        0 // Default to Classic Charcoal
    ));

    return layout;
}

} // namespace analog_master::params
