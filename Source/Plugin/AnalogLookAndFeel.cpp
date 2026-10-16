#include "AnalogLookAndFeel.h"
#include <cmath>
#include <numbers>

namespace analog_master::ui {

AnalogLookAndFeel::AnalogLookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe8dfd0));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x22000000));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x44665544));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff181a1d));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffeedcb3));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff524b42));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffdfb872));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff16171a));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffeedcb3));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff3a352a));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffffffff));
}

void AnalogLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& /*slider*/
) {
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(width), static_cast<float>(height)).reduced(6.0f);

    const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;

    // 1. Draw outer tick marks around the knob arc
    constexpr int numTicks = 11;
    for (int i = 0; i < numTicks; ++i) {
        const float tickAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (static_cast<float>(i) / (numTicks - 1));
        const float cosA = std::cos(tickAngle - std::numbers::pi_v<float> * 0.5f);
        const float sinA = std::sin(tickAngle - std::numbers::pi_v<float> * 0.5f);

        const float innerR = radius + 1.0f;
        const float outerR = radius + ((i == 0 || i == numTicks - 1 || i == 5) ? 5.5f : 3.5f);

        g.setColour(juce::Colour(0x99dfb872));
        g.drawLine(centreX + innerR * cosA, centreY + innerR * sinA,
                   centreX + outerR * cosA, centreY + outerR * sinA, 1.2f);
    }

    // 2. Drop shadow under knob
    g.setColour(juce::Colour(0x77000000));
    g.fillEllipse(rx + 2.0f, ry + 3.0f, rw, rw);

    // 3. Outer knurled metal collar
    juce::ColourGradient collarGrad(juce::Colour(0xff2b2f33), rx, ry,
                                   juce::Colour(0xff121416), rx + rw, ry + rw, false);
    collarGrad.addColour(0.5, juce::Colour(0xff454a50));
    g.setGradientFill(collarGrad);
    g.fillEllipse(rx, ry, rw, rw);

    g.setColour(juce::Colour(0xff60584c));
    g.drawEllipse(rx, ry, rw, rw, 1.2f);

    // 4. Inner machined brass / gold faceplate
    const float innerRadius = radius * 0.78f;
    const float irx = centreX - innerRadius;
    const float iry = centreY - innerRadius;
    const float irw = innerRadius * 2.0f;

    juce::ColourGradient brassGrad(juce::Colour(0xffc8a86b), irx, iry,
                                  juce::Colour(0xff4a3b22), irx + irw, iry + irw, false);
    brassGrad.addColour(0.3, juce::Colour(0xffdfc488));
    brassGrad.addColour(0.7, juce::Colour(0xff8a6f3e));
    g.setGradientFill(brassGrad);
    g.fillEllipse(irx, iry, irw, irw);

    // Subtle radial brushed lines
    g.setColour(juce::Colour(0x22ffffff));
    g.drawEllipse(irx + 2.0f, iry + 2.0f, irw - 4.0f, irw - 4.0f, 0.8f);

    // 5. Knob pointer line
    const float currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const float pCos = std::cos(currentAngle - std::numbers::pi_v<float> * 0.5f);
    const float pSin = std::sin(currentAngle - std::numbers::pi_v<float> * 0.5f);

    juce::Path pointer;
    const float pStart = innerRadius * 0.25f;
    const float pEnd = innerRadius * 0.92f;
    pointer.startNewSubPath(centreX + pStart * pCos, centreY + pStart * pSin);
    pointer.lineTo(centreX + pEnd * pCos, centreY + pEnd * pSin);

    // Glowing pointer shadow and line
    g.setColour(juce::Colour(0x99000000));
    g.strokePath(pointer, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(juce::Colour(0xfffff2d6));
    g.strokePath(pointer, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void AnalogLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    bool /*shouldDrawButtonAsHighlighted*/,
    bool /*shouldDrawButtonAsDown*/
) {
    auto bounds = button.getLocalBounds().toFloat();
    const float switchWidth = 24.0f;
    const float switchHeight = 36.0f;
    const float switchX = bounds.getX() + 4.0f;
    const float switchY = bounds.getCentreY() - switchHeight * 0.5f;

    // Switch bezel plate
    juce::Rectangle<float> switchPlate(switchX, switchY, switchWidth, switchHeight);
    g.setColour(juce::Colour(0xff181a1c));
    g.fillRoundedRectangle(switchPlate, 3.0f);
    g.setColour(juce::Colour(0xff4a453d));
    g.drawRoundedRectangle(switchPlate, 3.0f, 1.0f);

    // Switch slot
    juce::Rectangle<float> slot(switchPlate.getCentreX() - 3.0f, switchPlate.getY() + 4.0f, 6.0f, switchHeight - 8.0f);
    g.setColour(juce::Colour(0xff090a0b));
    g.fillRect(slot);

    // Toggle bat (lever)
    const bool isToggled = button.getToggleState();
    const float leverY = isToggled ? (slot.getY() + 3.0f) : (slot.getBottom() - 11.0f);
    juce::Rectangle<float> lever(switchPlate.getCentreX() - 6.0f, leverY, 12.0f, 8.0f);

    juce::ColourGradient leverGrad(juce::Colour(0xffdcd6cb), lever.getX(), lever.getY(),
                                  juce::Colour(0xff6a6458), lever.getRight(), lever.getBottom(), false);
    g.setGradientFill(leverGrad);
    g.fillRoundedRectangle(lever, 2.0f);
    g.setColour(juce::Colour(0xffffffff));
    g.drawRoundedRectangle(lever, 2.0f, 0.8f);

    // Jewel LED indicator
    const float ledSize = 10.0f;
    const float ledX = switchPlate.getRight() + 8.0f;
    const float ledY = bounds.getCentreY() - ledSize * 0.5f;
    drawVintageJewelLed(g, juce::Rectangle<float>(ledX, ledY, ledSize, ledSize), isToggled);

    // Text label
    g.setColour(isToggled ? juce::Colour(0xffeedcb3) : juce::Colour(0xff888277));
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText(button.getButtonText(),
               static_cast<int>(ledX + ledSize + 8.0f),
               0,
               static_cast<int>(bounds.getWidth() - (ledX + ledSize + 8.0f)),
               static_cast<int>(bounds.getHeight()),
               juce::Justification::centredLeft);
}

void AnalogLookAndFeel::drawVintageJewelLed(juce::Graphics& g, juce::Rectangle<float> bounds, bool isActive) {
    // Outer metal bezel
    g.setColour(juce::Colour(0xff3a3834));
    g.fillEllipse(bounds);
    g.setColour(juce::Colour(0xff6e675b));
    g.drawEllipse(bounds, 1.0f);

    // Inner glowing jewel
    auto inner = bounds.reduced(1.8f);
    if (isActive) {
        // Amber halo glow
        g.setColour(juce::Colour(0x44ff9900));
        g.fillEllipse(bounds.expanded(3.0f));

        juce::ColourGradient ledGrad(juce::Colour(0xffffefb8), inner.getCentreX(), inner.getCentreY(),
                                    juce::Colour(0xffff7700), inner.getX(), inner.getY(), true);
        g.setGradientFill(ledGrad);
        g.fillEllipse(inner);
    } else {
        juce::ColourGradient offGrad(juce::Colour(0xff442810), inner.getX(), inner.getY(),
                                    juce::Colour(0xff180e06), inner.getRight(), inner.getBottom(), false);
        g.setGradientFill(offGrad);
        g.fillEllipse(inner);
    }
}

void AnalogLookAndFeel::drawRackScrew(juce::Graphics& g, float centerX, float centerY, float radius) {
    const float x = centerX - radius;
    const float y = centerY - radius;
    const float d = radius * 2.0f;

    // Screw head shadow & outer circle
    g.setColour(juce::Colour(0x99000000));
    g.fillEllipse(x + 1.0f, y + 1.0f, d, d);

    juce::ColourGradient screwGrad(juce::Colour(0xff555a60), x, y,
                                  juce::Colour(0xff222528), x + d, y + d, false);
    g.setGradientFill(screwGrad);
    g.fillEllipse(x, y, d, d);

    g.setColour(juce::Colour(0xff777d85));
    g.drawEllipse(x, y, d, d, 0.8f);

    // Phillips slot
    g.setColour(juce::Colour(0xff111315));
    const float slotLen = radius * 0.65f;
    g.drawLine(centerX - slotLen, centerY, centerX + slotLen, centerY, 1.2f);
    g.drawLine(centerX, centerY - slotLen, centerX, centerY + slotLen, 1.2f);
}

void AnalogLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width, int height,
    bool /*isButtonDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& /*box*/
) {
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));

    // Vintage beveled dark inset
    g.setColour(juce::Colour(0xff131518));
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(juce::Colour(0xff50483c));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    // Vintage downward triangle arrow
    const float arrowX = static_cast<float>(width - 16);
    const float arrowY = static_cast<float>(height) * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowX, arrowY - 2.5f, arrowX + 8.0f, arrowY - 2.5f, arrowX + 4.0f, arrowY + 3.0f);
    g.setColour(juce::Colour(0xffdfb872));
    g.fillPath(arrow);
}

void AnalogLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
    g.setColour(juce::Colour(0xff16171a));
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff50483c));
    g.drawRect(bounds, 1.0f);
}

} // namespace analog_master::ui
