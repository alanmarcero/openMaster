#pragma once

#include "AnalogLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace analog_master::ui {

class VintageMeter : public juce::Component {
public:
    VintageMeter();

    void setLevels(float levelL, float levelR);
    void setThemePalette(const ThemePalette& palette);
    void paint(juce::Graphics& g) override;

private:
    float currentL_ = 0.0f;
    float currentR_ = 0.0f;
    ThemePalette palette_ { AnalogLookAndFeel::createPalette(Theme::ClassicCharcoal) };
};

} // namespace analog_master::ui
