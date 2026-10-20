#pragma once

#include "AnalogLookAndFeel.h"
#include "MasterProcessorCore.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace analog_master::ui {

// Twin VU meters above a stereo phase correlation bar.
// The needle shows the VU level against the reference. The readout shows the held sample peak in dBFS.
class VintageMeter : public juce::Component {
public:
    VintageMeter();

    // The peak hold counts calls at 30 Hz
    void setLevels(const core::MeterLevels& levels);
    void setThemePalette(const ThemePalette& palette);
    void setReferenceLevel(float dbfs);
    void paint(juce::Graphics& g) override;

private:
    struct ChannelState {
        float vuDb = -60.0f;
        float peakHoldDb = -60.0f;
        int peakHoldFrames = 0;
        float clipGlow = 0.0f;
    };

    static void updateChannel(ChannelState& state, float vu, float peak, bool clip);
    void paintChannel(juce::Graphics& g, juce::Rectangle<float> area, const ChannelState& state, const juce::String& name) const;
    void paintCorrelation(juce::Graphics& g, juce::Rectangle<float> area) const;

    ChannelState left_;
    ChannelState right_;
    float correlation_ = 1.0f;
    float referenceDbfs_ = -14.0f;
    ThemePalette palette_ { AnalogLookAndFeel::createPalette(Theme::CyberNeon) };
};

} // namespace analog_master::ui
