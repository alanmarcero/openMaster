#include "PluginEditor.h"
#include "BinaryData.h"
#include <algorithm>
#include <cmath>

AnalogMasterEditor::AnalogMasterEditor(AnalogMasterProcessor& owner)
    : AudioProcessorEditor(&owner), processor_(owner) {
    setLookAndFeel(&lookAndFeel_);

    // Load Gemini-generated custom Alan Marcero logo & cyberpunk chassis backdrop from BinaryData
    customLogoImage_ = juce::ImageCache::getFromMemory(BinaryData::alan_marcero_logo_jpg,
                                                       BinaryData::alan_marcero_logo_jpgSize);
    backdropImage_ = juce::ImageCache::getFromMemory(BinaryData::cyber_chassis_backdrop_jpg,
                                                     BinaryData::cyber_chassis_backdrop_jpgSize);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& suffix) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 18);
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, lookAndFeel_.getPalette().labelText);
        l.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        addAndMakeVisible(l);
    };

    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& text, const juce::StringArray& items) {
        c.addItemList(items, 1);
        c.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(c);

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, lookAndFeel_.getPalette().labelText);
        l.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        addAndMakeVisible(l);
    };

    const juce::StringArray slopeLabels { "12 dB/OCT", "24 dB/OCT", "48 dB/OCT", "72 dB HARD", "96 dB BRICK" };

    // 0. Look / Theme Switcher in Header (5 Cyberpunk Themes)
    const juce::StringArray themeChoices {
        "Cyber Neon",
        "Chrome Deck",
        "Acid Bio-Grid",
        "Deep Net Ice",
        "Solar Overdrive"
    };
    themeBox_.addItemList(themeChoices, 1);
    themeBox_.setJustificationType(juce::Justification::centred);
    themeBox_.onChange = [this]() {
        applyTheme(themeBox_.getSelectedItemIndex());
    };
    addAndMakeVisible(themeBox_);

    themeLabel_.setText("THEME:", juce::dontSendNotification);
    themeLabel_.setJustificationType(juce::Justification::centredRight);
    themeLabel_.setColour(juce::Label::textColourId, lookAndFeel_.getPalette().labelText);
    themeLabel_.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    addAndMakeVisible(themeLabel_);

    // 1. Low Cut
    setupSlider(lowCutFreqSlider_, lowCutFreqLabel_, "CUTOFF FREQUENCY", " Hz");
    setupCombo(lowCutSlopeBox_, lowCutSlopeLabel_, "FILTER SLOPE", slopeLabels);
    addAndMakeVisible(lowCutBypassButton_);

    // 2. Stereo Field
    setupSlider(stereoWidthSlider_, stereoWidthLabel_, "LOW STEREO WIDTH", " %");
    setupSlider(stereoFreqSlider_, stereoFreqLabel_, "X-OVER FREQUENCY", " Hz");
    addAndMakeVisible(stereoBypassButton_);
    addAndMakeVisible(stereoSoloButton_);

    // 3. High Cut
    setupSlider(highCutFreqSlider_, highCutFreqLabel_, "CUTOFF FREQUENCY", " Hz");
    setupCombo(highCutSlopeBox_, highCutSlopeLabel_, "FILTER SLOPE", slopeLabels);
    addAndMakeVisible(highCutBypassButton_);

    // 4. Master
    setupSlider(masterGainSlider_, masterGainLabel_, "MASTER OUTPUT TRIM", " dB");
    addAndMakeVisible(masterBypassButton_);
    addAndMakeVisible(meter_);

    // APVTS Attachments
    using namespace analog_master::params;
    themeAttach_ = std::make_unique<ComboBoxAttachment>(processor_.parameters, kTheme, themeBox_);

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
    masterBypassAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kMasterPower, masterBypassButton_);

    // Double-click any rotary knob to snap back to its mastering default
    lowCutFreqSlider_.setDoubleClickReturnValue(true, 20.0);
    stereoWidthSlider_.setDoubleClickReturnValue(true, 0.0);
    stereoFreqSlider_.setDoubleClickReturnValue(true, 120.0);
    highCutFreqSlider_.setDoubleClickReturnValue(true, 20000.0);
    masterGainSlider_.setDoubleClickReturnValue(true, 0.0);

    processor_.parameters.addParameterListener(kTheme, this);

    applyTheme(themeBox_.getSelectedItemIndex());

    setSize(1040, 540);
    startTimerHz(30);
}

AnalogMasterEditor::~AnalogMasterEditor() {
    processor_.parameters.removeParameterListener(analog_master::params::kTheme, this);
    stopTimer();
    setLookAndFeel(nullptr);
}

void AnalogMasterEditor::parameterChanged(const juce::String& parameterID, float newValue) {
    if (parameterID == analog_master::params::kTheme) {
        juce::MessageManager::callAsync([this, idx = static_cast<int>(newValue)]() {
            applyTheme(idx);
        });
    }
}

void AnalogMasterEditor::applyTheme(int themeIndex) {
    themeIndex = std::clamp(themeIndex, 0, 4);
    auto theme = static_cast<analog_master::ui::Theme>(themeIndex);
    lookAndFeel_.setTheme(theme);
    meter_.setThemePalette(lookAndFeel_.getPalette());

    const auto& pal = lookAndFeel_.getPalette();
    lowCutFreqLabel_.setColour(juce::Label::textColourId, pal.labelText);
    lowCutSlopeLabel_.setColour(juce::Label::textColourId, pal.labelText);
    stereoWidthLabel_.setColour(juce::Label::textColourId, pal.labelText);
    stereoFreqLabel_.setColour(juce::Label::textColourId, pal.labelText);
    highCutFreqLabel_.setColour(juce::Label::textColourId, pal.labelText);
    highCutSlopeLabel_.setColour(juce::Label::textColourId, pal.labelText);
    masterGainLabel_.setColour(juce::Label::textColourId, pal.labelText);
    themeLabel_.setColour(juce::Label::textColourId, pal.accentPrimary);

    sendLookAndFeelChange();
    repaint();
}

void AnalogMasterEditor::timerCallback() {
    auto levels = processor_.meterLevels();
    const float levelL = levels.vuL > 1e-4f ? levels.vuL : levels.peakL;
    const float levelR = levels.vuR > 1e-4f ? levels.vuR : levels.peakR;
    meter_.setLevels(levelL, levelR, levels.clipL, levels.clipR, levels.correlation);
    repaint();
}

void AnalogMasterEditor::paint(juce::Graphics& g) {
    const auto width = static_cast<float>(getWidth());
    const auto height = static_cast<float>(getHeight());
    const auto& pal = lookAndFeel_.getPalette();

    // 1. Cyberpunk Carbon-Titanium Base Gradient + Gemini-Generated Chassis Backdrop
    juce::ColourGradient panelGrad(pal.panelGradTop, 0.0f, 0.0f,
                                  pal.panelGradBottom, 0.0f, height, false);
    panelGrad.addColour(0.32, pal.panelGradMid1);
    panelGrad.addColour(0.72, pal.panelGradMid2);
    g.setGradientFill(panelGrad);
    g.fillRect(0.0f, 0.0f, width, height);

    if (backdropImage_.isValid()) {
        g.setOpacity(0.32f);
        g.drawImage(backdropImage_, juce::Rectangle<float>(0.0f, 0.0f, width, height),
                    juce::RectanglePlacement::stretchToFit);
        g.setOpacity(1.0f);
    }

    // Subtle cyber scanlines across chassis
    g.setColour(pal.panelStripe);
    for (int y = 0; y < getHeight(); y += 3) {
        g.drawHorizontalLine(y, 0.0f, width);
    }

    // Outer cyber chassis border and glowing neon inner frame
    g.setColour(pal.panelBorder);
    g.drawRect(0.0f, 0.0f, width, height, 2.0f);
    g.setColour(pal.panelInnerBorder);
    g.drawRect(3.0f, 3.0f, width - 6.0f, height - 6.0f, 1.2f);

    // 2. Corner Cyber Hex Rack Bolts
    lookAndFeel_.drawRackScrew(g, 18.0f, 18.0f, 7.5f);
    lookAndFeel_.drawRackScrew(g, width - 18.0f, 18.0f, 7.5f);
    lookAndFeel_.drawRackScrew(g, 18.0f, height - 18.0f, 7.5f);
    lookAndFeel_.drawRackScrew(g, width - 18.0f, height - 18.0f, 7.5f);

    // 3. Top Header HUD with Custom Alan Marcero Logo
    if (customLogoImage_.isValid()) {
        const juce::Rectangle<float> logoRect(36.0f, 9.0f, 44.0f, 44.0f);
        juce::Graphics::ScopedSaveState save(g);
        juce::Path clipCircle;
        clipCircle.addEllipse(logoRect);
        g.reduceClipRegion(clipCircle);
        g.drawImage(customLogoImage_, logoRect, juce::RectanglePlacement::centred);
        g.setColour(pal.accentPrimary.withAlpha(0.8f));
        g.drawEllipse(logoRect.reduced(0.5f), 1.5f);
    }

    g.setColour(pal.headerBrand);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("ALAN MARCERO", 88, 12, 165, 18, juce::Justification::centredLeft);

    g.setColour(pal.headerSub);
    g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
    g.drawText("CYBERNETIC MASTERING DSP", 88, 29, 175, 14, juce::Justification::centredLeft);

    // Center Beveled Cyber Title Plate
    juce::Rectangle<float> centerTitlePlate(width * 0.5f - 210.0f, 8.0f, 420.0f, 44.0f);
    g.setColour(pal.sectionWellBg);
    g.fillRoundedRectangle(centerTitlePlate, 6.0f);
    g.setColour(pal.accentPrimary.withAlpha(0.55f));
    g.drawRoundedRectangle(centerTitlePlate, 6.0f, 1.3f);

    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.setColour(pal.headerTitle);
    g.drawText("ANALOG MASTER STRIP", centerTitlePlate.withHeight(25.0f).withY(10.0f).toNearestInt(), juce::Justification::centred);

    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(pal.accentPrimary);
    g.drawText("CYBERPUNK SUB-BASS & AIR CONTOURING SYSTEM", centerTitlePlate.withHeight(14.0f).withY(33.0f).toNearestInt(), juce::Justification::centred);

    // Master System Status Pill in Header
    const bool powerOn = masterBypassButton_.getToggleState();
    juce::Rectangle<float> statusPill(width - 335.0f, 17.0f, 108.0f, 26.0f);
    g.setColour(powerOn ? pal.accentPrimary.withAlpha(0.20f) : pal.switchSlotBg);
    g.fillRoundedRectangle(statusPill, 13.0f);
    g.setColour(powerOn ? pal.accentPrimary : pal.sectionBorder);
    g.drawRoundedRectangle(statusPill, 13.0f, 1.2f);
    g.setColour(powerOn ? pal.headerTitle : pal.switchInactiveText);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText(powerOn ? "SYS: ONLINE" : "SYS: BYPASS", statusPill.toNearestInt(), juce::Justification::centred);

    // Glowing Header Divider Line
    g.setColour(pal.headerDivider);
    g.drawLine(32.0f, 60.0f, width - 32.0f, 60.0f, 1.2f);

    // 4. Four Cyberpunk Module Bays
    const float sectionY = 70.0f;
    const float sectionH = height - sectionY - 22.0f;
    const float sectionMargin = 32.0f;
    const float totalAvailW = width - sectionMargin * 2.0f;
    const float sectionSpacing = 12.0f;
    const float sectionW = (totalAvailW - sectionSpacing * 3.0f) * 0.25f;

    const char* moduleTags[4] = {
        "MOD 01 // SUB-BASS HPF",
        "MOD 02 // M/S FIELD",
        "MOD 03 // HARMONIC LPF",
        "MOD 04 // OUTPUT & TELEMETRY"
    };

    const char* sectionTitles[4] = {
        "LOW CUT EQ",
        "LOW STEREO FIELD",
        "HIGH CUT EQ",
        "MASTER OUTPUT"
    };

    const bool moduleActive[4] = {
        powerOn && lowCutBypassButton_.getToggleState(),
        powerOn && stereoBypassButton_.getToggleState(),
        powerOn && highCutBypassButton_.getToggleState(),
        powerOn
    };

    constexpr int kPolesPerSlope[5] = { 2, 4, 8, 12, 16 };
    constexpr int kDbPerOct[5] = { 12, 24, 48, 72, 96 };

    for (int i = 0; i < 4; ++i) {
        const float sx = sectionMargin + static_cast<float>(i) * (sectionW + sectionSpacing);
        juce::Rectangle<float> secRect(sx, sectionY, sectionW, sectionH);
        const bool active = moduleActive[i];
        const juce::Colour modAccent = (i == 3) ? pal.accentSecondary : pal.accentPrimary;

        // Module Bay Background
        g.setColour(pal.sectionWellBg);
        g.fillRoundedRectangle(secRect, 7.0f);

        // Subtle internal cyber HUD grid lines inside each module bay
        g.setColour(pal.hudGrid.withAlpha(0.08f));
        for (float gx = secRect.getX() + 20.0f; gx < secRect.getRight() - 10.0f; gx += 20.0f) {
            g.drawVerticalLine(static_cast<int>(gx), secRect.getY() + 44.0f, secRect.getBottom() - 8.0f);
        }
        for (float gy = secRect.getY() + 60.0f; gy < secRect.getBottom() - 10.0f; gy += 20.0f) {
            g.drawHorizontalLine(static_cast<int>(gy), secRect.getX() + 6.0f, secRect.getRight() - 6.0f);
        }

        // Outer neon glow when module is engaged
        if (active) {
            g.setColour(modAccent.withAlpha(0.18f));
            g.drawRoundedRectangle(secRect.expanded(1.5f), 8.0f, 2.5f);
        }

        // Module Bay Border
        g.setColour(active ? modAccent.withAlpha(0.85f) : pal.sectionBorder);
        g.drawRoundedRectangle(secRect, 7.0f, active ? 1.5f : 1.0f);

        // Top Module Header Plate
        juce::Rectangle<float> headerPlate(sx + 8.0f, sectionY + 8.0f, sectionW - 16.0f, 34.0f);
        g.setColour(pal.sectionHeaderPlate);
        g.fillRoundedRectangle(headerPlate, 4.5f);

        // Top cyber tag
        g.setColour(pal.headerSub);
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.drawText(moduleTags[i], headerPlate.withHeight(13.0f).withY(headerPlate.getY() + 2.0f).toNearestInt(), juce::Justification::centred);

        // Main Module Title
        g.setColour(active ? modAccent : pal.headerSub);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(sectionTitles[i], headerPlate.withHeight(18.0f).withY(headerPlate.getY() + 14.0f).toNearestInt(), juce::Justification::centred);

        // Glowing neon underline bar beneath module header
        g.setColour(active ? modAccent : pal.sectionBorder);
        g.fillRoundedRectangle(sx + 16.0f, headerPlate.getBottom() + 3.0f, sectionW - 32.0f, 2.0f, 1.0f);

        // Holographic Filter / Stereo Field Mini-Display in Bays 0, 1, 2
        if (i < 3) {
            juce::Rectangle<float> dispRect(sx + 14.0f, sectionY + 52.0f, sectionW - 28.0f, 52.0f);
            g.setColour(pal.meterFaceBottom);
            g.fillRoundedRectangle(dispRect, 4.5f);
            g.setColour(active ? modAccent.withAlpha(0.45f) : pal.sectionBorder);
            g.drawRoundedRectangle(dispRect, 4.5f, 1.0f);

            // Internal reticle lines
            g.setColour(pal.hudGrid.withAlpha(0.16f));
            g.drawHorizontalLine(static_cast<int>(dispRect.getY() + 14.0f), dispRect.getX() + 3.0f, dispRect.getRight() - 3.0f);
            g.drawHorizontalLine(static_cast<int>(dispRect.getCentreY()), dispRect.getX() + 3.0f, dispRect.getRight() - 3.0f);

            const auto plot = dispRect.reduced(5.0f, 6.0f);
            const juce::Colour traceCol = active ? modAccent : pal.headerSub.withAlpha(0.45f);

            if (i == 0 || i == 2) {
                const bool isHighPass = (i == 0);
                const double fc = isHighPass ? lowCutFreqSlider_.getValue() : highCutFreqSlider_.getValue();
                const int slopeIdx = std::clamp(isHighPass ? lowCutSlopeBox_.getSelectedItemIndex() : highCutSlopeBox_.getSelectedItemIndex(), 0, 4);
                const int poles = kPolesPerSlope[slopeIdx];
                const double minF = isHighPass ? 10.0 : 1000.0;
                const double maxF = isHighPass ? 1000.0 : 30000.0;
                const double logRatio = std::log(maxF / minF);

                juce::Path curvePath;
                juce::Path fillPath;
                constexpr int kSteps = 44;
                for (int s = 0; s <= kSteps; ++s) {
                    const float normX = static_cast<float>(s) / static_cast<float>(kSteps);
                    const double f = minF * std::exp(static_cast<double>(normX) * logRatio);
                    const double ratio = isHighPass ? (fc / f) : (f / fc);
                    const double magDb = -10.0 * std::log10(1.0 + std::pow(ratio, 2.0 * poles));
                    const float normY = std::clamp(static_cast<float>((magDb + 36.0) / 36.0), 0.0f, 1.0f);
                    const float px = plot.getX() + normX * plot.getWidth();
                    const float py = plot.getBottom() - normY * (plot.getHeight() - 6.0f);

                    if (s == 0) {
                        curvePath.startNewSubPath(px, py);
                        fillPath.startNewSubPath(px, plot.getBottom());
                        fillPath.lineTo(px, py);
                    } else {
                        curvePath.lineTo(px, py);
                        fillPath.lineTo(px, py);
                    }
                }
                fillPath.lineTo(plot.getRight(), plot.getBottom());
                fillPath.closeSubPath();

                g.setColour(traceCol.withAlpha(active ? 0.18f : 0.06f));
                g.fillPath(fillPath);
                g.setColour(traceCol);
                g.strokePath(curvePath, juce::PathStrokeType(1.7f));

                // Cutoff -3 dB marker dot
                const float fcNormX = std::clamp(static_cast<float>(std::log(fc / minF) / logRatio), 0.0f, 1.0f);
                const float fcNormY = std::clamp(static_cast<float>((-3.0 + 36.0) / 36.0), 0.0f, 1.0f);
                const float cx = plot.getX() + fcNormX * plot.getWidth();
                const float cy = plot.getBottom() - fcNormY * (plot.getHeight() - 6.0f);
                g.setColour(pal.headerTitle);
                g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);

                // Telemetry text overlay
                g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
                g.setColour(pal.headerTitle.withAlpha(0.85f));
                juce::String freqStr = (fc >= 1000.0) ? (juce::String(fc / 1000.0, 1) + " kHz") : (juce::String(static_cast<int>(fc)) + " Hz");
                g.drawText(freqStr + " // -" + juce::String(kDbPerOct[slopeIdx]) + " dB/OCT",
                           dispRect.reduced(6.0f, 3.0f).toNearestInt(), juce::Justification::bottomRight);
            } else {
                // Bay 1: Holographic M/S Stereo Width Cone & Crossover Display
                const float widthPct = static_cast<float>(stereoWidthSlider_.getValue());
                const float widthNorm = std::clamp(widthPct / 200.0f, 0.0f, 1.0f);
                const double xoverHz = stereoFreqSlider_.getValue();
                const bool soloLow = stereoSoloButton_.getToggleState();

                const float centerX = plot.getCentreX();
                const float baseY = plot.getBottom() - 2.0f;
                const float topY = plot.getY() + 4.0f;
                const float halfSpread = 2.0f + widthNorm * (plot.getWidth() * 0.44f);

                juce::Path cone;
                cone.startNewSubPath(centerX, baseY);
                cone.lineTo(centerX - halfSpread, topY);
                cone.lineTo(centerX + halfSpread, topY);
                cone.closeSubPath();

                g.setColour(traceCol.withAlpha(active ? 0.22f : 0.08f));
                g.fillPath(cone);
                g.setColour(traceCol);
                g.strokePath(cone, juce::PathStrokeType(1.5f));

                // Center Mid axis line
                g.setColour(pal.headerTitle.withAlpha(0.6f));
                g.drawVerticalLine(static_cast<int>(centerX), topY, baseY);

                g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
                g.setColour(soloLow ? pal.meterRedZone : pal.headerTitle.withAlpha(0.85f));
                juce::String modeTag = soloLow ? "SOLO LOW" : (widthPct < 5.0f ? "MONO SUB" : (widthPct > 105.0f ? "M/S WIDE" : "M/S CONTOUR"));
                g.drawText(juce::String(static_cast<int>(xoverHz)) + " Hz // " + modeTag,
                           dispRect.reduced(6.0f, 3.0f).toNearestInt(), juce::Justification::bottomRight);
            }
        }

        // Bottom neon Edge Underglow Bar on active modules
        if (active) {
            g.setColour(modAccent);
            g.fillRoundedRectangle(secRect.getCentreX() - 36.0f, secRect.getBottom() - 3.0f, 72.0f, 3.0f, 1.5f);
        }
    }
}

void AnalogMasterEditor::resized() {
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());

    // Header Look Selector
    themeLabel_.setBounds(static_cast<int>(width - 224.0f), 17, 50, 26);
    themeBox_.setBounds(static_cast<int>(width - 170.0f), 17, 136, 26);

    const float sectionY = 70.0f;
    const float sectionH = height - sectionY - 22.0f;
    const float sectionMargin = 32.0f;
    const float totalAvailW = width - sectionMargin * 2.0f;
    const float sectionSpacing = 12.0f;
    const float sectionW = (totalAvailW - sectionSpacing * 3.0f) * 0.25f;

    auto getSectionRect = [&](int index) {
        const float sx = sectionMargin + static_cast<float>(index) * (sectionW + sectionSpacing);
        return juce::Rectangle<float>(sx, sectionY, sectionW, sectionH);
    };

    // SECTION 0: LOW CUT EQ
    {
        auto r = getSectionRect(0).reduced(14.0f);
        r.removeFromTop(98.0f); // Header + Holographic HPF Curve Display

        lowCutFreqLabel_.setBounds(r.removeFromTop(18.0f).toNearestInt());
        lowCutFreqSlider_.setBounds(r.removeFromTop(165.0f).toNearestInt());

        r.removeFromTop(12.0f);
        lowCutSlopeLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        lowCutSlopeBox_.setBounds(r.removeFromTop(28.0f).reduced(12.0f, 0.0f).toNearestInt());

        r.removeFromTop(18.0f);
        lowCutBypassButton_.setBounds(r.removeFromTop(36.0f).reduced(28.0f, 0.0f).toNearestInt());
    }

    // SECTION 1: LOW STEREO FIELD
    {
        auto r = getSectionRect(1).reduced(14.0f);
        r.removeFromTop(96.0f); // Header + Holographic Stereo Cone Display

        stereoWidthLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        stereoWidthSlider_.setBounds(r.removeFromTop(116.0f).toNearestInt());

        r.removeFromTop(4.0f);
        stereoFreqLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        stereoFreqSlider_.setBounds(r.removeFromTop(116.0f).toNearestInt());

        r.removeFromTop(8.0f);
        auto toggleArea = r.removeFromTop(36.0f);
        stereoBypassButton_.setBounds(toggleArea.removeFromLeft(toggleArea.getWidth() * 0.5f).reduced(3.0f, 0.0f).toNearestInt());
        stereoSoloButton_.setBounds(toggleArea.reduced(3.0f, 0.0f).toNearestInt());
    }

    // SECTION 2: HIGH CUT EQ
    {
        auto r = getSectionRect(2).reduced(14.0f);
        r.removeFromTop(98.0f); // Header + Holographic LPF Curve Display

        highCutFreqLabel_.setBounds(r.removeFromTop(18.0f).toNearestInt());
        highCutFreqSlider_.setBounds(r.removeFromTop(165.0f).toNearestInt());

        r.removeFromTop(12.0f);
        highCutSlopeLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        highCutSlopeBox_.setBounds(r.removeFromTop(28.0f).reduced(12.0f, 0.0f).toNearestInt());

        r.removeFromTop(18.0f);
        highCutBypassButton_.setBounds(r.removeFromTop(36.0f).reduced(28.0f, 0.0f).toNearestInt());
    }

    // SECTION 3: MASTER OUTPUT
    {
        auto r = getSectionRect(3).reduced(14.0f);
        r.removeFromTop(44.0f);

        meter_.setBounds(r.removeFromTop(148.0f).reduced(2.0f, 0.0f).toNearestInt());

        r.removeFromTop(12.0f);
        masterGainLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        masterGainSlider_.setBounds(r.removeFromTop(135.0f).toNearestInt());

        r.removeFromTop(12.0f);
        masterBypassButton_.setBounds(r.removeFromTop(36.0f).reduced(20.0f, 0.0f).toNearestInt());
    }
}
