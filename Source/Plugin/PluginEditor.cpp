#include "PluginEditor.h"
#include "BinaryData.h"
#include "ButterworthCoeffs.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

namespace {

constexpr int kEditorWidth = 1040;
constexpr int kEditorHeight = 540;
constexpr float kBayTop = 70.0f;
constexpr float kBayMargin = 32.0f;
constexpr float kBaySpacing = 12.0f;
constexpr float kBayBottomMargin = 22.0f;

void setScale(juce::Slider& slider, std::initializer_list<double> values, std::initializer_list<const char*> labels) {
    juce::Array<juce::var> valueArray;
    juce::Array<juce::var> labelArray;
    for (const double v : values) {
        valueArray.add(v);
    }
    for (const char* l : labels) {
        labelArray.add(juce::String(l));
    }
    slider.getProperties().set("scaleValues", valueArray);
    slider.getProperties().set("scaleLabels", labelArray);
}

void setStateText(juce::ToggleButton& button, const juce::String& onText, const juce::String& offText) {
    button.getProperties().set("onText", onText);
    button.getProperties().set("offText", offText);
}

// Uses the same filter design as the DSP core
double filterResponseDb(const analog_master::core::FilterDesign& design, double freqHz, double sampleRate) {
    const std::complex<double> zInv = std::polar(1.0, -2.0 * std::numbers::pi * freqHz / sampleRate);
    std::complex<double> response = 1.0;
    for (const auto& s : design) {
        response *= (s.b0 + zInv * (s.b1 + zInv * s.b2)) / (1.0 + zInv * (s.a1 + zInv * s.a2));
    }
    return 20.0 * std::log10(std::max(std::abs(response), 1e-9));
}

} // namespace

AnalogMasterEditor::AnalogMasterEditor(AnalogMasterProcessor& owner)
    : AudioProcessorEditor(&owner), processor_(owner) {
    using namespace analog_master::params;
    setLookAndFeel(&lookAndFeel_);

    logoImage_ = juce::ImageCache::getFromMemory(BinaryData::alan_marcero_logo_jpg, BinaryData::alan_marcero_logo_jpgSize);
    backdropImage_ = juce::ImageCache::getFromMemory(BinaryData::cyber_chassis_backdrop_jpg, BinaryData::cyber_chassis_backdrop_jpgSize);

    auto setupLabel = [this](juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        addAndMakeVisible(label);
    };

    // The parameter strings include the unit
    auto setupKnob = [this, &setupLabel](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 96, 18);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible(slider);
        setupLabel(label, text);
    };

    auto setupSlopeBox = [this, &setupLabel](juce::ComboBox& box, juce::Label& label, const char* paramId) {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(processor_.parameters.getParameter(paramId))) {
            box.addItemList(choice->choices, 1);
        }
        box.setJustificationType(juce::Justification::centred);
        box.onChange = [this] { repaint(); };
        addAndMakeVisible(box);
        setupLabel(label, "SLOPE");
    };

    auto setupButton = [this](juce::ToggleButton& button, const juce::String& onText, const juce::String& offText) {
        setStateText(button, onText, offText);
        button.onClick = [this] { repaint(); };
        addAndMakeVisible(button);
    };

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(processor_.parameters.getParameter(kTheme))) {
        themeBox_.addItemList(choice->choices, 1);
    }
    themeBox_.setJustificationType(juce::Justification::centred);
    themeBox_.onChange = [this] { applyTheme(themeBox_.getSelectedItemIndex()); };
    addAndMakeVisible(themeBox_);
    setupLabel(themeLabel_, "THEME");
    themeLabel_.setJustificationType(juce::Justification::centredRight);

    // 1. Low Cut
    setupKnob(lowCutFreqSlider_, lowCutFreqLabel_, "FREQUENCY");
    setScale(lowCutFreqSlider_, { 10.0, 20.0, 40.0, 80.0, 200.0 }, { "10", "20", "40", "80", "200" });
    setupSlopeBox(lowCutSlopeBox_, lowCutSlopeLabel_, kLowCutSlope);
    setupButton(lowCutBypassButton_, "ACTIVE", "BYPASSED");

    // 2. Low-End Stereo Field
    setupKnob(stereoWidthSlider_, stereoWidthLabel_, "WIDTH");
    setScale(stereoWidthSlider_, { 0.0, 50.0, 100.0, 150.0, 200.0 }, { "MONO", "50", "100", "150", "200" });
    setupKnob(stereoFreqSlider_, stereoFreqLabel_, "CROSSOVER");
    setScale(stereoFreqSlider_, { 40.0, 80.0, 120.0, 250.0, 500.0 }, { "40", "80", "120", "250", "500" });
    setupButton(stereoBypassButton_, "ACTIVE", "BYPASSED");
    setupButton(stereoSoloButton_, "LOWS ONLY", "FULL MIX");
    stereoSoloButton_.getProperties().set("useSecondaryAccent", true);

    // 3. High Cut
    setupKnob(highCutFreqSlider_, highCutFreqLabel_, "FREQUENCY");
    setScale(highCutFreqSlider_, { 8000.0, 12000.0, 15000.0, 20000.0, 24000.0 }, { "8k", "12k", "15k", "20k", "24k" });
    setupSlopeBox(highCutSlopeBox_, highCutSlopeLabel_, kHighCutSlope);
    setupButton(highCutBypassButton_, "ACTIVE", "BYPASSED");

    // 4. Master Output
    addAndMakeVisible(meter_);
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(processor_.parameters.getParameter(kMeterReference))) {
        meterReferenceBox_.addItemList(choice->choices, 1);
    }
    meterReferenceBox_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(meterReferenceBox_);
    setupKnob(masterGainSlider_, masterGainLabel_, "OUTPUT TRIM");
    setScale(masterGainSlider_, { -18.0, -9.0, 0.0, 9.0, 18.0 }, { "-18", "-9", "0", "+9", "+18" });
    setupButton(masterPowerButton_, "POWER ON", "BYPASSED");
    masterPowerButton_.getProperties().set("useSecondaryAccent", true);

    themeAttach_ = std::make_unique<ComboBoxAttachment>(processor_.parameters, kTheme, themeBox_);
    meterReferenceAttach_ = std::make_unique<ComboBoxAttachment>(processor_.parameters, kMeterReference, meterReferenceBox_);

    lowCutFreqAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kLowCutFreq, lowCutFreqSlider_);
    lowCutSlopeAttach_ = std::make_unique<ComboBoxAttachment>(processor_.parameters, kLowCutSlope, lowCutSlopeBox_);
    lowCutBypassAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kLowCutIn, lowCutBypassButton_);

    stereoFreqAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kStereoFreq, stereoFreqSlider_);
    stereoWidthAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kStereoWidth, stereoWidthSlider_);
    stereoBypassAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kStereoIn, stereoBypassButton_);
    stereoSoloAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kStereoSoloLow, stereoSoloButton_);

    highCutFreqAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kHighCutFreq, highCutFreqSlider_);
    highCutSlopeAttach_ = std::make_unique<ComboBoxAttachment>(processor_.parameters, kHighCutSlope, highCutSlopeBox_);
    highCutBypassAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kHighCutIn, highCutBypassButton_);

    masterGainAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kMasterGain, masterGainSlider_);
    masterPowerAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kMasterPower, masterPowerButton_);

    // Double-click a knob to return it to its default
    lowCutFreqSlider_.setDoubleClickReturnValue(true, 20.0);
    stereoWidthSlider_.setDoubleClickReturnValue(true, 0.0);
    stereoFreqSlider_.setDoubleClickReturnValue(true, 120.0);
    highCutFreqSlider_.setDoubleClickReturnValue(true, 20000.0);
    masterGainSlider_.setDoubleClickReturnValue(true, 0.0);

    applyTheme(themeBox_.getSelectedItemIndex());

    setSize(kEditorWidth, kEditorHeight);
    startTimerHz(30);
}

AnalogMasterEditor::~AnalogMasterEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void AnalogMasterEditor::applyTheme(int themeIndex) {
    lookAndFeel_.setTheme(static_cast<analog_master::ui::Theme>(std::clamp(themeIndex, 0, 4)));
    const auto& pal = lookAndFeel_.getPalette();
    meter_.setThemePalette(pal);

    for (auto* label : { &lowCutFreqLabel_, &lowCutSlopeLabel_, &stereoWidthLabel_, &stereoFreqLabel_,
                         &highCutFreqLabel_, &highCutSlopeLabel_, &masterGainLabel_ }) {
        label->setColour(juce::Label::textColourId, pal.labelText);
    }
    themeLabel_.setColour(juce::Label::textColourId, pal.accentPrimary);

    sendLookAndFeelChange();
    repaint();
}

void AnalogMasterEditor::timerCallback() {
    const auto referenceIndex = static_cast<size_t>(processor_.parameters.getRawParameterValue(analog_master::params::kMeterReference)->load());
    meter_.setReferenceLevel(analog_master::params::kMeterReferenceDbfs[referenceIndex]);
    meter_.setLevels(processor_.takeMeterLevels());
}

juce::Rectangle<float> AnalogMasterEditor::moduleBayBounds(int index) const {
    const float bayW = (static_cast<float>(getWidth()) - kBayMargin * 2.0f - kBaySpacing * 3.0f) * 0.25f;
    const float bayH = static_cast<float>(getHeight()) - kBayTop - kBayBottomMargin;
    return { kBayMargin + static_cast<float>(index) * (bayW + kBaySpacing), kBayTop, bayW, bayH };
}

void AnalogMasterEditor::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    const auto& pal = lookAndFeel_.getPalette();

    juce::ColourGradient panelGrad(pal.panelGradTop, 0.0f, 0.0f, pal.panelGradBottom, 0.0f, bounds.getHeight(), false);
    panelGrad.addColour(0.32, pal.panelGradMid1);
    panelGrad.addColour(0.72, pal.panelGradMid2);
    g.setGradientFill(panelGrad);
    g.fillRect(bounds);

    if (backdropImage_.isValid()) {
        g.setOpacity(0.32f);
        g.drawImage(backdropImage_, bounds, juce::RectanglePlacement::stretchToFit);
        g.setOpacity(1.0f);
    }

    g.setColour(pal.panelStripe);
    for (int y = 0; y < getHeight(); y += 3) {
        g.drawHorizontalLine(y, 0.0f, bounds.getWidth());
    }

    g.setColour(pal.panelBorder);
    g.drawRect(bounds, 2.0f);
    g.setColour(pal.panelInnerBorder);
    g.drawRect(bounds.reduced(3.0f), 1.2f);

    lookAndFeel_.drawRackScrew(g, 18.0f, 18.0f, 7.5f);
    lookAndFeel_.drawRackScrew(g, bounds.getRight() - 18.0f, 18.0f, 7.5f);
    lookAndFeel_.drawRackScrew(g, 18.0f, bounds.getBottom() - 18.0f, 7.5f);
    lookAndFeel_.drawRackScrew(g, bounds.getRight() - 18.0f, bounds.getBottom() - 18.0f, 7.5f);

    const juce::Rectangle<float> logoRect(36.0f, 9.0f, 44.0f, 44.0f);
    if (logoImage_.isValid()) {
        juce::Graphics::ScopedSaveState save(g);
        juce::Path circle;
        circle.addEllipse(logoRect);
        g.reduceClipRegion(circle);
        g.drawImage(logoImage_, logoRect, juce::RectanglePlacement::centred);
    }
    g.setColour(pal.accentPrimary.withAlpha(0.8f));
    g.drawEllipse(logoRect.reduced(0.5f), 1.5f);

    g.setColour(pal.headerBrand);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("ALAN MARCERO", juce::Rectangle<float>(88.0f, 20.0f, 160.0f, 20.0f), juce::Justification::centredLeft);

    const juce::Rectangle<float> titlePlate(bounds.getCentreX() - 210.0f, 8.0f, 420.0f, 44.0f);
    g.setColour(pal.sectionWellBg);
    g.fillRoundedRectangle(titlePlate, 6.0f);
    g.setColour(pal.accentPrimary.withAlpha(0.55f));
    g.drawRoundedRectangle(titlePlate, 6.0f, 1.3f);

    g.setColour(pal.headerTitle);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("openMaster", titlePlate.withHeight(26.0f).translated(0.0f, 2.0f), juce::Justification::centred);
    g.setColour(pal.accentPrimary);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("LOW CUT  /  LOW-END STEREO  /  HIGH CUT", titlePlate.withTrimmedTop(27.0f).withHeight(13.0f), juce::Justification::centred);

    g.setColour(pal.headerDivider);
    g.drawLine(32.0f, 60.0f, bounds.getRight() - 32.0f, 60.0f, 1.2f);

    const bool powerOn = masterPowerButton_.getToggleState();
    const bool bayActive[4] = {
        powerOn && lowCutBypassButton_.getToggleState(),
        powerOn && stereoBypassButton_.getToggleState(),
        powerOn && highCutBypassButton_.getToggleState(),
        powerOn
    };
    for (int i = 0; i < 4; ++i) {
        paintModuleBay(g, i, moduleBayBounds(i), bayActive[i]);
    }

    g.setColour(pal.headerSub.withAlpha(0.6f));
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.drawText("v1.1.0", juce::Rectangle<float>(bounds.getWidth() - 180.0f, bounds.getBottom() - 20.0f, 150.0f, 16.0f), juce::Justification::centredRight);
}

void AnalogMasterEditor::paintModuleBay(juce::Graphics& g, int index, juce::Rectangle<float> bay, bool active) const {
    const auto& pal = lookAndFeel_.getPalette();
    const juce::Colour accent = index == 3 ? pal.accentSecondary : pal.accentPrimary;

    const char* tags[4] = { "01  /  HIGH-PASS", "02  /  MID-SIDE BELOW CROSSOVER", "03  /  LOW-PASS", "04  /  TRIM & METERS" };
    const char* titles[4] = { "LOW CUT", "LOW-END STEREO", "HIGH CUT", "MASTER OUTPUT" };

    g.setColour(pal.sectionWellBg);
    g.fillRoundedRectangle(bay, 7.0f);

    g.setColour(pal.hudGrid.withAlpha(0.08f));
    for (float x = bay.getX() + 20.0f; x < bay.getRight() - 10.0f; x += 20.0f) {
        g.drawVerticalLine(static_cast<int>(x), bay.getY() + 44.0f, bay.getBottom() - 8.0f);
    }
    for (float y = bay.getY() + 60.0f; y < bay.getBottom() - 10.0f; y += 20.0f) {
        g.drawHorizontalLine(static_cast<int>(y), bay.getX() + 6.0f, bay.getRight() - 6.0f);
    }

    if (active) {
        g.setColour(accent.withAlpha(0.18f));
        g.drawRoundedRectangle(bay.expanded(1.5f), 8.0f, 2.5f);
    }
    g.setColour(active ? accent.withAlpha(0.85f) : pal.sectionBorder);
    g.drawRoundedRectangle(bay, 7.0f, active ? 1.5f : 1.0f);

    const auto headerPlate = bay.reduced(8.0f).withHeight(34.0f);
    g.setColour(pal.sectionHeaderPlate);
    g.fillRoundedRectangle(headerPlate, 4.5f);

    g.setColour(pal.headerSub);
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawText(tags[index], headerPlate.withHeight(14.0f).translated(0.0f, 2.0f), juce::Justification::centred);
    g.setColour(active ? accent : pal.headerSub);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(titles[index], headerPlate.withTrimmedTop(14.0f).withHeight(18.0f), juce::Justification::centred);

    g.setColour(active ? accent : pal.sectionBorder);
    g.fillRoundedRectangle(bay.getX() + 16.0f, headerPlate.getBottom() + 3.0f, bay.getWidth() - 32.0f, 2.0f, 1.0f);

    if (index < 3) {
        const juce::Rectangle<float> display(bay.getX() + 14.0f, bay.getY() + 52.0f, bay.getWidth() - 28.0f, 52.0f);
        g.setColour(pal.meterFaceBottom);
        g.fillRoundedRectangle(display, 4.5f);
        g.setColour(active ? accent.withAlpha(0.45f) : pal.sectionBorder);
        g.drawRoundedRectangle(display, 4.5f, 1.0f);

        if (index == 1) {
            paintStereoDisplay(g, display, active);
        } else {
            paintFilterDisplay(g, display, index == 0, active);
        }
    }

    if (active) {
        g.setColour(accent);
        g.fillRoundedRectangle(bay.getCentreX() - 36.0f, bay.getBottom() - 3.0f, 72.0f, 3.0f, 1.5f);
    }
}

void AnalogMasterEditor::paintFilterDisplay(juce::Graphics& g, juce::Rectangle<float> area, bool isHighPass, bool active) const {
    const auto& pal = lookAndFeel_.getPalette();
    const juce::Colour trace = active ? pal.accentPrimary : pal.headerSub.withAlpha(0.45f);

    auto inner = area.reduced(6.0f, 4.0f);
    const auto readout = inner.removeFromBottom(11.0f);
    const auto plot = inner.withTrimmedBottom(2.0f);

    const double sampleRate = processor_.getSampleRate() > 0.0 ? processor_.getSampleRate() : 48000.0;
    const double cutoff = (isHighPass ? lowCutFreqSlider_ : highCutFreqSlider_).getValue();
    const int slopeIndex = std::max(0, (isHighPass ? lowCutSlopeBox_ : highCutSlopeBox_).getSelectedItemIndex());
    const auto slope = static_cast<analog_master::core::FilterSlope>(slopeIndex);
    const int order = analog_master::core::slopeToOrder(slope);
    const auto design = isHighPass
        ? analog_master::core::designButterworthHighPass(sampleRate, cutoff, order)
        : analog_master::core::designButterworthLowPass(sampleRate, cutoff, order);

    const double minF = isHighPass ? 10.0 : 1000.0;
    const double maxF = isHighPass ? 1000.0 : std::min(30000.0, sampleRate * 0.499);
    const double logSpan = std::log(maxF / minF);
    constexpr double kFloorDb = -48.0;

    auto xFor = [&](double f) { return plot.getX() + static_cast<float>(std::log(f / minF) / logSpan) * plot.getWidth(); };
    auto yFor = [&](double db) { return plot.getY() + static_cast<float>(std::clamp(db, kFloorDb, 0.0) / kFloorDb) * plot.getHeight(); };

    g.setColour(pal.hudGrid.withAlpha(0.25f));
    for (const double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 2000.0, 5000.0, 10000.0, 20000.0 }) {
        if (f > minF && f < maxF) {
            g.drawVerticalLine(static_cast<int>(xFor(f)), plot.getY(), plot.getBottom());
        }
    }
    g.drawHorizontalLine(static_cast<int>(yFor(-24.0)), plot.getX(), plot.getRight());

    juce::Path curve;
    constexpr int kSteps = 80;
    for (int s = 0; s <= kSteps; ++s) {
        const double f = minF * std::exp(logSpan * static_cast<double>(s) / kSteps);
        const juce::Point<float> p(xFor(f), yFor(filterResponseDb(design, f, sampleRate)));
        if (s == 0) {
            curve.startNewSubPath(p);
        } else {
            curve.lineTo(p);
        }
    }
    juce::Path fill(curve);
    fill.lineTo(plot.getRight(), plot.getBottom());
    fill.lineTo(plot.getX(), plot.getBottom());
    fill.closeSubPath();

    g.setColour(trace.withAlpha(active ? 0.18f : 0.06f));
    g.fillPath(fill);
    g.setColour(trace);
    g.strokePath(curve, juce::PathStrokeType(1.6f));

    g.setColour(pal.headerTitle);
    g.fillEllipse(juce::Rectangle<float>(5.0f, 5.0f).withCentre({ xFor(std::clamp(cutoff, minF, maxF)), yFor(-3.0) }));

    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(pal.headerTitle.withAlpha(0.85f));
    const char* freqParamId = isHighPass ? analog_master::params::kLowCutFreq : analog_master::params::kHighCutFreq;
    g.drawText(processor_.parameters.getParameter(freqParamId)->getCurrentValueAsText(), readout, juce::Justification::centredLeft);
    g.drawText(juce::String(analog_master::core::slopeToString(slope)), readout, juce::Justification::centredRight);
}

void AnalogMasterEditor::paintStereoDisplay(juce::Graphics& g, juce::Rectangle<float> area, bool active) const {
    const auto& pal = lookAndFeel_.getPalette();
    const juce::Colour trace = active ? pal.accentPrimary : pal.headerSub.withAlpha(0.45f);

    auto inner = area.reduced(6.0f, 4.0f);
    const auto readout = inner.removeFromBottom(11.0f);
    const auto plot = inner.withTrimmedBottom(2.0f);

    const float widthPct = static_cast<float>(stereoWidthSlider_.getValue());
    const bool soloLow = stereoSoloButton_.getToggleState();
    const float apexY = plot.getBottom();
    const float topY = plot.getY();

    // The dashed lines mark 100% width
    const float fullSpread = plot.getWidth() * 0.46f;
    const float spread = std::max(1.0f, fullSpread * widthPct / 200.0f);
    const float unityX = fullSpread * 0.5f;

    juce::Path cone;
    cone.startNewSubPath(plot.getCentreX(), apexY);
    cone.lineTo(plot.getCentreX() - spread, topY);
    cone.lineTo(plot.getCentreX() + spread, topY);
    cone.closeSubPath();
    g.setColour(trace.withAlpha(active ? 0.22f : 0.08f));
    g.fillPath(cone);
    g.setColour(trace);
    g.strokePath(cone, juce::PathStrokeType(1.4f));

    const float dashes[] = { 3.0f, 3.0f };
    g.setColour(pal.headerTitle.withAlpha(0.45f));
    for (const float side : { -1.0f, 1.0f }) {
        g.drawDashedLine(juce::Line<float>(plot.getCentreX(), apexY, plot.getCentreX() + side * unityX, topY), dashes, 2, 1.0f);
    }
    g.setColour(pal.headerTitle.withAlpha(0.35f));
    g.drawVerticalLine(static_cast<int>(plot.getCentreX()), topY, apexY);

    juce::String mode = "WIDE";
    if (widthPct < 0.5f) {
        mode = "MONO";
    } else if (widthPct < 99.5f) {
        mode = "NARROW";
    } else if (widthPct < 100.5f) {
        mode = "UNCHANGED";
    }

    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(pal.headerTitle.withAlpha(0.85f));
    const auto crossoverText = processor_.parameters.getParameter(analog_master::params::kStereoFreq)->getCurrentValueAsText();
    g.drawText("BELOW " + crossoverText, readout, juce::Justification::centredLeft);
    g.setColour(soloLow ? pal.accentSecondary : pal.headerTitle.withAlpha(0.85f));
    g.drawText(soloLow ? "LOWS ONLY" : mode, readout, juce::Justification::centredRight);
}

void AnalogMasterEditor::resized() {
    const float width = static_cast<float>(getWidth());

    themeLabel_.setBounds(static_cast<int>(width - 224.0f), 17, 50, 26);
    themeBox_.setBounds(static_cast<int>(width - 170.0f), 17, 136, 26);

    // 1. Low Cut
    {
        auto r = moduleBayBounds(0).reduced(14.0f);
        r.removeFromTop(98.0f);
        lowCutFreqLabel_.setBounds(r.removeFromTop(18.0f).toNearestInt());
        lowCutFreqSlider_.setBounds(r.removeFromTop(165.0f).toNearestInt());
        r.removeFromTop(12.0f);
        lowCutSlopeLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        lowCutSlopeBox_.setBounds(r.removeFromTop(28.0f).reduced(12.0f, 0.0f).toNearestInt());
        r.removeFromTop(18.0f);
        lowCutBypassButton_.setBounds(r.removeFromTop(36.0f).reduced(28.0f, 0.0f).toNearestInt());
    }

    // 2. Low-End Stereo Field
    {
        auto r = moduleBayBounds(1).reduced(14.0f);
        r.removeFromTop(96.0f);
        stereoWidthLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        stereoWidthSlider_.setBounds(r.removeFromTop(116.0f).toNearestInt());
        r.removeFromTop(4.0f);
        stereoFreqLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        stereoFreqSlider_.setBounds(r.removeFromTop(116.0f).toNearestInt());
        r.removeFromTop(8.0f);
        auto toggles = r.removeFromTop(36.0f);
        stereoBypassButton_.setBounds(toggles.removeFromLeft(toggles.getWidth() * 0.5f).reduced(3.0f, 0.0f).toNearestInt());
        stereoSoloButton_.setBounds(toggles.reduced(3.0f, 0.0f).toNearestInt());
    }

    // 3. High Cut
    {
        auto r = moduleBayBounds(2).reduced(14.0f);
        r.removeFromTop(98.0f);
        highCutFreqLabel_.setBounds(r.removeFromTop(18.0f).toNearestInt());
        highCutFreqSlider_.setBounds(r.removeFromTop(165.0f).toNearestInt());
        r.removeFromTop(12.0f);
        highCutSlopeLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        highCutSlopeBox_.setBounds(r.removeFromTop(28.0f).reduced(12.0f, 0.0f).toNearestInt());
        r.removeFromTop(18.0f);
        highCutBypassButton_.setBounds(r.removeFromTop(36.0f).reduced(28.0f, 0.0f).toNearestInt());
    }

    // 4. Master Output
    {
        auto r = moduleBayBounds(3).reduced(14.0f);
        r.removeFromTop(44.0f);
        meter_.setBounds(r.removeFromTop(148.0f).reduced(2.0f, 0.0f).toNearestInt());
        r.removeFromTop(6.0f);
        meterReferenceBox_.setBounds(r.removeFromTop(24.0f).reduced(12.0f, 0.0f).toNearestInt());
        r.removeFromTop(8.0f);
        masterGainLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        masterGainSlider_.setBounds(r.removeFromTop(113.0f).toNearestInt());
        r.removeFromTop(12.0f);
        masterPowerButton_.setBounds(r.removeFromTop(36.0f).reduced(20.0f, 0.0f).toNearestInt());
    }
}
