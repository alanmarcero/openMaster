#pragma once

#include "AnalogLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace analog_master::ui {

class VintageMeter : public juce::Component {
public:
    VintageMeter();

    void setLevels(float levelL, float levelR);
    void setLevels(float vuL, float vuR, bool clipL, bool clipR);
    void setThemePalette(const ThemePalette& palette);
    void paint(juce::Graphics& g) override;

private:
    float targetL_ = 0.0f;
    float targetR_ = 0.0f;
    float needlePosL_ = 0.0f;
    float needlePosR_ = 0.0f;
    float needleVelL_ = 0.0f;
    float needleVelR_ = 0.0f;
    float clipAmountL_ = 0.0f;
    float clipAmountR_ = 0.0f;
    ThemePalette palette_ { AnalogLookAndFeel::createPalette(Theme::CyberNeon) };
};

} // namespace analog_master::ui
