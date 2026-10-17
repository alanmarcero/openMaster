#include "VintageMeter.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace analog_master::ui {

VintageMeter::VintageMeter() {
    setOpaque(false);
}

void VintageMeter::setLevels(float levelL, float levelR) {
    setLevels(levelL, levelR, levelL >= 0.999f, levelR >= 0.999f);
}

void VintageMeter::setLevels(float vuL, float vuR, bool clipL, bool clipR) {
    targetL_ = vuL;
    targetR_ = vuR;

    constexpr float dt = 1.0f / 30.0f;
    // 2nd-order damped needle physics: natural frequency 11.5 rad/s, damping ratio ~0.75
    const float forceL = (targetL_ - needlePosL_) * 135.0f - needleVelL_ * 17.5f;
    needleVelL_ += forceL * dt;
    needlePosL_ += needleVelL_ * dt;
    needlePosL_ = std::clamp(needlePosL_, 0.0f, 1.6f);

    const float forceR = (targetR_ - needlePosR_) * 135.0f - needleVelR_ * 17.5f;
    needleVelR_ += forceR * dt;
    needlePosR_ += needleVelR_ * dt;
    needlePosR_ = std::clamp(needlePosR_, 0.0f, 1.6f);

    // Peak clip hold and decay (~600ms hold)
    clipAmountL_ = clipL ? 1.0f : std::max(0.0f, clipAmountL_ - 0.05f);
    clipAmountR_ = clipR ? 1.0f : std::max(0.0f, clipAmountR_ - 0.05f);

    repaint();
}

void VintageMeter::setThemePalette(const ThemePalette& palette) {
    palette_ = palette;
    repaint();
}

static void drawSingleMeter(juce::Graphics& g, juce::Rectangle<float> area, float level, float clipAmount, const juce::String& channelName, const ThemePalette& palette) {
    // 1. Cyber OLED Meter Bezel Frame
    g.setColour(palette.meterFrameBg);
    g.fillRoundedRectangle(area, 5.0f);
    g.setColour(palette.meterFrameBorder);
    g.drawRoundedRectangle(area, 5.0f, 1.2f);

    // 2. Dark Holographic OLED Screen Face
    auto face = area.reduced(2.5f);
    juce::ColourGradient faceGrad(palette.meterFaceTop, face.getCentreX(), face.getY(),
                                 palette.meterFaceBottom, face.getCentreX(), face.getBottom(), false);
    faceGrad.addColour(0.65, palette.meterFaceMid);
    g.setGradientFill(faceGrad);
    g.fillRoundedRectangle(face, 3.5f);

    // Subtle HUD horizontal scanlines inside meter screen
    g.setColour(palette.hudGrid.withAlpha(0.10f));
    for (float y = face.getY() + 3.0f; y < face.getBottom() - 2.0f; y += 3.0f) {
        g.drawHorizontalLine(static_cast<int>(y), face.getX() + 2.0f, face.getRight() - 2.0f);
    }

    g.setColour(palette.accentPrimary.withAlpha(0.32f));
    g.drawRoundedRectangle(face, 3.5f, 1.0f);

    // 3. Holographic Arc & Segmented Neon Bar along the top arc
    const float pivotX = face.getCentreX();
    const float pivotY = face.getBottom() + face.getHeight() * 0.45f;
    const float needleLen = face.getHeight() * 1.15f;

    const float minAngle = -0.65f;
    const float maxAngle = 0.65f;

    const float levelDb = (level > 1e-5f) ? (20.0f * std::log10(level)) : -60.0f;
    const float needleNorm = std::clamp((levelDb + 24.0f) / 27.0f, 0.0f, 1.08f);

    // Segmented neon arc blocks
    constexpr int kArcSegments = 20;
    for (int s = 0; s < kArcSegments; ++s) {
        const float segNorm = static_cast<float>(s) / static_cast<float>(kArcSegments - 1);
        const float a = minAngle + segNorm * (maxAngle - minAngle);
        const float cosA = std::sin(a);
        const float sinA = -std::cos(a);

        const float rInner = needleLen * 0.89f;
        const float rOuter = needleLen * 0.96f;
        const bool segLit = (segNorm <= needleNorm && needleNorm > 0.01f);
        const bool inRedZone = (segNorm >= (24.0f / 27.0f));

        juce::Colour segCol = inRedZone
            ? palette.meterRedZone
            : palette.accentPrimary.interpolatedWith(palette.accentSecondary, segNorm * 0.65f);

        g.setColour(segLit ? segCol : segCol.withAlpha(0.16f));
        g.drawLine(pivotX + rInner * cosA, pivotY + rInner * sinA,
                   pivotX + rOuter * cosA, pivotY + rOuter * sinA, 2.2f);
    }

    // 4. Scale Ticks: -20, -10, -5, 0, +3 dB
    const struct ScaleTick { float db; const char* label; } ticks[] = {
        { -20.0f, "-20" },
        { -10.0f, "-10" },
        { -5.0f, "-5" },
        { 0.0f, "0" },
        { 3.0f, "+3" }
    };

    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));

    for (const auto& t : ticks) {
        const float norm = std::clamp((t.db + 24.0f) / 27.0f, 0.0f, 1.0f);
        const float a = minAngle + norm * (maxAngle - minAngle);

        const float cosA = std::sin(a);
        const float sinA = -std::cos(a);

        const float r1 = needleLen * 0.75f;
        const float r2 = needleLen * 0.86f;

        g.setColour((t.db >= 0.0f) ? palette.meterRedZone : palette.meterScaleText);
        g.drawLine(pivotX + r1 * cosA, pivotY + r1 * sinA,
                   pivotX + r2 * cosA, pivotY + r2 * sinA, 1.1f);

        g.drawText(t.label,
                   static_cast<int>(pivotX + (r1 - 8.5f) * cosA - 11.0f),
                   static_cast<int>(pivotY + (r1 - 8.5f) * sinA - 5.0f),
                   22, 10,
                   juce::Justification::centred, false);
    }

    // 5. Cyber Channel Badge & Digital dB Telemetry Readout
    g.setColour(palette.accentPrimary);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("CH-" + channelName, static_cast<int>(face.getX() + 5), static_cast<int>(face.getY() + 3), 34, 11, juce::Justification::left);

    juce::String dbReadout = (levelDb <= -48.0f) ? "-INF dB" : juce::String(levelDb, 1) + " dB";
    g.setColour((levelDb >= 0.0f) ? palette.meterRedZone : palette.headerTitle.withAlpha(0.85f));
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawText(dbReadout, static_cast<int>(face.getX() + 6), static_cast<int>(face.getBottom() - 13), 48, 11, juce::Justification::left);

    // 6. Peak / Overload Indicator LED in top right
    const float peakLedSize = 6.0f;
    const float peakLedX = face.getRight() - 11.5f;
    const float peakLedY = face.getY() + 4.5f;
    juce::Rectangle<float> peakLedBounds(peakLedX, peakLedY, peakLedSize, peakLedSize);

    g.setColour(palette.meterScaleText.withAlpha(0.75f));
    g.setFont(juce::FontOptions(6.5f, juce::Font::bold));
    g.drawText("CLIP", static_cast<int>(peakLedX - 24.0f), static_cast<int>(peakLedY - 1.0f), 22, 8, juce::Justification::right);

    g.setColour(palette.jewelBezelBg);
    g.fillEllipse(peakLedBounds);
    g.setColour(palette.jewelBezelBorder);
    g.drawEllipse(peakLedBounds, 0.8f);

    if (clipAmount > 0.05f) {
        g.setColour(palette.meterRedZone.withAlpha(0.55f * clipAmount));
        g.fillEllipse(peakLedBounds.expanded(3.0f));
        g.setColour(palette.meterRedZone);
        g.fillEllipse(peakLedBounds.reduced(1.0f));
    } else {
        g.setColour(juce::Colour(0xff260812));
        g.fillEllipse(peakLedBounds.reduced(1.0f));
    }

    // 7. Laser Needle & Bloom Glow
    const float needleAngle = minAngle + needleNorm * (maxAngle - minAngle);
    const float nCos = std::sin(needleAngle);
    const float nSin = -std::cos(needleAngle);

    const float tipX = pivotX + needleLen * 0.91f * nCos;
    const float tipY = pivotY + needleLen * 0.91f * nSin;

    g.setColour((levelDb >= 0.0f) ? palette.meterRedZone.withAlpha(0.45f) : palette.meterNeedleGlow);
    g.drawLine(pivotX, pivotY, tipX, tipY, 3.8f);

    g.setColour((levelDb >= 0.0f) ? palette.meterRedZone : palette.meterNeedle);
    g.drawLine(pivotX, pivotY, tipX, tipY, 1.6f);

    g.setColour(palette.headerTitle);
    g.fillEllipse(tipX - 1.8f, tipY - 1.8f, 3.6f, 3.6f);

    // Pivot cap
    g.setColour(palette.meterFrameBg);
    g.fillEllipse(pivotX - 6.5f, face.getBottom() - 6.5f, 13.0f, 13.0f);
    g.setColour(palette.accentPrimary.withAlpha(0.6f));
    g.drawEllipse(pivotX - 6.5f, face.getBottom() - 6.5f, 13.0f, 13.0f, 1.0f);
}

void VintageMeter::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    const float meterWidth = (b.getWidth() - 8.0f) * 0.5f;

    drawSingleMeter(g, juce::Rectangle<float>(b.getX(), b.getY(), meterWidth, b.getHeight()), needlePosL_, clipAmountL_, "L", palette_);
    drawSingleMeter(g, juce::Rectangle<float>(b.getX() + meterWidth + 8.0f, b.getY(), meterWidth, b.getHeight()), needlePosR_, clipAmountR_, "R", palette_);
}

} // namespace analog_master::ui
