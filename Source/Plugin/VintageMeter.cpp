#include "VintageMeter.h"
#include <algorithm>
#include <cmath>

namespace analog_master::ui {

namespace {

constexpr float kFloorDb = -60.0f;
constexpr float kScaleMinDb = -24.0f;
constexpr float kScaleMaxDb = 6.0f;
constexpr int kSegments = 30;
const juce::Colour kGreen {0xff90ee63}, kAmber {0xffffbd4a}, kRed {0xffff5544};
constexpr int kPeakHoldFrames = 30;     // 1 s at 30 Hz
constexpr float kPeakFallDbPerFrame = 0.7f;

float toDb(float linear) {
    return linear > 1e-6f ? std::max(kFloorDb, 20.0f * std::log10(linear)) : kFloorDb;
}

float meterPosition(float db) {
    return std::clamp((db - kScaleMinDb) / (kScaleMaxDb - kScaleMinDb), 0.0f, 1.0f);
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
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(palette_.meterScaleText);
    g.drawText(name, area.withWidth(12.0f).withHeight(14.0f), juce::Justification::centredLeft);

    const juce::Rectangle<float> track(area.getX() + 17.0f, area.getY() + 1.0f, area.getWidth() - 37.0f, 12.0f);
    const float pitch = track.getWidth() / kSegments;
    const float vu = state.vuDb - referenceDbfs_;
    const int peakSegment = std::clamp(static_cast<int>(std::ceil(meterPosition(state.peakHoldDb - referenceDbfs_) * kSegments)) - 1, 0, kSegments - 1);
    for (int segment = 0; segment < kSegments; ++segment) {
        const float threshold = kScaleMinDb + static_cast<float>(segment + 1);
        const auto colour = threshold > 0 ? kRed : threshold > -6 ? kAmber : kGreen;
        const bool lit = state.vuDb > kFloorDb && vu >= threshold;
        const juce::Rectangle<float> led(track.getX() + static_cast<float>(segment) * pitch, track.getY(), pitch - 1.5f, track.getHeight());
        if (lit) {
            g.setColour(colour.withAlpha(0.12f));
            g.fillRoundedRectangle(led.expanded(1.0f), 1.5f);
        }
        g.setColour(colour.withAlpha(lit ? 0.95f : 0.09f));
        g.fillRoundedRectangle(led, 0.6f);
        if (state.peakHoldDb > kFloorDb && state.peakHoldDb - referenceDbfs_ >= kScaleMinDb && segment == peakSegment) {
            g.setColour(colour.brighter(0.5f));
            g.drawRoundedRectangle(led, 0.6f, 0.8f);
        }
    }

    const juce::Rectangle<float> clip(area.getRight() - 12.0f, track.getY(), 12.0f, track.getHeight());
    g.setColour(kRed.withAlpha(state.clipGlow > 0.0f ? 0.3f + 0.7f * state.clipGlow : 0.09f));
    g.fillRoundedRectangle(clip, 1.0f);

    const auto readout = area.withTrimmedTop(20.0f).withHeight(16.0f).withTrimmedLeft(17.0f);
    g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
    g.setColour(palette_.meterScaleText);
    g.drawText("PEAK dBFS", readout.withWidth(55.0f), juce::Justification::centredLeft);
    const auto peakText = state.peakHoldDb <= -59.5f ? juce::String("----")
        : (state.peakHoldDb > 0.05f ? "+" : "") + juce::String(state.peakHoldDb, 1);
    drawLedNumber(g, readout.withTrimmedRight(1.0f), peakText, state.peakHoldDb >= 0.0f ? kRed : kGreen);
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

    constexpr int segments = 21;
    const float pitch = track.getWidth() / segments;
    const int position = juce::roundToInt((correlation_ + 1.0f) * 10.0f);
    for (int segment = 0; segment < segments; ++segment) {
        const bool lit = segment >= std::min(10, position) && segment <= std::max(10, position);
        const auto colour = segment < 10 ? kRed : segment == 10 ? kAmber : kGreen;
        g.setColour(colour.withAlpha(lit ? 0.85f : 0.09f));
        g.fillRect(track.getX() + static_cast<float>(segment) * pitch, track.getY(), pitch - 1.5f, track.getHeight());
    }
}

void VintageMeter::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    const auto correlation = bounds.removeFromBottom(16.0f);
    bounds.removeFromBottom(4.0f);
    g.setColour(palette_.meterFrameBg);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(palette_.meterFrameBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    const auto face = bounds.reduced(3.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff16211c), face.getTopLeft(), juce::Colour(0xff070c09), face.getBottomLeft(), false));
    g.fillRoundedRectangle(face, 2.0f);

    auto inner = bounds.reduced(8.0f, 6.0f);
    const auto scale = inner.removeFromTop(17.0f);
    const auto track = scale.withTrimmedLeft(17.0f).withTrimmedRight(20.0f);
    g.setFont(juce::FontOptions(6.5f, juce::Font::bold));
    g.setColour(palette_.meterScaleText);
    g.drawText("VU", scale.withWidth(13.0f), juce::Justification::centredLeft);
    for (const int db : {-24, -18, -12, -6, 0, 6}) {
        const float x = track.getX() + meterPosition(static_cast<float>(db)) * track.getWidth();
        g.setColour(db > 0 ? kRed : db == 0 ? kAmber : palette_.meterScaleText);
        g.drawText(db > 0 ? "+6" : juce::String(db), juce::Rectangle<float>(x - 10, scale.getY(), 20, 11), juce::Justification::centred);
    }
    g.setColour(kRed.withAlpha(0.75f));
    g.setFont(juce::FontOptions(5.5f, juce::Font::bold));
    g.drawText("CLIP", scale.withWidth(16.0f).withRightX(scale.getRight() + 2.0f).withHeight(11.0f), juce::Justification::centred);
    const float rowHeight = inner.getHeight() * 0.5f;
    paintChannel(g, inner.removeFromTop(rowHeight), left_, "L");
    paintChannel(g, inner, right_, "R");
    paintCorrelation(g, correlation);
}

} // namespace analog_master::ui
