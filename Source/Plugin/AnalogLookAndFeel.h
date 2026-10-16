#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace analog_master::ui {

class AnalogLookAndFeel : public juce::LookAndFeel_V4 {
public:
    AnalogLookAndFeel();

    void drawRotarySlider(
        juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider
    ) override;

    void drawToggleButton(
        juce::Graphics& g,
        juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown
    ) override;

    void drawComboBox(
        juce::Graphics& g,
        int width, int height,
        bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox& box
    ) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    static void drawVintageJewelLed(juce::Graphics& g, juce::Rectangle<float> bounds, bool isActive);
    static void drawRackScrew(juce::Graphics& g, float centerX, float centerY, float radius);
};

} // namespace analog_master::ui
