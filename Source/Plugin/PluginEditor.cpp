#include "PluginEditor.h"

AnalogMasterEditor::AnalogMasterEditor(AnalogMasterProcessor& owner)
    : AudioProcessorEditor(&owner), processor_(owner) {
    setLookAndFeel(&lookAndFeel_);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& suffix) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colour(0xffdfc488));
        l.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        addAndMakeVisible(l);
    };

    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& text, const juce::StringArray& items) {
        c.addItemList(items, 1);
        c.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(c);

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colour(0xffdfc488));
        l.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        addAndMakeVisible(l);
    };

    const juce::StringArray slopeLabels { "12 dB", "24 dB", "48 dB", "64 dB HARD", "96 dB BRICK" };

    // 1. Low Cut
    setupSlider(lowCutFreqSlider_, lowCutFreqLabel_, "FREQUENCY", " Hz");
    setupCombo(lowCutSlopeBox_, lowCutSlopeLabel_, "SLOPE", slopeLabels);
    addAndMakeVisible(lowCutBypassButton_);

    // 2. Stereo Field
    setupSlider(stereoWidthSlider_, stereoWidthLabel_, "STEREO WIDTH", " %");
    setupSlider(stereoFreqSlider_, stereoFreqLabel_, "CROSSOVER", " Hz");
    addAndMakeVisible(stereoBypassButton_);
    addAndMakeVisible(stereoSoloButton_);

    // 3. High Cut
    setupSlider(highCutFreqSlider_, highCutFreqLabel_, "FREQUENCY", " Hz");
    setupCombo(highCutSlopeBox_, highCutSlopeLabel_, "SLOPE", slopeLabels);
    addAndMakeVisible(highCutBypassButton_);

    // 4. Master
    setupSlider(masterGainSlider_, masterGainLabel_, "MASTER TRIM", " dB");
    addAndMakeVisible(masterBypassButton_);
    addAndMakeVisible(meter_);

    // APVTS Attachments
    using namespace analog_master::params;
    lowCutFreqAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kLowCutFreq, lowCutFreqSlider_);
    lowCutSlopeAttach_ = std::make_unique<ComboBoxAttachment>(processor_.parameters, kLowCutSlope, lowCutSlopeBox_);
    lowCutBypassAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kLowCutBypass, lowCutBypassButton_);

    stereoFreqAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kStereoFreq, stereoFreqSlider_);
    stereoWidthAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kStereoWidth, stereoWidthSlider_);
    stereoBypassAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kStereoBypass, stereoBypassButton_);
    stereoSoloAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kStereoSoloLow, stereoSoloButton_);

    highCutFreqAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kHighCutFreq, highCutFreqSlider_);
    highCutSlopeAttach_ = std::make_unique<ComboBoxAttachment>(processor_.parameters, kHighCutSlope, highCutSlopeBox_);
    highCutBypassAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kHighCutBypass, highCutBypassButton_);

    masterGainAttach_ = std::make_unique<SliderAttachment>(processor_.parameters, kMasterGain, masterGainSlider_);
    masterBypassAttach_ = std::make_unique<ButtonAttachment>(processor_.parameters, kMasterBypass, masterBypassButton_);

    setSize(920, 380);
    startTimerHz(30);
}

AnalogMasterEditor::~AnalogMasterEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void AnalogMasterEditor::timerCallback() {
    auto levels = processor_.meterLevels();
    meter_.setLevels(levels.peakL, levels.peakR);
}

void AnalogMasterEditor::paint(juce::Graphics& g) {
    const auto width = static_cast<float>(getWidth());
    const auto height = static_cast<float>(getHeight());

    // 1. Brushed Dark Anodized Aluminum Background
    juce::ColourGradient panelGrad(juce::Colour(0xff22252a), 0.0f, 0.0f,
                                  juce::Colour(0xff141618), 0.0f, height, false);
    panelGrad.addColour(0.3, juce::Colour(0xff2a2e34));
    panelGrad.addColour(0.7, juce::Colour(0xff1d2024));
    g.setGradientFill(panelGrad);
    g.fillRect(0.0f, 0.0f, width, height);

    // Subtle brushed horizontal stripes
    g.setColour(juce::Colour(0x06ffffff));
    for (int y = 0; y < getHeight(); y += 2) {
        g.drawHorizontalLine(y, 0.0f, width);
    }

    // Outer rack border and chamfer
    g.setColour(juce::Colour(0xff3f3b33));
    g.drawRect(0.0f, 0.0f, width, height, 2.0f);
    g.setColour(juce::Colour(0xff111214));
    g.drawRect(2.0f, 2.0f, width - 4.0f, height - 4.0f, 1.0f);

    // 2. Corner Rack Mount Screws
    analog_master::ui::AnalogLookAndFeel::drawRackScrew(g, 18.0f, 18.0f, 8.0f);
    analog_master::ui::AnalogLookAndFeel::drawRackScrew(g, width - 18.0f, 18.0f, 8.0f);
    analog_master::ui::AnalogLookAndFeel::drawRackScrew(g, 18.0f, height - 18.0f, 8.0f);
    analog_master::ui::AnalogLookAndFeel::drawRackScrew(g, width - 18.0f, height - 18.0f, 8.0f);

    // 3. Top Header Silk-Screen
    g.setColour(juce::Colour(0xffd4b475));
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("ALAN MARCERO", 40, 14, 180, 16, juce::Justification::left);

    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("ANALOG MASTER STRIP", static_cast<int>(width * 0.5f - 150), 12, 300, 16, juce::Justification::centred);

    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xffa8977a));
    g.drawText("PRECISION SUB-BASS & AIR CONTOURING PROCESSOR", static_cast<int>(width * 0.5f - 200), 28, 400, 14, juce::Justification::centred);

    // Horizontal divider under header
    g.setColour(juce::Colour(0x55dfc488));
    g.drawLine(36.0f, 48.0f, width - 36.0f, 48.0f, 1.0f);

    // 4. Uniform Section Frames
    const float sectionY = 58.0f;
    const float sectionH = height - sectionY - 26.0f;
    const float sectionMargin = 38.0f;
    const float totalAvailW = width - sectionMargin * 2.0f;
    const float sectionSpacing = 12.0f;
    const float sectionW = (totalAvailW - sectionSpacing * 3.0f) * 0.25f;

    const char* sectionTitles[4] = {
        "LOW CUT EQ",
        "LOW STEREO FIELD",
        "HIGH CUT EQ",
        "MASTER OUTPUT"
    };

    for (int i = 0; i < 4; ++i) {
        float sx = sectionMargin + i * (sectionW + sectionSpacing);
        juce::Rectangle<float> secRect(sx, sectionY, sectionW, sectionH);

        // Section background well
        g.setColour(juce::Colour(0x33000000));
        g.fillRoundedRectangle(secRect, 5.0f);

        // Section border
        g.setColour(juce::Colour(0xff443f36));
        g.drawRoundedRectangle(secRect, 5.0f, 1.0f);

        // Title header plate
        juce::Rectangle<float> headerPlate(sx + 8.0f, sectionY + 8.0f, sectionW - 16.0f, 22.0f);
        g.setColour(juce::Colour(0x55111315));
        g.fillRoundedRectangle(headerPlate, 3.0f);

        g.setColour(juce::Colour(0xffdfc488));
        g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        g.drawText(sectionTitles[i], headerPlate.toNearestInt(), juce::Justification::centred);
    }
}

void AnalogMasterEditor::resized() {
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());
    const float sectionY = 58.0f;
    const float sectionH = height - sectionY - 26.0f;
    const float sectionMargin = 38.0f;
    const float totalAvailW = width - sectionMargin * 2.0f;
    const float sectionSpacing = 12.0f;
    const float sectionW = (totalAvailW - sectionSpacing * 3.0f) * 0.25f;

    auto getSectionRect = [&](int index) {
        float sx = sectionMargin + index * (sectionW + sectionSpacing);
        return juce::Rectangle<float>(sx, sectionY, sectionW, sectionH);
    };

    // SECTION 0: LOW CUT
    {
        auto r = getSectionRect(0).reduced(12.0f);
        r.removeFromTop(24.0f); // Space for section title

        lowCutFreqLabel_.setBounds(r.removeFromTop(18.0f).toNearestInt());
        lowCutFreqSlider_.setBounds(r.removeFromTop(95.0f).toNearestInt());

        r.removeFromTop(8.0f);
        lowCutSlopeLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        lowCutSlopeBox_.setBounds(r.removeFromTop(26.0f).reduced(10.0f, 0.0f).toNearestInt());

        r.removeFromTop(12.0f);
        lowCutBypassButton_.setBounds(r.removeFromTop(32.0f).reduced(24.0f, 0.0f).toNearestInt());
    }

    // SECTION 1: STEREO FIELD
    {
        auto r = getSectionRect(1).reduced(12.0f);
        r.removeFromTop(24.0f);

        stereoWidthLabel_.setBounds(r.removeFromTop(18.0f).toNearestInt());
        stereoWidthSlider_.setBounds(r.removeFromTop(75.0f).toNearestInt());

        stereoFreqLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        stereoFreqSlider_.setBounds(r.removeFromTop(75.0f).toNearestInt());

        r.removeFromTop(6.0f);
        auto toggleArea = r.removeFromTop(32.0f);
        stereoBypassButton_.setBounds(toggleArea.removeFromLeft(toggleArea.getWidth() * 0.5f).toNearestInt());
        stereoSoloButton_.setBounds(toggleArea.toNearestInt());
    }

    // SECTION 2: HIGH CUT
    {
        auto r = getSectionRect(2).reduced(12.0f);
        r.removeFromTop(24.0f);

        highCutFreqLabel_.setBounds(r.removeFromTop(18.0f).toNearestInt());
        highCutFreqSlider_.setBounds(r.removeFromTop(95.0f).toNearestInt());

        r.removeFromTop(8.0f);
        highCutSlopeLabel_.setBounds(r.removeFromTop(16.0f).toNearestInt());
        highCutSlopeBox_.setBounds(r.removeFromTop(26.0f).reduced(10.0f, 0.0f).toNearestInt());

        r.removeFromTop(12.0f);
        highCutBypassButton_.setBounds(r.removeFromTop(32.0f).reduced(24.0f, 0.0f).toNearestInt());
    }

    // SECTION 3: MASTER OUTPUT
    {
        auto r = getSectionRect(3).reduced(12.0f);
        r.removeFromTop(24.0f);

        meter_.setBounds(r.removeFromTop(85.0f).reduced(4.0f, 0.0f).toNearestInt());

        r.removeFromTop(8.0f);
        masterGainLabel_.setBounds(r.removeFromTop(18.0f).toNearestInt());
        masterGainSlider_.setBounds(r.removeFromTop(75.0f).toNearestInt());

        r.removeFromTop(8.0f);
        masterBypassButton_.setBounds(r.removeFromTop(32.0f).reduced(20.0f, 0.0f).toNearestInt());
    }
}
