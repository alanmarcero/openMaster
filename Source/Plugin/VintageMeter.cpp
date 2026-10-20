#include "VintageMeter.h"
#include <algorithm>
#include <cmath>

namespace analog_master::ui {

namespace {

constexpr float kFloorDb = -60.0f;
constexpr float kScaleMinDb = -20.0f;
constexpr float kScaleMaxDb = 3.0f;
constexpr float kArcHalfAngle = 0.55f; // radians either side of vertical
constexpr int kPeakHoldFrames = 30;     // 1 s at 30 Hz
constexpr float kPeakFallDbPerFrame = 0.7f;

float toDb(float linear) {
    return linear > 1e-6f ? std::max(kFloorDb, 20.0f * std::log10(linear)) : kFloorDb;
}

float dbToAngle(float db) {
    const float norm = std::clamp((db - kScaleMinDb) / (kScaleMaxDb - kScaleMinDb), 0.0f, 1.0f);
    return -kArcHalfAngle + norm * 2.0f * kArcHalfAngle;
}

juce::Point<float> polar(juce::Point<float> pivot, float radius, float angle) {
    return { pivot.x + radius * std::sin(angle), pivot.y - radius * std::cos(angle) };
}

juce::String formatDb(float db) {
    if (db <= -59.5f) {
        return "-inf";
    }
    return (db > 0.05f ? "+" : "") + juce::String(db, 1);
}

} // namespace

VintageMeter::VintageMeter() {
    setOpaque(false);
}

void VintageMeter::setLevels(const core::MeterLevels& levels) {
    updateChannel(left_, levels.vuL, levels.peakL, levels.clipL);
    updateChannel(right_, levels.vuR, levels.peakR, levels.clipR);
    correlation_ += 0.25f * (std::clamp(levels.correlation, -1.0f, 1.0f) - correlation_);
    repaint();
}

void VintageMeter::updateChannel(ChannelState& state, float vu, float peak, bool clip) {
    // The core applies the VU ballistics
    state.vuDb = toDb(vu);

    const float peakDb = toDb(peak);
    if (peakDb >= state.peakHoldDb) {
        state.peakHoldDb = peakDb;
        state.peakHoldFrames = kPeakHoldFrames;
    } else if (state.peakHoldFrames > 0) {
        --state.peakHoldFrames;
    } else {
        state.peakHoldDb = std::max(peakDb, state.peakHoldDb - kPeakFallDbPerFrame);
    }

    state.clipGlow = clip ? 1.0f : std::max(0.0f, state.clipGlow - 0.025f);
}

void VintageMeter::setReferenceLevel(float dbfs) {
    referenceDbfs_ = dbfs;
}

void VintageMeter::setThemePalette(const ThemePalette& palette) {
    palette_ = palette;
    repaint();
}

void VintageMeter::paintChannel(juce::Graphics& g, juce::Rectangle<float> area, const ChannelState& state, const juce::String& name) const {
    g.setColour(palette_.meterFrameBg);
    g.fillRoundedRectangle(area, 5.0f);
    g.setColour(palette_.meterFrameBorder);
    g.drawRoundedRectangle(area, 5.0f, 1.2f);

    auto face = area.reduced(3.0f);
    juce::ColourGradient faceGrad(palette_.meterFaceTop, face.getCentreX(), face.getY(),
                                  palette_.meterFaceBottom, face.getCentreX(), face.getBottom(), false);
    faceGrad.addColour(0.65, palette_.meterFaceMid);
    g.setGradientFill(faceGrad);
    g.fillRoundedRectangle(face, 3.5f);

    juce::Graphics::ScopedSaveState clipState(g);
    g.reduceClipRegion(face.toNearestInt());

    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(palette_.accentPrimary);
    g.drawText(name, face.withHeight(14.0f).withTrimmedLeft(5.0f), juce::Justification::centredLeft);

    const juce::Rectangle<float> led(face.getRight() - 11.0f, face.getY() + 4.0f, 6.0f, 6.0f);
    g.setColour(palette_.meterScaleText.withAlpha(0.8f));
    g.setFont(juce::FontOptions(6.5f, juce::Font::bold));
    g.drawText("CLIP", juce::Rectangle<float>(led.getX() - 25.0f, led.getY() - 2.0f, 22.0f, 10.0f), juce::Justification::centredRight);
    g.setColour(state.clipGlow > 0.0f ? palette_.meterRedZone.withAlpha(0.35f + 0.65f * state.clipGlow) : palette_.jewelBezelBg);
    g.fillEllipse(led);
    g.setColour(palette_.jewelBezelBorder);
    g.drawEllipse(led, 0.8f);

    const float readoutH = 12.0f;
    const juce::Point<float> pivot(face.getCentreX(), face.getBottom() - readoutH - 4.0f);
    const float maxRadiusByWidth = (face.getWidth() * 0.5f - 6.0f) / std::sin(kArcHalfAngle);
    const float maxRadiusByHeight = pivot.y - (face.getY() + 28.0f);
    const float radius = std::min(maxRadiusByWidth, maxRadiusByHeight);

    juce::Path scaleArc;
    scaleArc.addCentredArc(pivot.x, pivot.y, radius, radius, 0.0f, dbToAngle(kScaleMinDb), dbToAngle(0.0f), true);
    g.setColour(palette_.meterScaleText.withAlpha(0.7f));
    g.strokePath(scaleArc, juce::PathStrokeType(1.0f));

    juce::Path redArc;
    redArc.addCentredArc(pivot.x, pivot.y, radius, radius, 0.0f, dbToAngle(0.0f), dbToAngle(kScaleMaxDb), true);
    g.setColour(palette_.meterRedZone);
    g.strokePath(redArc, juce::PathStrokeType(2.5f));

    const struct { float db; const char* label; } ticks[] = {
        { -20.0f, "-20" }, { -10.0f, "-10" }, { -5.0f, "-5" }, { 0.0f, "0" }, { 3.0f, "" }
    };
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    for (const auto& t : ticks) {
        const float a = dbToAngle(t.db);
        g.setColour(t.db >= 0.0f ? palette_.meterRedZone : palette_.meterScaleText);
        g.drawLine(juce::Line<float>(polar(pivot, radius - 4.0f, a), polar(pivot, radius + 3.0f, a)), 1.2f);
        // Outside the arc, so the needle at rest does not cover the -20 label
        const auto labelArea = juce::Rectangle<float>(18.0f, 9.0f).withCentre(polar(pivot, radius + 11.0f, a)).constrainedWithin(face);
        g.drawText(t.label, labelArea, juce::Justification::centred);
    }

    const float vu = state.vuDb - referenceDbfs_;
    const auto tip = polar(pivot, radius + 4.0f, dbToAngle(vu));
    const bool needleHot = vu >= 0.0f;
    g.setColour(needleHot ? palette_.meterRedZone.withAlpha(0.45f) : palette_.meterNeedleGlow);
    g.drawLine(juce::Line<float>(pivot, tip), 3.5f);
    g.setColour(needleHot ? palette_.meterRedZone : palette_.meterNeedle);
    g.drawLine(juce::Line<float>(pivot, tip), 1.5f);

    const auto cap = juce::Rectangle<float>(9.0f, 9.0f).withCentre(pivot);
    g.setColour(palette_.meterFrameBg);
    g.fillEllipse(cap);
    g.setColour(palette_.accentPrimary.withAlpha(0.6f));
    g.drawEllipse(cap, 1.0f);

    const auto readout = face.removeFromBottom(readoutH).reduced(5.0f, 0.0f);
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.setColour(palette_.meterScaleText);
    g.drawText("VU " + (state.vuDb <= kFloorDb ? juce::String("-inf") : formatDb(vu)), readout, juce::Justification::centredLeft);
    g.setColour(state.peakHoldDb >= 0.0f ? palette_.meterRedZone : palette_.headerTitle.withAlpha(0.85f));
    g.drawText("PK " + formatDb(state.peakHoldDb), readout, juce::Justification::centredRight);
}

void VintageMeter::paintCorrelation(juce::Graphics& g, juce::Rectangle<float> area) const {
    g.setColour(palette_.meterFrameBg);
    g.fillRoundedRectangle(area, 3.5f);
    g.setColour(palette_.meterFrameBorder);
    g.drawRoundedRectangle(area, 3.5f, 1.0f);

    auto inner = area.reduced(4.0f, 2.0f);
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.setColour(palette_.meterRedZone.withAlpha(0.9f));
    g.drawText("-1", inner.removeFromLeft(14.0f), juce::Justification::centred);
    g.setColour(palette_.accentPrimary.withAlpha(0.9f));
    g.drawText("+1", inner.removeFromRight(14.0f), juce::Justification::centred);

    const auto track = inner.reduced(4.0f, 3.0f);
    g.setColour(palette_.meterFaceBottom);
    g.fillRoundedRectangle(track, 2.0f);

    const float midX = track.getCentreX();
    const float corrX = midX + correlation_ * track.getWidth() * 0.5f;
    g.setColour((correlation_ < 0.0f ? palette_.meterRedZone : palette_.accentPrimary).withAlpha(0.55f));
    g.fillRect(juce::Rectangle<float>(std::min(midX, corrX), track.getY() + 1.0f,
                                      std::max(1.0f, std::abs(corrX - midX)), track.getHeight() - 2.0f));

    g.setColour(palette_.meterScaleText.withAlpha(0.6f));
    g.drawVerticalLine(static_cast<int>(midX), track.getY(), track.getBottom());
    g.setColour(palette_.headerTitle);
    g.fillRoundedRectangle(corrX - 1.5f, track.getY() - 1.0f, 3.0f, track.getHeight() + 2.0f, 1.0f);
}

void VintageMeter::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    const auto corrArea = b.removeFromBottom(16.0f);
    b.removeFromBottom(4.0f);
    const float meterWidth = (b.getWidth() - 6.0f) * 0.5f;

    paintChannel(g, b.removeFromLeft(meterWidth), left_, "L");
    paintChannel(g, b.removeFromRight(meterWidth), right_, "R");
    paintCorrelation(g, corrArea);
}

} // namespace analog_master::ui
