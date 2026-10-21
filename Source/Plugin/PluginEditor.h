#pragma once
#include "AnalogLookAndFeel.h"
#include "PluginProcessor.h"
#include "VintageMeter.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class AnalogMasterEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit AnalogMasterEditor(AnalogMasterProcessor &);
    ~AnalogMasterEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Control {
        std::unique_ptr<juce::Component> widget;
        juce::Label label;
        std::unique_ptr<SliderAttachment> sliderAttachment;
        std::unique_ptr<ButtonAttachment> buttonAttachment;
        std::unique_ptr<ComboAttachment> comboAttachment;
        int section = -1;
    };
    Control &knob(std::vector<std::unique_ptr<Control>> &, const juce::String &id, const juce::String &title,
                  int section = -1);
    Control &toggle(std::vector<std::unique_ptr<Control>> &, const juce::String &id,
                    const juce::String &title, int section = -1);
    Control &choice(std::vector<std::unique_ptr<Control>> &, const juce::String &id,
                    const juce::String &title);
    void showDetails(int section);
    void timerCallback() override;
    void applyTheme();
    void place(Control &, juce::Rectangle<int>);
    juce::Rectangle<int> cardBounds(int index) const;
    AnalogMasterProcessor &processor_;
    analog_master::ui::AnalogLookAndFeel lookAndFeel_;
    juce::Image backdrop_, logo_;
    std::vector<std::unique_ptr<Control>> primary_, switches_, details_, header_;
    std::array<juce::TextButton, 6> sections_;
    juce::ComboBox detailMode_, dynamicBand_;
    juce::TextButton resetButton_{"RESET METERS"};
    analog_master::ui::VintageMeter meter_;
    juce::TooltipWindow tooltip_{this, 700};
    analog_master::core::MeterLevels levels_;
    int selected_ = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogMasterEditor)
};
