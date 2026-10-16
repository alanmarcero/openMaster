#include "VintageMeter.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace analog_master::ui {

VintageMeter::VintageMeter() {
    setOpaque(false);
}

void VintageMeter::setLevels(float levelL, float levelR) {
    // Ballistics: fast attack, smooth decay
    currentL_ = (levelL > currentL_) ? levelL : (currentL_ * 0.88f + levelL * 0.12f);
    currentR_ = (levelR > currentR_) ? levelR : (currentR_ * 0.88f + levelR * 0.12f);
    repaint();
}

static void drawSingleMeter(juce::Graphics& g, juce::Rectangle<float> area, float level, const juce::String& channelName) {
    // Meter frame
    g.setColour(juce::Colour(0xff121416));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(juce::Colour(0xff443f36));
    g.drawRoundedRectangle(area, 4.0f, 1.2f);

    // Warm backlit faceplate
    auto face = area.reduced(2.5f);
    juce::ColourGradient faceGrad(juce::Colour(0xffeeddbb), face.getCentreX(), face.getY(),
                                 juce::Colour(0xffcaa877), face.getCentreX(), face.getBottom(), false);
    faceGrad.addColour(0.8, juce::Colour(0xffdfc495));
    g.setGradientFill(faceGrad);
    g.fillRoundedRectangle(face, 2.5f);

    // Bezel shadow
    g.setColour(juce::Colour(0x33000000));
    g.drawRoundedRectangle(face, 2.5f, 1.5f);

    // Meter scale arc
    const float pivotX = face.getCentreX();
    const float pivotY = face.getBottom() + face.getHeight() * 0.45f;
    const float needleLen = face.getHeight() * 1.15f;

    const float minAngle = -0.65f;
    const float maxAngle = 0.65f;

    // Draw scale ticks: -20, -10, -7, -5, -3, -1, 0, +1, +3 dB
    const struct ScaleTick { float db; const char* label; } ticks[] = {
        { -20.0f, "20" },
        { -10.0f, "10" },
        { -5.0f, "5" },
        { 0.0f, "0" },
        { 3.0f, "+3" }
    };

    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));

    for (const auto& t : ticks) {
        // Map dB (-24 to +3) to [0, 1]
        float norm = std::clamp((t.db + 24.0f) / 27.0f, 0.0f, 1.0f);
        float a = minAngle + norm * (maxAngle - minAngle);

        float cosA = std::sin(a);
        float sinA = -std::cos(a);

        float r1 = needleLen * 0.76f;
        float r2 = needleLen * 0.88f;

        g.setColour((t.db >= 0.0f) ? juce::Colour(0xffaa2211) : juce::Colour(0xff222222));
        g.drawLine(pivotX + r1 * cosA, pivotY + r1 * sinA,
                   pivotX + r2 * cosA, pivotY + r2 * sinA, 1.0f);

        g.drawText(t.label,
                   static_cast<int>(pivotX + (r1 - 9.0f) * cosA - 10.0f),
                   static_cast<int>(pivotY + (r1 - 9.0f) * sinA - 5.0f),
                   20, 10,
                   juce::Justification::centred);
    }

    // VU header & channel label
    g.setColour(juce::Colour(0xff443322));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("VU", static_cast<int>(face.getX() + 4), static_cast<int>(face.getY() + 3), 20, 10, juce::Justification::left);
    g.drawText(channelName, static_cast<int>(face.getRight() - 20), static_cast<int>(face.getY() + 3), 16, 10, juce::Justification::right);

    // Needle position
    float levelDb = (level > 1e-5f) ? (20.0f * std::log10(level)) : -60.0f;
    float needleNorm = std::clamp((levelDb + 24.0f) / 27.0f, 0.0f, 1.08f);
    float needleAngle = minAngle + needleNorm * (maxAngle - minAngle);

    float nCos = std::sin(needleAngle);
    float nSin = -std::cos(needleAngle);

    // Needle shadow
    g.setColour(juce::Colour(0x44000000));
    g.drawLine(pivotX + 1.0f, pivotY + 1.0f,
               pivotX + 1.0f + needleLen * 0.90f * nCos,
               pivotY + 1.0f + needleLen * 0.90f * nSin, 1.8f);

    // Needle
    g.setColour((levelDb >= 0.0f) ? juce::Colour(0xffc22211) : juce::Colour(0xff181614));
    g.drawLine(pivotX, pivotY,
               pivotX + needleLen * 0.90f * nCos,
               pivotY + needleLen * 0.90f * nSin, 1.5f);

    // Pivot cap
    g.setColour(juce::Colour(0xff18191b));
    g.fillEllipse(pivotX - 7.0f, face.getBottom() - 7.0f, 14.0f, 14.0f);
    g.setColour(juce::Colour(0xff6a6254));
    g.drawEllipse(pivotX - 7.0f, face.getBottom() - 7.0f, 14.0f, 14.0f, 1.0f);
}

void VintageMeter::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    const float meterWidth = (b.getWidth() - 8.0f) * 0.5f;

    drawSingleMeter(g, juce::Rectangle<float>(b.getX(), b.getY(), meterWidth, b.getHeight()), currentL_, "L");
    drawSingleMeter(g, juce::Rectangle<float>(b.getX() + meterWidth + 8.0f, b.getY(), meterWidth, b.getHeight()), currentR_, "R");
}

} // namespace analog_master::ui
