#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace analog_master::ui {

class VintageMeter : public juce::Component {
public:
    VintageMeter();

    void setLevels(float levelL, float levelR);
    void paint(juce::Graphics& g) override;

private:
    float currentL_ = 0.0f;
    float currentR_ = 0.0f;
};

} // namespace analog_master::ui
