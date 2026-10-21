#include "AnalogLookAndFeel.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace analog_master::ui {

void drawLedNumber(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& value, juce::Colour colour) {
    constexpr int masks[] {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
    const float height = area.getHeight();
    const float width = height * 0.47f;
    const float stroke = height * 0.095f;
    float units = 0.0f;
    for (auto character : value) units += character == '.' ? 0.4f : 1.0f;
    float x = area.getRight() - units * (width + 3.0f);
    const float y = area.getY();
    for (auto character : value) {
        if (character == '.') {
            g.setColour(colour);
            g.fillRect(x, area.getBottom() - stroke, stroke, stroke);
            x += (width + 3.0f) * 0.4f;
            continue;
        }
        const int mask = character >= '0' && character <= '9' ? masks[character - '0'] : 0x40;
        const juce::Rectangle<float> segments[] {
            {x + stroke, y, width - stroke * 2, stroke},
            {x + width - stroke, y + stroke, stroke, height * 0.5f - stroke * 1.5f},
            {x + width - stroke, y + height * 0.5f + stroke * 0.5f, stroke, height * 0.5f - stroke * 1.5f},
            {x + stroke, y + height - stroke, width - stroke * 2, stroke},
            {x, y + height * 0.5f + stroke * 0.5f, stroke, height * 0.5f - stroke * 1.5f},
            {x, y + stroke, stroke, height * 0.5f - stroke * 1.5f},
            {x + stroke, y + height * 0.5f - stroke * 0.5f, width - stroke * 2, stroke}
        };
        for (int segment = 0; segment < 7; ++segment) {
            g.setColour(colour.withMultipliedAlpha((mask & (1 << segment)) != 0 ? 1.0f : 0.07f));
            g.fillRoundedRectangle(segments[segment], stroke * 0.25f);
        }
        if (character == '+') {
            g.setColour(colour);
            g.fillRect(x + width * 0.5f - stroke * 0.5f, y + height * 0.25f, stroke, height * 0.5f);
        }
        x += width + 3.0f;
    }
}

ThemePalette AnalogLookAndFeel::createPalette(Theme theme) {
    ThemePalette p;
    p.accentPrimary = juce::Colour(0xff90ee63);
    p.accentSecondary = juce::Colour(0xffffbd4a);
    switch (theme) {
        case Theme::Silver: p.accentPrimary = juce::Colour(0xffdce4dc); break;
        case Theme::Phosphor: p.accentSecondary = juce::Colour(0xffc9ef7c); break;
        case Theme::Ice: p.accentPrimary = juce::Colour(0xff8dd8ed); break;
        case Theme::Amber:
            p.accentPrimary = juce::Colour(0xffffbd4a);
            p.accentSecondary = juce::Colour(0xffff795b);
            break;
        case Theme::Studio84: break;
    }

    p.panelGradTop = juce::Colour(0xff272a28);
    p.panelGradMid1 = juce::Colour(0xff202321);
    p.panelGradMid2 = juce::Colour(0xff171a18);
    p.panelGradBottom = juce::Colour(0xff101210);
    p.panelStripe = juce::Colour(0xff242824);
    p.panelBorder = juce::Colour(0xff090b09);
    p.panelInnerBorder = juce::Colour(0xff3b403b);
    p.hudGrid = juce::Colour(0xff344537);
    p.screwTop = juce::Colour(0xff50564f);
    p.screwBottom = juce::Colour(0xff1b1e1a);
    p.screwBorder = juce::Colour(0xff62685f);
    p.screwSlot = juce::Colour(0xff080a08);
    p.headerBrand = juce::Colour(0xffc0c5b8);
    p.headerTitle = juce::Colour(0xffe6e9db);
    p.headerSub = juce::Colour(0xff969e91);
    p.headerDivider = juce::Colour(0xff41473e);
    p.sectionWellBg = juce::Colour(0xff191c19);
    p.sectionBorder = juce::Colour(0xff3a4038);
    p.sectionHeaderPlate = juce::Colour(0xff1f231f);
    p.sectionTitleText = juce::Colour(0xffe6e9db);
    p.labelText = juce::Colour(0xffbac2b2);
    p.knobTick = juce::Colour(0xff8b9484);
    p.knobDropShadow = juce::Colour(0xff060806);
    p.knobCollarTop = juce::Colour(0xff454b42);
    p.knobCollarMid = juce::Colour(0xff282d27);
    p.knobCollarBottom = juce::Colour(0xff0d100d);
    p.knobCollarBorder = juce::Colour(0xff596051);
    p.knobCoreTop = juce::Colour(0xff30362d);
    p.knobCoreMid1 = juce::Colour(0xff272d24);
    p.knobCoreMid2 = juce::Colour(0xff1c211a);
    p.knobCoreBottom = juce::Colour(0xff11150f);
    p.knobCoreBorder = juce::Colour(0xff4a5243);
    p.knobPointer = juce::Colour(0xffe7edcf);
    p.knobPointerShadow = juce::Colour(0xff060806);
    p.switchPlateBg = juce::Colour(0xff222720);
    p.switchPlateBorder = juce::Colour(0xff535c4b);
    p.switchSlotBg = juce::Colour(0xff080c08);
    p.switchLeverTop = juce::Colour(0xff343c2e);
    p.switchLeverBottom = juce::Colour(0xff1c2218);
    p.switchLeverBorder = juce::Colour(0xff5a6350);
    p.switchActiveText = juce::Colour(0xffe7eddb);
    p.switchInactiveText = juce::Colour(0xff9da692);
    p.jewelBezelBg = juce::Colour(0xff10150f);
    p.jewelBezelBorder = juce::Colour(0xff414b37);
    p.jewelCoreOff = juce::Colour(0xff192115);
    p.jewelEdgeOff = juce::Colour(0xff10150d);
    p.comboBg = juce::Colour(0xff090f09);
    p.comboBorder = juce::Colour(0xff3d4935);
    p.comboArrow = juce::Colour(0xff919e84);
    p.popupBg = juce::Colour(0xff151c12);
    p.popupHighlightBg = juce::Colour(0xff2b3d20);
    p.meterFrameBg = juce::Colour(0xff070b07);
    p.meterFrameBorder = juce::Colour(0xff46503e);
    p.meterFaceTop = juce::Colour(0xff16211c);
    p.meterFaceMid = juce::Colour(0xff101910);
    p.meterFaceBottom = juce::Colour(0xff070c09);
    p.meterFaceShadow = juce::Colour(0xff020502);
    p.meterScaleText = juce::Colour(0xffb6c3ab);
    p.meterRedZone = juce::Colour(0xffff5544);
    p.jewelHalo = p.accentPrimary.withAlpha(0.15f);
    p.jewelCoreOn = p.accentPrimary;
    p.jewelEdgeOn = p.accentPrimary.darker(0.35f);
    p.comboText = p.accentPrimary;
    p.popupHighlightText = p.accentPrimary;
    p.meterNeedle = p.accentPrimary;
    p.meterNeedleGlow = p.accentPrimary.withAlpha(0.15f);
    return p;
}

AnalogLookAndFeel::AnalogLookAndFeel() {
    setTheme(Theme::Studio84);
}

void AnalogLookAndFeel::setTheme(Theme theme) {
    theme_ = theme;
    palette_ = createPalette(theme);

    setColour(juce::Slider::textBoxTextColourId, palette_.accentPrimary);
    setColour(juce::Slider::textBoxBackgroundColourId, palette_.comboBg);
    setColour(juce::Slider::textBoxOutlineColourId, palette_.sectionBorder);

    setColour(juce::ComboBox::backgroundColourId, palette_.comboBg);
    setColour(juce::ComboBox::textColourId, palette_.comboText);
    setColour(juce::ComboBox::outlineColourId, palette_.comboBorder);
    setColour(juce::ComboBox::arrowColourId, palette_.comboArrow);

    setColour(juce::PopupMenu::backgroundColourId, palette_.popupBg);
    setColour(juce::PopupMenu::textColourId, palette_.comboText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, palette_.popupHighlightBg);
    setColour(juce::PopupMenu::highlightedTextColourId, palette_.popupHighlightText);
}

void AnalogLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider
) {
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));

    constexpr float kLabelMargin = 21.0f;
    const float radius = std::max(8.0f, std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f - kLabelMargin);
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    auto pointAt = [&](float r, float angle) {
        return juce::Point<float>(centreX + r * std::sin(angle), centreY - r * std::cos(angle));
    };

    // Discrete illuminated indices echo the meter's LED segments.
    constexpr int kMinorTicks = 31;
    for (int i = 0; i < kMinorTicks; ++i) {
        const float norm = static_cast<float>(i) / static_cast<float>(kMinorTicks - 1);
        const float angle = rotaryStartAngle + norm * (rotaryEndAngle - rotaryStartAngle);
        const bool lit = norm <= sliderPosProportional + 0.002f;
        g.setColour(lit ? palette_.accentPrimary.withAlpha(0.9f) : palette_.accentPrimary.withAlpha(0.1f));
        g.drawLine(juce::Line<float>(pointAt(radius + 1.0f, angle), pointAt(radius + 5.0f, angle)), 2.0f);
    }

    // 2. Scale labels, placed through the slider's skewed range
    const auto* scaleValues = slider.getProperties()["scaleValues"].getArray();
    const auto* scaleLabels = slider.getProperties()["scaleLabels"].getArray();
    if (scaleValues != nullptr && scaleLabels != nullptr) {
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        for (int i = 0; i < std::min(scaleValues->size(), scaleLabels->size()); ++i) {
            const double value = static_cast<double>((*scaleValues)[i]);
            const float norm = static_cast<float>(slider.valueToProportionOfLength(value));
            const float angle = rotaryStartAngle + norm * (rotaryEndAngle - rotaryStartAngle);
            const bool lit = norm <= sliderPosProportional + 0.002f;

            g.setColour(palette_.knobTick);
            g.drawLine(juce::Line<float>(pointAt(radius + 7.0f, angle), pointAt(radius + 10.0f, angle)), 1.0f);

            g.setColour(lit ? palette_.headerTitle : palette_.knobTick);
            g.drawText((*scaleLabels)[i].toString(),
                       juce::Rectangle<float>(34.0f, 10.0f).withCentre(pointAt(radius + 15.5f, angle)),
                       juce::Justification::centred, false);
        }
    }

    // Fluted black cap with a recessed skirt and a silk-screened pointer.
    const float bodyRadius = radius * 0.88f;
    const auto body = juce::Rectangle<float>(bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre({ centreX, centreY });

    g.setColour(palette_.knobDropShadow);
    g.fillEllipse(body.translated(1.5f, 3.0f));

    juce::ColourGradient collarGrad(palette_.knobCollarTop, body.getX(), body.getY(),
                                    palette_.knobCollarBottom, body.getRight(), body.getBottom(), false);
    collarGrad.addColour(0.4, palette_.knobCollarMid);
    g.setGradientFill(collarGrad);
    g.fillEllipse(body);
    g.setColour(palette_.knobCollarBorder);
    g.drawEllipse(body, 0.8f);

    constexpr int kGrooves = 40;
    g.setColour(juce::Colours::black.withAlpha(0.65f));
    for (int i = 0; i < kGrooves; ++i) {
        const float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(kGrooves);
        g.drawLine(juce::Line<float>(pointAt(bodyRadius * 0.76f, angle), pointAt(bodyRadius * 0.96f, angle)), 1.3f);
    }

    const float capRadius = bodyRadius * 0.73f;
    const auto cap = juce::Rectangle<float>(capRadius * 2.0f, capRadius * 2.0f).withCentre({ centreX, centreY });
    juce::ColourGradient coreGrad(palette_.knobCoreTop, cap.getX(), cap.getY(),
                                  palette_.knobCoreBottom, cap.getRight(), cap.getBottom(), false);
    coreGrad.addColour(0.35, palette_.knobCoreMid1);
    coreGrad.addColour(0.75, palette_.knobCoreMid2);
    g.setGradientFill(coreGrad);
    g.fillEllipse(cap);
    g.setColour(palette_.knobCoreBorder);
    g.drawEllipse(cap, 0.7f);

    // 5. Pointer
    const juce::Line<float> pointer(pointAt(capRadius * 0.35f, currentAngle), pointAt(bodyRadius * 0.88f, currentAngle));
    g.setColour(palette_.knobPointerShadow);
    g.drawLine(pointer, 3.8f);
    g.setColour(palette_.knobPointer);
    g.drawLine(pointer, 2.4f);
}

void AnalogLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown
) {
    const auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const bool isOn = button.getToggleState();
    const auto& props = button.getProperties();
    const juce::Colour accent = props["useSecondaryAccent"] ? palette_.accentSecondary : palette_.accentPrimary;

    // The text names the current state
    const juce::String text = isOn ? props.getWithDefault("onText", button.getButtonText()).toString()
                                   : props.getWithDefault("offText", button.getButtonText()).toString();

    g.setColour(palette_.switchSlotBg);
    g.fillRoundedRectangle(bounds, 2.0f);
    auto face = bounds.reduced(2.0f).translated(0.0f, shouldDrawButtonAsDown ? 1.0f : 0.0f);
    auto top = palette_.switchLeverTop;
    if (shouldDrawButtonAsHighlighted) top = top.brighter(0.12f);
    g.setGradientFill(juce::ColourGradient(top, face.getTopLeft(), palette_.switchLeverBottom, face.getBottomLeft(), false));
    g.fillRoundedRectangle(face, 1.0f);
    g.setColour(button.hasKeyboardFocus(true) ? accent : palette_.switchPlateBorder);
    g.drawRoundedRectangle(face, 1.0f, 0.8f);

    const auto led = juce::Rectangle<float>(7.0f, 4.0f).withCentre({ bounds.getX() + 13.0f, bounds.getCentreY() });
    g.setColour(juce::Colours::black);
    g.fillRect(led.expanded(1.5f));
    if (isOn) {
        g.setColour(accent.withAlpha(0.12f));
        g.fillRoundedRectangle(led.expanded(2.0f), 2.0f);
    }
    g.setColour(accent.withAlpha(isOn ? 1.0f : 0.12f));
    g.fillRect(led);

    g.setColour(isOn ? palette_.switchActiveText : palette_.switchInactiveText);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawFittedText(text, face.withTrimmedLeft(20.0f).withTrimmedRight(4.0f).toNearestInt(), juce::Justification::centred, 1);
}

void AnalogLookAndFeel::drawRackScrew(juce::Graphics& g, float centerX, float centerY, float radius) const {
    drawRackScrew(g, centerX, centerY, radius, palette_);
}

void AnalogLookAndFeel::drawRackScrew(juce::Graphics& g, float centerX, float centerY, float radius, const ThemePalette& palette) {
    const float x = centerX - radius;
    const float y = centerY - radius;
    const float d = radius * 2.0f;

    g.setColour(juce::Colour(0xaa000000));
    g.fillEllipse(x + 1.0f, y + 1.5f, d, d);

    juce::ColourGradient screwGrad(palette.screwTop, x, y,
                                  palette.screwBottom, x + d, y + d, false);
    g.setGradientFill(screwGrad);
    g.fillEllipse(x, y, d, d);

    g.setColour(palette.screwBorder);
    g.drawEllipse(x, y, d, d, 1.0f);

    // Recessed hex socket
    juce::Path hex;
    const float hr = radius * 0.52f;
    for (int i = 0; i < 6; ++i) {
        const float a = static_cast<float>(i) * (std::numbers::pi_v<float> / 3.0f);
        const float hx = centerX + hr * std::cos(a);
        const float hy = centerY + hr * std::sin(a);
        if (i == 0) hex.startNewSubPath(hx, hy);
        else hex.lineTo(hx, hy);
    }
    hex.closeSubPath();
    g.setColour(palette.screwSlot);
    g.fillPath(hex);
    g.setColour(palette.screwTop);
    g.strokePath(hex, juce::PathStrokeType(0.8f));
}

void AnalogLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width, int height,
    bool /*isButtonDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& box
) {
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height)).reduced(1.0f);

    g.setColour(palette_.comboBg);
    g.fillRoundedRectangle(bounds, 1.5f);
    g.setColour(box.hasKeyboardFocus(true) ? palette_.accentPrimary : palette_.comboBorder);
    g.drawRoundedRectangle(bounds, 1.5f, 1.0f);
    g.setColour(palette_.sectionBorder);
    g.drawVerticalLine(width - 24, 5.0f, static_cast<float>(height - 5));

    const float arrowX = static_cast<float>(width - 15);
    const float arrowY = static_cast<float>(height) * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowX, arrowY - 2.5f, arrowX + 7.5f, arrowY - 2.5f, arrowX + 3.75f, arrowY + 3.0f);
    g.setColour(palette_.comboArrow);
    g.fillPath(arrow);
}

void AnalogLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
    g.setColour(palette_.popupBg);
    g.fillRect(bounds);
    g.setColour(palette_.comboBorder);
    g.drawRect(bounds, 1.0f);
}

juce::Font AnalogLookAndFeel::getComboBoxFont(juce::ComboBox&) {
    return juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain);
}

juce::Label* AnalogLookAndFeel::createSliderTextBox(juce::Slider& slider) {
    auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->getProperties().set("ledReadout", true);
    label->setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    label->setColour(juce::Label::textWhenEditingColourId, palette_.accentPrimary);
    label->setColour(juce::Label::backgroundWhenEditingColourId, palette_.comboBg);
    label->setColour(juce::Label::outlineWhenEditingColourId, palette_.accentPrimary);
    label->setColour(juce::TextEditor::highlightColourId, palette_.accentPrimary.withAlpha(0.25f));
    label->setColour(juce::TextEditor::highlightedTextColourId, palette_.headerTitle);
    label->setColour(juce::CaretComponent::caretColourId, palette_.accentPrimary);
    return label;
}

void AnalogLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label) {
    if (!static_cast<bool>(label.getProperties()["ledReadout"]) || label.isBeingEdited()) {
        juce::LookAndFeel_V4::drawLabel(g, label);
        return;
    }

    const auto bounds = label.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(palette_.comboBg);
    g.fillRoundedRectangle(bounds, 1.5f);
    g.setColour(label.isMouseOver() ? palette_.accentPrimary.withAlpha(0.65f) : palette_.comboBorder);
    g.drawRoundedRectangle(bounds, 1.5f, 1.0f);

    const auto text = label.getText();
    int numberEnd = 0;
    while (numberEnd < text.length() && juce::String("0123456789.+-").containsChar(text[numberEnd])) ++numberEnd;
    const auto number = text.substring(0, numberEnd);
    const auto unit = text.substring(numberEnd).trim();
    const juce::Font unitFont(juce::FontOptions(9.0f));
    const float unitWidth = juce::GlyphArrangement::getStringWidth(unitFont, unit);
    const float digitHeight = 14.0f;
    float digitUnits = 0.0f;
    for (auto c : number) digitUnits += c == '.' ? 0.4f : 1.0f;
    const float numberWidth = digitUnits * (digitHeight * 0.47f + 3.0f);
    const float left = bounds.getCentreX() - (numberWidth + 6.0f + unitWidth) * 0.5f;
    const float alpha = label.isEnabled() ? 1.0f : 0.4f;
    drawLedNumber(g, {left, bounds.getCentreY() - digitHeight * 0.5f, numberWidth, digitHeight}, number, palette_.accentPrimary.withAlpha(alpha));
    g.setColour(palette_.labelText.withAlpha(alpha));
    g.setFont(unitFont);
    g.drawText(unit, juce::Rectangle<float>(left + numberWidth + 6.0f, bounds.getY(), unitWidth + 1.0f, bounds.getHeight()), juce::Justification::centredLeft);
}

} // namespace analog_master::ui
