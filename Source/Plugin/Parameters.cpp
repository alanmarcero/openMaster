#include "Parameters.h"

namespace analog_master::params {

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::StringArray slopeChoices {
        "12 dB/oct",
        "24 dB/oct",
        "48 dB/oct",
        "72 dB/oct (Hard)",
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
        3 // Default to 72 dB/oct (Hard)
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
            .withValueFromStringFunction([](const juce::String& text) {
                const auto value = text.getFloatValue();
                return text.containsIgnoreCase("k") ? value * 1000.0f : value;
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kHighCutSlope, 1 },
        "High Cut Slope",
        slopeChoices,
        3 // Default to 72 dB/oct (Hard)
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

    // Theme indices stay stable for saved sessions.
    const juce::StringArray themeChoices {
        "Studio 84",
        "Silver",
        "Phosphor",
        "Ice",
        "Amber"
    };

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kTheme, 1 },
        "Faceplate Theme",
        themeChoices,
        0 // Default to Studio 84
    ));

    juce::StringArray meterReferenceChoices;
    for (const float dbfs : kMeterReferenceDbfs) {
        meterReferenceChoices.add("0 VU = " + juce::String(juce::roundToInt(dbfs)) + " dBFS");
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kMeterReference, 1 },
        "VU Reference",
        meterReferenceChoices,
        3 // -14 dBFS suits mastered material; -18 dBFS is the EBU R68 tracking level
    ));

    // Append only: the original parameter order is part of session compatibility.
    auto number = [&layout](const char* id, const char* name, float lo, float hi,
                            float initial, const char* unit = "dB", float skew = 1.f) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(lo, hi, .01f, skew), initial,
            juce::AudioParameterFloatAttributes().withLabel(unit)
                .withStringFromValueFunction([unit](float v, int) { return juce::String(v, 1) + " " + unit; })
                .withValueFromStringFunction([unit](const juce::String& t) {
                    return t.getFloatValue() * (juce::String(unit) == "Hz" && t.containsIgnoreCase("k") ? 1000.f : 1.f);
                })));
    };
    auto toggle = [&layout](const char* id, const char* name, bool initial = false) {
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, name, initial));
    };
    auto choice = [&layout](const char* id, const char* name, juce::StringArray values) {
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, name, values, 0));
    };
    toggle("tone_in", "Tone Active");
    number("tone_low", "Low Shelf Gain", -9, 9, 0);
    number("tone_high", "High Shelf Gain", -9, 9, 0);
    number("tone_tilt", "Tone Tilt", -6, 6, 0);
    number("tone_low_hz", "Low Shelf Frequency", 40, 400, 100, "Hz", .5f);
    number("tone_high_hz", "High Shelf Frequency", 2000, 16000, 10000, "Hz", .5f);
    toggle("comp_in", "Compressor Active");
    number("comp_threshold", "Compressor Threshold", -48, 0, -18);
    number("comp_ratio", "Compressor Ratio", 1, 10, 2, ":1", .5f);
    number("comp_attack", "Compressor Attack", .1f, 100, 30, "ms", .5f);
    number("comp_release", "Compressor Release", 20, 1000, 200, "ms", .5f);
    number("comp_knee", "Compressor Knee", 0, 18, 6);
    number("comp_mix", "Compressor Mix", 0, 100, 100, "%");
    number("comp_makeup", "Compressor Makeup", -12, 12, 0);
    number("comp_sc_hz", "Compressor Sidechain HP", 20, 300, 80, "Hz", .5f);
    toggle("dyn_in", "Dynamic EQ Active");
    number("dyn_amount", "Dynamic EQ Amount", 0, 100, 100, "%");
    choice("dyn_solo", "Dynamic EQ Audition", {"Off", "Low", "Mid", "High"});
    for (int i = 0; i < 3; ++i) {
        const auto prefix = "dyn_" + juce::String(i) + "_";
        const auto name = "Dynamic Band " + juce::String(i + 1) + " ";
        number((prefix + "freq").toRawUTF8(), (name + "Frequency").toRawUTF8(), 30, 16000,
               std::array<float, 3>{180, 2500, 7000}[static_cast<size_t>(i)], "Hz", .25f);
        number((prefix + "q").toRawUTF8(), (name + "Q").toRawUTF8(), .3f, 6, 1, "Q", .5f);
        number((prefix + "threshold").toRawUTF8(), (name + "Threshold").toRawUTF8(), -60, 0, -24);
        number((prefix + "range").toRawUTF8(), (name + "Range").toRawUTF8(), 0, 12, 6);
        number((prefix + "attack").toRawUTF8(), (name + "Attack").toRawUTF8(), .1f, 100, 10, "ms", .5f);
        number((prefix + "release").toRawUTF8(), (name + "Release").toRawUTF8(), 20, 1000, 150, "ms", .5f);
    }
    toggle("sat_in", "Saturation Active");
    number("sat_drive", "Saturation Drive", 0, 24, 6);
    choice("sat_character", "Saturation Character", {"Tape", "Transformer"});
    number("sat_mix", "Saturation Mix", 0, 100, 50, "%");
    number("sat_output", "Saturation Output", -18, 6, 0);
    toggle("sat_auto", "Saturation Gain Compensation", true);
    toggle("clip_in", "Clipper Active");
    number("clip_drive", "Clipper Drive", 0, 24, 0);
    choice("clip_shape", "Clipper Shape", {"Soft", "Hard"});
    number("clip_ceiling", "Clipper Ceiling", -12, 0, -1);
    number("clip_mix", "Clipper Mix", 0, 100, 100, "%");
    toggle("limit_in", "Limiter Active");
    number("limit_drive", "Limiter Drive", 0, 24, 0);
    number("limit_ceiling", "Limiter Ceiling", -12, 0, -1);
    number("limit_release", "Limiter Release", 50, 1000, 100, "ms", .5f);
    toggle("match_bypass", "Loudness Matched Bypass");

    return layout;
}

} // namespace analog_master::params
