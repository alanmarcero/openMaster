#include "VintageMeter.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace analog_master::ui {

VintageMeter::VintageMeter() {
    setOpaque(false);
}

void VintageMeter::setLevels(float levelL, float levelR) {
    currentL_ = (levelL > currentL_) ? levelL : (currentL_ * 0.88f + levelL * 0.12f);
    currentR_ = (levelR > currentR_) ? levelR : (currentR_ * 0.88f + levelR * 0.12f);
    repaint();
}

void VintageMeter::setThemePalette(const ThemePalette& palette) {
    palette_ = palette;
    repaint();
}

static void drawSingleMeter(juce::Graphics& g, juce::Rectangle<float> area, float level, const juce::String& channelName, const ThemePalette& palette) {
    // Meter frame
    g.setColour(palette.meterFrameBg);
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(palette.meterFrameBorder);
    g.drawRoundedRectangle(area, 4.0f, 1.2f);

    // Warm backlit faceplate
    auto face = area.reduced(2.5f);
    juce::ColourGradient faceGrad(palette.meterFaceTop, face.getCentreX(), face.getY(),
                                 palette.meterFaceBottom, face.getCentreX(), face.getBottom(), false);
    faceGrad.addColour(0.8, palette.meterFaceMid);
    g.setGradientFill(faceGrad);
    g.fillRoundedRectangle(face, 2.5f);

    // Bezel shadow
    g.setColour(palette.meterFaceShadow);
    g.drawRoundedRectangle(face, 2.5f, 1.5f);

    // Meter scale arc
    const float pivotX = face.getCentreX();
    const float pivotY = face.getBottom() + face.getHeight() * 0.45f;
    const float needleLen = face.getHeight() * 1.15f;

    const float minAngle = -0.65f;
    const float maxAngle = 0.65f;

    // Draw scale ticks: -20, -10, -5, 0, +3 dB
    const struct ScaleTick { float db; const char* label; } ticks[] = {
        { -20.0f, "20" },
        { -10.0f, "10" },
        { -5.0f, "5" },
        { 0.0f, "0" },
        { 3.0f, "+3" }
    };

    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));

    for (const auto& t : ticks) {
        float norm = std::clamp((t.db + 24.0f) / 27.0f, 0.0f, 1.0f);
        float a = minAngle + norm * (maxAngle - minAngle);

        float cosA = std::sin(a);
        float sinA = -std::cos(a);

        float r1 = needleLen * 0.76f;
        float r2 = needleLen * 0.88f;

        g.setColour((t.db >= 0.0f) ? palette.meterRedZone : palette.meterScaleText);
        g.drawLine(pivotX + r1 * cosA, pivotY + r1 * sinA,
                   pivotX + r2 * cosA, pivotY + r2 * sinA, 1.0f);

        g.drawText(t.label,
                   static_cast<int>(pivotX + (r1 - 9.0f) * cosA - 10.0f),
                   static_cast<int>(pivotY + (r1 - 9.0f) * sinA - 5.0f),
                   20, 10,
                   juce::Justification::centred);
    }

    // VU header & channel label
    g.setColour(palette.meterScaleText.withAlpha(0.7f));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("VU", static_cast<int>(face.getX() + 4), static_cast<int>(face.getY() + 3), 20, 10, juce::Justification::left);
    g.drawText(channelName, static_cast<int>(face.getRight() - 20), static_cast<int>(face.getY() + 3), 16, 10, juce::Justification::right);

    // Needle position
    float levelDb = (level > 1e-5f) ? (20.0f * std::log10(level)) : -60.0f;
    float needleNorm = std::clamp((levelDb + 24.0f) / 27.0f, 0.0f, 1.08f);
    float needleAngle = minAngle + needleNorm * (maxAngle - minAngle);

    float nCos = std::sin(needleAngle);
    float nSin = -std::cos(needleAngle);

    // Needle glow / shadow
    g.setColour(palette.meterNeedleGlow);
    g.drawLine(pivotX, pivotY,
               pivotX + needleLen * 0.90f * nCos,
               pivotY + needleLen * 0.90f * nSin, 3.0f);

    // Needle line
    g.setColour((levelDb >= 0.0f) ? palette.meterRedZone : palette.meterNeedle);
    g.drawLine(pivotX, pivotY,
               pivotX + needleLen * 0.90f * nCos,
               pivotY + needleLen * 0.90f * nSin, 1.5f);

    // Pivot cap
    g.setColour(palette.meterFrameBg);
    g.fillEllipse(pivotX - 7.0f, face.getBottom() - 7.0f, 14.0f, 14.0f);
    g.setColour(palette.meterFrameBorder);
    g.drawEllipse(pivotX - 7.0f, face.getBottom() - 7.0f, 14.0f, 14.0f, 1.0f);
}

void VintageMeter::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    const float meterWidth = (b.getWidth() - 8.0f) * 0.5f;

    drawSingleMeter(g, juce::Rectangle<float>(b.getX(), b.getY(), meterWidth, b.getHeight()), currentL_, "L", palette_);
    drawSingleMeter(g, juce::Rectangle<float>(b.getX() + meterWidth + 8.0f, b.getY(), meterWidth, b.getHeight()), currentR_, "R", palette_);
}

} // namespace analog_master::ui
