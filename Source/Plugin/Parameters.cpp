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
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction([](float val, int) {
                return val < 100.0f ? juce::String(val, 1) + " Hz" : juce::String(std::round(val)) + " Hz";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kLowCutSlope, 1 },
        "Low Cut Slope",
        slopeChoices,
        3 // Default to 64 dB/oct (Hard)
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kLowCutIn, 1 },
        "Low Cut Active",
        true // Engaged by default
    ));

    // 2. Low-End Stereo Field
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kStereoFreq, 1 },
        "Stereo Crossover",
        juce::NormalisableRange<float>(40.0f, 500.0f, 1.0f, 0.45f),
        120.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(std::round(val)) + " Hz";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kStereoWidth, 1 },
        "Low Stereo Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f),
        0.0f, // Default 0% = Mono low-end
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                if (val <= 0.5f) return juce::String("0% (Mono)");
                if (std::abs(val - 100.0f) < 0.5f) return juce::String("100% (Stereo)");
                if (val >= 199.5f) return juce::String("200% (Enhanced)");
                return juce::String(std::round(val)) + "%";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kStereoIn, 1 },
        "Stereo Field Active",
        true // Engaged by default
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
        juce::NormalisableRange<float>(8000.0f, 24000.0f, 10.0f, 1.0f),
        20000.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val / 1000.0f, 1) + " kHz";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kHighCutSlope, 1 },
        "High Cut Slope",
        slopeChoices,
        3 // Default to 64 dB/oct (Hard)
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kHighCutIn, 1 },
        "High Cut Active",
        true // Engaged by default
    ));

    // 4. Master Output & Power
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kMasterGain, 1 },
        "Master Trim",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([](float val, int) {
                return (val > 0.05f ? "+" : "") + juce::String(val, 1) + " dB";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kMasterPower, 1 },
        "Master Power",
        true // Power ON by default
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
