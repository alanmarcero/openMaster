#pragma once

#include "AnalogLookAndFeel.h"
#include "PluginProcessor.h"
#include "VintageMeter.h"
#include <juce_gui_basics/juce_gui_basics.h>

class AnalogMasterEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit AnalogMasterEditor(AnalogMasterProcessor& owner);
    ~AnalogMasterEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    AnalogMasterProcessor& processor_;
    analog_master::ui::AnalogLookAndFeel lookAndFeel_;

    // UI Controls
    // 1. Low Cut
    juce::Slider lowCutFreqSlider_;
    juce::ComboBox lowCutSlopeBox_;
    juce::ToggleButton lowCutBypassButton_{ "IN" };
    juce::Label lowCutFreqLabel_{ {}, "FREQ" };
    juce::Label lowCutSlopeLabel_{ {}, "SLOPE" };

    // 2. Low Stereo Field
    juce::Slider stereoFreqSlider_;
    juce::Slider stereoWidthSlider_;
    juce::ToggleButton stereoBypassButton_{ "IN" };
    juce::ToggleButton stereoSoloButton_{ "SOLO" };
    juce::Label stereoFreqLabel_{ {}, "X-OVER" };
    juce::Label stereoWidthLabel_{ {}, "WIDTH" };

    // 3. High Cut
    juce::Slider highCutFreqSlider_;
    juce::ComboBox highCutSlopeBox_;
    juce::ToggleButton highCutBypassButton_{ "IN" };
    juce::Label highCutFreqLabel_{ {}, "FREQ" };
    juce::Label highCutSlopeLabel_{ {}, "SLOPE" };

    // 4. Master Output & Meter
    juce::Slider masterGainSlider_;
    juce::ToggleButton masterBypassButton_{ "POWER" };
    juce::Label masterGainLabel_{ {}, "TRIM" };
    analog_master::ui::VintageMeter meter_;

    // APVTS Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

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
    std::unique_ptr<ButtonAttachment> masterBypassAttach_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogMasterEditor)
};
