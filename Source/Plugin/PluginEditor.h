#pragma once

#include "AnalogLookAndFeel.h"
#include "PluginProcessor.h"
#include "VintageMeter.h"
#include <juce_gui_basics/juce_gui_basics.h>

class AnalogMasterEditor final : public juce::AudioProcessorEditor,
                                  private juce::Timer {
public:
    explicit AnalogMasterEditor(AnalogMasterProcessor& owner);
    ~AnalogMasterEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void applyTheme(int themeIndex);
    void paintModuleBay(juce::Graphics& g, int index, juce::Rectangle<float> bay, bool active) const;
    void paintFilterDisplay(juce::Graphics& g, juce::Rectangle<float> area, bool isHighPass, bool active) const;
    void paintStereoDisplay(juce::Graphics& g, juce::Rectangle<float> area, bool active) const;
    [[nodiscard]] juce::Rectangle<float> moduleBayBounds(int index) const;

    AnalogMasterProcessor& processor_;
    analog_master::ui::AnalogLookAndFeel lookAndFeel_;

    juce::Image logoImage_;
    juce::Image backdropImage_;

    juce::ComboBox themeBox_;
    juce::Label themeLabel_;

    // 1. Low Cut
    juce::Slider lowCutFreqSlider_;
    juce::ComboBox lowCutSlopeBox_;
    juce::ToggleButton lowCutBypassButton_;
    juce::Label lowCutFreqLabel_;
    juce::Label lowCutSlopeLabel_;

    // 2. Low-End Stereo Field
    juce::Slider stereoWidthSlider_;
    juce::Slider stereoFreqSlider_;
    juce::ToggleButton stereoBypassButton_;
    juce::ToggleButton stereoSoloButton_;
    juce::Label stereoWidthLabel_;
    juce::Label stereoFreqLabel_;

    // 3. High Cut
    juce::Slider highCutFreqSlider_;
    juce::ComboBox highCutSlopeBox_;
    juce::ToggleButton highCutBypassButton_;
    juce::Label highCutFreqLabel_;
    juce::Label highCutSlopeLabel_;

    // 4. Master Output & Meters
    analog_master::ui::VintageMeter meter_;
    juce::Slider masterGainSlider_;
    juce::ToggleButton masterPowerButton_;
    juce::Label masterGainLabel_;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboBoxAttachment> themeAttach_;

    std::unique_ptr<SliderAttachment> lowCutFreqAttach_;
    std::unique_ptr<ComboBoxAttachment> lowCutSlopeAttach_;
    std::unique_ptr<ButtonAttachment> lowCutBypassAttach_;

    std::unique_ptr<SliderAttachment> stereoFreqAttach_;
    std::unique_ptr<SliderAttachment> stereoWidthAttach_;
    std::unique_ptr<ButtonAttachment> stereoBypassAttach_;
    std::unique_ptr<ButtonAttachment> stereoSoloAttach_;

    std::unique_ptr<SliderAttachment> highCutFreqAttach_;
    std::unique_ptr<ComboBoxAttachment> highCutSlopeAttach_;
    std::unique_ptr<ButtonAttachment> highCutBypassAttach_;

    std::unique_ptr<SliderAttachment> masterGainAttach_;
    std::unique_ptr<ButtonAttachment> masterPowerAttach_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogMasterEditor)
};
