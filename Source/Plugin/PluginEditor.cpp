#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

namespace {
constexpr const char *sectionNames[]{"01  FILTERS",  "02  TONE",  "03  STEREO",
                                     "04  DYNAMICS", "05  COLOR", "06  OUTPUT"};
constexpr const char *sectionDescriptions[]{"Clean the ends",      "Shape the balance",
                                            "Focus the low end",   "Control the movement",
                                            "Add weight and edge", "Set the final level"};
} // namespace

AnalogMasterEditor::AnalogMasterEditor(AnalogMasterProcessor &owner)
    : AudioProcessorEditor(&owner), processor_(owner) {
    setLookAndFeel(&lookAndFeel_);
    backdrop_ = juce::ImageCache::getFromMemory(BinaryData::analogcyberfaceplate_png,
                                                BinaryData::analogcyberfaceplate_pngSize);
    logo_ = juce::ImageCache::getFromMemory(BinaryData::alan_marcero_logo_jpg,
                                            BinaryData::alan_marcero_logo_jpgSize);
    knob(primary_, "low_cut_freq", "LOW CUT", 0);
    knob(primary_, "high_cut_freq", "HIGH CUT", 0);
    knob(primary_, "tone_low", "LOW SHELF", 1);
    knob(primary_, "tone_high", "HIGH SHELF", 1);
    knob(primary_, "stereo_width", "LOW WIDTH", 2);
    knob(primary_, "stereo_freq", "CROSSOVER", 2);
    knob(primary_, "comp_threshold", "COMP THRESHOLD", 3);
    knob(primary_, "dyn_amount", "DYNAMIC EQ", 3);
    knob(primary_, "sat_drive", "SATURATION", 4);
    knob(primary_, "clip_drive", "CLIP DRIVE", 4);
    knob(primary_, "limit_drive", "LIMIT DRIVE", 5);
    knob(primary_, "limit_ceiling", "CEILING", 5);
    toggle(switches_, "low_cut_in", "LOW", 0);
    toggle(switches_, "high_cut_in", "HIGH", 0);
    toggle(switches_, "tone_in", "TONE", 1);
    toggle(switches_, "stereo_in", "STEREO", 2);
    toggle(switches_, "comp_in", "COMP", 3);
    toggle(switches_, "dyn_in", "DYN EQ", 3);
    toggle(switches_, "sat_in", "SAT", 4);
    toggle(switches_, "clip_in", "CLIP", 4);
    toggle(switches_, "limit_in", "LIMITER", 5);
    toggle(header_, "master_power", "POWER");
    auto &theme = choice(header_, "theme", "");
    dynamic_cast<juce::ComboBox *>(theme.widget.get())->onChange = [this] { applyTheme(); };
    choice(header_, "meter_ref", "");
    dynamic_cast<juce::ToggleButton *>(toggle(header_, "match_bypass", "MATCH BYPASS").widget.get())
        ->setTooltip("Compare at matched short-term loudness. Attenuates the louder side after 3 seconds; "
                     "POWER switches wet/dry.");
    for (size_t i = 0; i < sections_.size(); ++i) {
        auto &button = sections_[i];
        button.setButtonText(sectionNames[i]);
        button.setComponentID("section_" + juce::String(static_cast<int>(i)));
        button.setTooltip("Show " + juce::String(sectionNames[i]).substring(4).toLowerCase() + " details");
        button.onClick = [this, i] { showDetails(static_cast<int>(i)); };
        addAndMakeVisible(button);
    }
    detailMode_.setComponentID("detail_mode");
    dynamicBand_.setComponentID("dynamic_band");
    detailMode_.onChange = [this] { showDetails(selected_); };
    dynamicBand_.addItemList({"LOW BAND", "MID BAND", "HIGH BAND"}, 1);
    dynamicBand_.setSelectedId(1, juce::dontSendNotification);
    dynamicBand_.onChange = [this] { showDetails(selected_); };
    addChildComponent(detailMode_);
    addChildComponent(dynamicBand_);
    resetButton_.onClick = [this] { processor_.resetMeters(); };
    addAndMakeVisible(resetButton_);
    addAndMakeVisible(meter_);
    setSize(1120, 720);
    showDetails(0);
    applyTheme();
    startTimerHz(30);
}
AnalogMasterEditor::~AnalogMasterEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}
AnalogMasterEditor::Control &AnalogMasterEditor::knob(std::vector<std::unique_ptr<Control>> &list,
                                                      const juce::String &id, const juce::String &title,
                                                      int section) {
    auto c = std::make_unique<Control>();
    c->section = section;
    auto slider = std::make_unique<juce::Slider>();
    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 112, 22);
    slider->setComponentID(id);
    slider->setName(title);
    slider->setTooltip(processor_.parameters.getParameter(id)->getName(100) +
                       ". Double-click to reset; click the value to type.");
    slider->setDoubleClickReturnValue(true, processor_.parameters.getParameter(id)->convertFrom0to1(
                                                processor_.parameters.getParameter(id)->getDefaultValue()));
    c->label.setText(title, juce::dontSendNotification);
    c->label.setFont(juce::FontOptions(10.f, juce::Font::bold));
    c->label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(c->label);
    addAndMakeVisible(*slider);
    c->sliderAttachment = std::make_unique<SliderAttachment>(processor_.parameters, id, *slider);
    c->widget = std::move(slider);
    list.push_back(std::move(c));
    return *list.back();
}
AnalogMasterEditor::Control &AnalogMasterEditor::toggle(std::vector<std::unique_ptr<Control>> &list,
                                                        const juce::String &id, const juce::String &title,
                                                        int section) {
    auto c = std::make_unique<Control>();
    c->section = section;
    auto button = std::make_unique<juce::ToggleButton>(title);
    button->setComponentID(id);
    button->setTooltip(processor_.parameters.getParameter(id)->getName(100));
    addAndMakeVisible(*button);
    c->buttonAttachment = std::make_unique<ButtonAttachment>(processor_.parameters, id, *button);
    c->widget = std::move(button);
    list.push_back(std::move(c));
    return *list.back();
}
AnalogMasterEditor::Control &AnalogMasterEditor::choice(std::vector<std::unique_ptr<Control>> &list,
                                                        const juce::String &id, const juce::String &title) {
    auto c = std::make_unique<Control>();
    auto combo = std::make_unique<juce::ComboBox>();
    combo->setComponentID(id);
    combo->setTooltip(processor_.parameters.getParameter(id)->getName(100));
    if (auto *p = dynamic_cast<juce::AudioParameterChoice *>(processor_.parameters.getParameter(id)))
        combo->addItemList(p->choices, 1);
    combo->setJustificationType(juce::Justification::centred);
    c->label.setText(title, juce::dontSendNotification);
    c->label.setFont(juce::FontOptions(10.f, juce::Font::bold));
    c->label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*combo);
    addAndMakeVisible(c->label);
    c->comboAttachment = std::make_unique<ComboAttachment>(processor_.parameters, id, *combo);
    c->widget = std::move(combo);
    list.push_back(std::move(c));
    return *list.back();
}
void AnalogMasterEditor::showDetails(int section) {
    if (section != selected_) {
        detailMode_.clear(juce::dontSendNotification);
        if (section == 3)
            detailMode_.addItemList({"COMPRESSOR", "DYNAMIC EQ"}, 1);
        if (section == 4)
            detailMode_.addItemList({"SATURATION", "CLIPPER"}, 1);
        detailMode_.setSelectedId(1, juce::dontSendNotification);
    }
    selected_ = section;
    details_.clear();
    detailMode_.setVisible(section == 3 || section == 4);
    dynamicBand_.setVisible(section == 3 && detailMode_.getSelectedId() == 2);
    switch (section) {
    case 0:
        choice(details_, "low_cut_slope", "LOW CUT SLOPE");
        choice(details_, "high_cut_slope", "HIGH CUT SLOPE");
        break;
    case 1:
        knob(details_, "tone_tilt", "TILT");
        knob(details_, "tone_low_hz", "LOW FREQUENCY");
        knob(details_, "tone_high_hz", "HIGH FREQUENCY");
        break;
    case 2:
        toggle(details_, "stereo_solo_low", "AUDITION LOW");
        break;
    case 3:
        if (detailMode_.getSelectedId() == 2) {
            const auto prefix = "dyn_" + juce::String(std::max(0, dynamicBand_.getSelectedItemIndex())) + "_";
            knob(details_, prefix + "freq", "FREQUENCY");
            knob(details_, prefix + "q", "Q");
            knob(details_, prefix + "threshold", "THRESHOLD");
            knob(details_, prefix + "range", "MAX CUT");
            knob(details_, prefix + "attack", "ATTACK");
            knob(details_, prefix + "release", "RELEASE");
            choice(details_, "dyn_solo", "AUDITION");
        } else {
            knob(details_, "comp_ratio", "RATIO");
            knob(details_, "comp_attack", "ATTACK");
            knob(details_, "comp_release", "RELEASE");
            knob(details_, "comp_knee", "KNEE");
            knob(details_, "comp_mix", "MIX");
            knob(details_, "comp_makeup", "MAKEUP");
            knob(details_, "comp_sc_hz", "DETECTOR HP");
        }
        break;
    case 4:
        if (detailMode_.getSelectedId() == 2) {
            choice(details_, "clip_shape", "SHAPE");
            knob(details_, "clip_ceiling", "CEILING");
            knob(details_, "clip_mix", "MIX");
        } else {
            choice(details_, "sat_character", "CHARACTER");
            knob(details_, "sat_mix", "MIX");
            knob(details_, "sat_output", "OUTPUT");
            toggle(details_, "sat_auto", "AUTO GAIN");
        }
        break;
    case 5:
        knob(details_, "limit_release", "RELEASE");
        knob(details_, "master_gain", "PRE-LIMIT TRIM");
        break;
    default:
        break;
    }
    applyTheme();
    resized();
    repaint();
}
void AnalogMasterEditor::applyTheme() {
    const int index = static_cast<int>(processor_.parameters.getRawParameterValue("theme")->load());
    lookAndFeel_.setTheme(static_cast<analog_master::ui::Theme>(juce::jlimit(0, 4, index)));
    const auto &p = lookAndFeel_.getPalette();
    meter_.setThemePalette(p);
    for (auto *list : {&primary_, &switches_, &details_, &header_})
        for (auto &c : *list) {
            c->label.setColour(juce::Label::textColourId, p.labelText);
            c->widget->lookAndFeelChanged();
        }
    for (size_t i = 0; i < sections_.size(); ++i) {
        auto &button = sections_[i];
        button.setColour(juce::TextButton::buttonColourId, p.sectionHeaderPlate);
        button.setColour(juce::TextButton::textColourOffId,
                         static_cast<int>(i) == selected_ ? p.accentPrimary : p.labelText);
    }
    resetButton_.setColour(juce::TextButton::buttonColourId, p.sectionHeaderPlate);
    resetButton_.setColour(juce::TextButton::textColourOffId, p.labelText);
    repaint();
}
void AnalogMasterEditor::timerCallback() {
    levels_ = processor_.takeMeterLevels();
    meter_.setLevels(levels_);
    const int ref =
        juce::jlimit(0, 6, static_cast<int>(processor_.parameters.getRawParameterValue("meter_ref")->load()));
    meter_.setReferenceLevel(analog_master::params::kMeterReferenceDbfs[static_cast<size_t>(ref)]);
    repaint(836, 270, 264, 382);
}
juce::Rectangle<int> AnalogMasterEditor::cardBounds(int i) const {
    return {20 + (i % 3) * 272, 80 + (i / 3) * 208, 260, 194};
}
void AnalogMasterEditor::place(Control &c, juce::Rectangle<int> area) {
    if (dynamic_cast<juce::Slider *>(c.widget.get())) {
        c.label.setBounds(area.removeFromTop(16));
        c.widget->setBounds(area);
    } else if (dynamic_cast<juce::ComboBox *>(c.widget.get())) {
        c.label.setBounds(area.getX(), area.getCentreY() - 28, area.getWidth(), 18);
        c.widget->setBounds(area.getX(), area.getCentreY() - 6, area.getWidth(), 28);
    } else
        c.widget->setBounds(area.getX(), area.getCentreY() - 16, area.getWidth(), 32);
}
void AnalogMasterEditor::resized() {
    for (int i = 0; i < 6; ++i) {
        const auto card = cardBounds(i);
        sections_[static_cast<size_t>(i)].setBounds(card.reduced(8).removeFromTop(24));
        for (int k = 0; k < 2; ++k)
            place(*primary_[static_cast<size_t>(2 * i + k)],
                  {card.getX() + 10 + k * 122, card.getY() + 39, 118, 116});
        int n = 0;
        for (auto &c : switches_)
            if (c->section == i) {
                c->widget->setBounds(card.getX() + 12 + n * 122, card.getBottom() - 33, 114, 27);
                ++n;
            }
    }
    header_[0]->widget->setBounds(702, 22, 120, 30);
    header_[1]->widget->setBounds(948, 22, 152, 30);
    header_[2]->widget->setBounds(850, 247, 236, 26);
    header_[3]->widget->setBounds(850, 624, 236, 32);
    meter_.setBounds(850, 94, 236, 148);
    resetButton_.setBounds(850, 662, 236, 26);
    detailMode_.setBounds(380, 507, 180, 26);
    dynamicBand_.setBounds(576, 507, 228, 26);
    const int count = static_cast<int>(details_.size());
    const int width = count > 0 ? std::min(190, 776 / count) : 1;
    for (int i = 0; i < count; ++i)
        place(*details_[static_cast<size_t>(i)], {32 + i * width, 548, width - 8, 122});
}
void AnalogMasterEditor::paint(juce::Graphics &g) {
    const auto &p = lookAndFeel_.getPalette();
    g.fillAll(p.panelGradBottom);
    if (backdrop_.isValid()) {
        g.setOpacity(.68f);
        g.drawImage(backdrop_, getLocalBounds().toFloat());
        g.setOpacity(1);
    }
    g.setColour(juce::Colours::black.withAlpha(.3f));
    g.fillRect(getLocalBounds());
    if (logo_.isValid())
        g.drawImageWithin(logo_, 20, 17, 38, 38, juce::RectanglePlacement::centred);
    g.setColour(p.headerTitle);
    g.setFont(juce::FontOptions(27.f, juce::Font::bold));
    g.drawText("openMaster", 70, 12, 245, 34, juce::Justification::centredLeft);
    g.setColour(p.accentPrimary);
    g.setFont(juce::FontOptions(10.f, juce::Font::bold));
    g.drawText("STUDIO 84  /  MASTERING PROCESSOR", 72, 45, 300, 17, juce::Justification::centredLeft);
    g.setColour(p.headerSub);
    g.drawText("FACEPLATE", 847, 25, 90, 23, juce::Justification::centredRight);
    for (int i = 0; i < 6; ++i) {
        const auto card = cardBounds(i).toFloat();
        g.setColour(p.sectionWellBg.withAlpha(.97f));
        g.fillRoundedRectangle(card, 4);
        g.setColour(i == selected_ ? p.accentPrimary.withAlpha(.65f) : p.sectionBorder);
        g.drawRoundedRectangle(card, 4, 1);
    }
    g.setColour(p.sectionWellBg);
    g.fillRoundedRectangle({20.f, 498.f, 804.f, 198.f}, 4);
    g.fillRoundedRectangle({836.f, 80.f, 264.f, 616.f}, 4);
    g.setColour(p.sectionBorder);
    g.drawRoundedRectangle({20.f, 498.f, 804.f, 198.f}, 4, 1);
    g.drawRoundedRectangle({836.f, 80.f, 264.f, 616.f}, 4, 1);
    g.setColour(p.accentPrimary);
    g.setFont(juce::FontOptions(12.f, juce::Font::bold));
    g.drawText(juce::String(sectionNames[selected_]).substring(4) + " / DETAIL", 34, 507, 325, 24,
               juce::Justification::centredLeft);
    g.setColour(p.headerSub);
    g.setFont(juce::FontOptions(10.f));
    juce::String hint = sectionDescriptions[selected_];
    if (selected_ == 0)
        hint = "Choose each filter's slope. LOW and HIGH switches engage them independently.";
    if (selected_ == 2)
        hint = "Width affects the low band only. Audition the crossover with AUDITION LOW.";
    if (selected_ == 4)
        hint = "8x oversampling  /  Drive into the curve; use MIX to blend.";
    if (selected_ == 5)
        hint = "5 ms lookahead  /  Trim precedes clipping and limiting.";
    g.drawText(hint, 34, 674, 774, 14, juce::Justification::centredLeft);
    const auto readout = [&](int y, const char *label, float value, const char *units, juce::Colour colour) {
        g.setColour(juce::Colour(0xff080e0b));
        g.fillRoundedRectangle({850.f, static_cast<float>(y), 236.f, 62.f}, 3);
        g.setColour(p.labelText);
        g.setFont(juce::FontOptions(10.f, juce::Font::bold));
        g.drawText(label, 860, y + 5, 164, 15, juce::Justification::centredLeft);
        g.drawText(units, 1037, y + 35, 39, 16, juce::Justification::centredRight);
        analog_master::ui::drawLedNumber(g, {860.f, static_cast<float>(y + 26), 167.f, 28.f},
                                         std::isfinite(value) ? juce::String(value, 1) : "---", colour);
    };
    readout(285, "INTEGRATED", levels_.integratedLufs, "LUFS", p.accentPrimary);
    readout(355, "SHORT TERM", levels_.shortTermLufs, "LUFS", p.accentPrimary);
    readout(425, "TRUE PEAK / MAX", levels_.truePeakDbtp, "dBTP", p.accentSecondary);
    const float gr[]{levels_.compressorGr, levels_.dynamicEqGr, levels_.clipperGr, levels_.limiterGr};
    const char *names[]{"COMP", "DYN EQ", "CLIP", "LIMIT"};
    for (int i = 0; i < 4; ++i) {
        const int y = 505 + i * 23;
        g.setColour(p.labelText);
        g.setFont(juce::FontOptions(9.f, juce::Font::bold));
        g.drawText(names[i], 851, y, 48, 16, juce::Justification::centredLeft);
        for (int j = 0; j < 18; ++j) {
            g.setColour(gr[i] > static_cast<float>(j) * .5f ? p.accentSecondary
                                                            : p.accentSecondary.withAlpha(.10f));
            g.fillRect(902 + j * 7, y + 3, 5, 9);
        }
        g.setColour(p.accentSecondary);
        g.drawText(juce::String(gr[i], 1), 1033, y, 52, 16, juce::Justification::centredRight);
    }
    g.setColour(p.headerSub);
    g.setFont(juce::FontOptions(9.f));
    const bool match = processor_.parameters.getRawParameterValue("match_bypass")->load() > .5f;
    const juce::String status = !match               ? "GAIN REDUCTION / dB"
                                : levels_.matchReady ? "MATCH " + juce::String(levels_.matchDb, 1) + " dB"
                                                     : "MATCH / LISTENING...";
    g.drawText(status, 850, 601, 236, 16, juce::Justification::centred);
}
