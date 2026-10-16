#include "AnalogLookAndFeel.h"
#include <cmath>
#include <numbers>

namespace analog_master::ui {

ThemePalette AnalogLookAndFeel::createPalette(Theme theme) {
    ThemePalette p;
    switch (theme) {
        case Theme::Silverface: {
            p.panelGradTop = juce::Colour(0xffd8dce2);
            p.panelGradMid1 = juce::Colour(0xffe4e8ee);
            p.panelGradMid2 = juce::Colour(0xffcdd2d8);
            p.panelGradBottom = juce::Colour(0xffbec3ca);
            p.panelStripe = juce::Colour(0x18ffffff);
            p.panelBorder = juce::Colour(0xff8c929a);
            p.panelInnerBorder = juce::Colour(0x66ffffff);

            p.screwTop = juce::Colour(0xffb0b5bc);
            p.screwBottom = juce::Colour(0xff686d75);
            p.screwBorder = juce::Colour(0xffe8ecf0);
            p.screwSlot = juce::Colour(0xff2a2e33);

            p.headerBrand = juce::Colour(0xff162230);
            p.headerTitle = juce::Colour(0xff0e1722);
            p.headerSub = juce::Colour(0xff3a4858);
            p.headerDivider = juce::Colour(0x44162230);

            p.sectionWellBg = juce::Colour(0x14000000);
            p.sectionBorder = juce::Colour(0xff8a929c);
            p.sectionHeaderPlate = juce::Colour(0x221a2430);
            p.sectionTitleText = juce::Colour(0xff162230);
            p.labelText = juce::Colour(0xff1e2a38);

            p.knobTick = juce::Colour(0xcc1e2a38);
            p.knobDropShadow = juce::Colour(0x44000000);
            p.knobCollarTop = juce::Colour(0xffe4e8ec);
            p.knobCollarMid = juce::Colour(0xffbcc3cb);
            p.knobCollarBottom = juce::Colour(0xff949ba4);
            p.knobCollarBorder = juce::Colour(0xff68707a);
            p.knobCoreTop = juce::Colour(0xff343a42);
            p.knobCoreMid1 = juce::Colour(0xff4a525e);
            p.knobCoreMid2 = juce::Colour(0xff22262c);
            p.knobCoreBottom = juce::Colour(0xff14161a);
            p.knobCoreBorder = juce::Colour(0xff7a8490);
            p.knobPointer = juce::Colour(0xff00d4ff);
            p.knobPointerShadow = juce::Colour(0x99000000);

            p.switchPlateBg = juce::Colour(0xffc8ccd2);
            p.switchPlateBorder = juce::Colour(0xff7a828c);
            p.switchSlotBg = juce::Colour(0xff202428);
            p.switchLeverTop = juce::Colour(0xffffffff);
            p.switchLeverBottom = juce::Colour(0xff8a9098);
            p.switchLeverBorder = juce::Colour(0xff555c64);
            p.switchActiveText = juce::Colour(0xff0e1722);
            p.switchInactiveText = juce::Colour(0xff6a7480);

            p.jewelBezelBg = juce::Colour(0xff889098);
            p.jewelBezelBorder = juce::Colour(0xffffffff);
            p.jewelHalo = juce::Colour(0x440088ff);
            p.jewelCoreOn = juce::Colour(0xffd6f0ff);
            p.jewelEdgeOn = juce::Colour(0xff0066ee);
            p.jewelCoreOff = juce::Colour(0xff0b1828);
            p.jewelEdgeOff = juce::Colour(0xff040a12);

            p.comboBg = juce::Colour(0xffe0e4ea);
            p.comboBorder = juce::Colour(0xff8a929c);
            p.comboText = juce::Colour(0xff162230);
            p.comboArrow = juce::Colour(0xff1e2a38);
            p.popupBg = juce::Colour(0xffe8ecf2);
            p.popupHighlightBg = juce::Colour(0xffccd4de);
            p.popupHighlightText = juce::Colour(0xff0e1722);

            p.meterFrameBg = juce::Colour(0xff1e2228);
            p.meterFrameBorder = juce::Colour(0xff8a929c);
            p.meterFaceTop = juce::Colour(0xfff5f8fa);
            p.meterFaceMid = juce::Colour(0xffe4eaef);
            p.meterFaceBottom = juce::Colour(0xffd2d8e0);
            p.meterFaceShadow = juce::Colour(0x22000000);
            p.meterScaleText = juce::Colour(0xff141820);
            p.meterRedZone = juce::Colour(0xffcc2222);
            p.meterNeedle = juce::Colour(0xff0e141c);
            p.meterNeedleGlow = juce::Colour(0x440088ff);
            break;
        }
        case Theme::BritishConsole: {
            p.panelGradTop = juce::Colour(0xff1a3024);
            p.panelGradMid1 = juce::Colour(0xff223d2e);
            p.panelGradMid2 = juce::Colour(0xff16281e);
            p.panelGradBottom = juce::Colour(0xff0d1812);
            p.panelStripe = juce::Colour(0x0a000000);
            p.panelBorder = juce::Colour(0xff8a7538);
            p.panelInnerBorder = juce::Colour(0xff09100c);

            p.screwTop = juce::Colour(0xffc8a855);
            p.screwBottom = juce::Colour(0xff58441c);
            p.screwBorder = juce::Colour(0xffdfc477);
            p.screwSlot = juce::Colour(0xff181006);

            p.headerBrand = juce::Colour(0xfff2e6c8);
            p.headerTitle = juce::Colour(0xffe8d8b0);
            p.headerSub = juce::Colour(0xffa8b8a0);
            p.headerDivider = juce::Colour(0x55d4b870);

            p.sectionWellBg = juce::Colour(0x38000000);
            p.sectionBorder = juce::Colour(0xff4c6850);
            p.sectionHeaderPlate = juce::Colour(0x660a140e);
            p.sectionTitleText = juce::Colour(0xffe8d8b0);
            p.labelText = juce::Colour(0xffeddcb4);

            p.knobTick = juce::Colour(0xbbe8d8b0);
            p.knobDropShadow = juce::Colour(0x66000000);
            p.knobCollarTop = juce::Colour(0xffc8a855);
            p.knobCollarMid = juce::Colour(0xff8a6e30);
            p.knobCollarBottom = juce::Colour(0xff5c4820);
            p.knobCollarBorder = juce::Colour(0xffa08238);
            p.knobCoreTop = juce::Colour(0xffdfc477);
            p.knobCoreMid1 = juce::Colour(0xfffaecc0);
            p.knobCoreMid2 = juce::Colour(0xffa8883e);
            p.knobCoreBottom = juce::Colour(0xff7a5c24);
            p.knobCoreBorder = juce::Colour(0xff503c14);
            p.knobPointer = juce::Colour(0xfffff8e8);
            p.knobPointerShadow = juce::Colour(0xbb181006);

            p.switchPlateBg = juce::Colour(0xff102016);
            p.switchPlateBorder = juce::Colour(0xff6a8060);
            p.switchSlotBg = juce::Colour(0xff080e0a);
            p.switchLeverTop = juce::Colour(0xffe0c880);
            p.switchLeverBottom = juce::Colour(0xff786028);
            p.switchLeverBorder = juce::Colour(0xffa89050);
            p.switchActiveText = juce::Colour(0xfff2e6c8);
            p.switchInactiveText = juce::Colour(0xff788c7c);

            p.jewelBezelBg = juce::Colour(0xff6a5830);
            p.jewelBezelBorder = juce::Colour(0xffa89050);
            p.jewelHalo = juce::Colour(0x55ff2233);
            p.jewelCoreOn = juce::Colour(0xffffd0d4);
            p.jewelEdgeOn = juce::Colour(0xffee1122);
            p.jewelCoreOff = juce::Colour(0xff38080c);
            p.jewelEdgeOff = juce::Colour(0xff160305);

            p.comboBg = juce::Colour(0xff122218);
            p.comboBorder = juce::Colour(0xff5c7860);
            p.comboText = juce::Colour(0xfff2e6c8);
            p.comboArrow = juce::Colour(0xffdfc477);
            p.popupBg = juce::Colour(0xff0e1c14);
            p.popupHighlightBg = juce::Colour(0xff244430);
            p.popupHighlightText = juce::Colour(0xffffffff);

            p.meterFrameBg = juce::Colour(0xff0c1610);
            p.meterFrameBorder = juce::Colour(0xff7a6530);
            p.meterFaceTop = juce::Colour(0xfff9f5e8);
            p.meterFaceMid = juce::Colour(0xffede3cc);
            p.meterFaceBottom = juce::Colour(0xffded2b4);
            p.meterFaceShadow = juce::Colour(0x33000000);
            p.meterScaleText = juce::Colour(0xff201a12);
            p.meterRedZone = juce::Colour(0xffb81e1e);
            p.meterNeedle = juce::Colour(0xff201608);
            p.meterNeedleGlow = juce::Colour(0x44ff2233);
            break;
        }
        case Theme::MidnightSlate: {
            p.panelGradTop = juce::Colour(0xff131a26);
            p.panelGradMid1 = juce::Colour(0xff1a2434);
            p.panelGradMid2 = juce::Colour(0xff111722);
            p.panelGradBottom = juce::Colour(0xff0a0d14);
            p.panelStripe = juce::Colour(0x0cffffff);
            p.panelBorder = juce::Colour(0xff2e3d54);
            p.panelInnerBorder = juce::Colour(0xff080b10);

            p.screwTop = juce::Colour(0xff485466);
            p.screwBottom = juce::Colour(0xff1c222c);
            p.screwBorder = juce::Colour(0xff6c7d96);
            p.screwSlot = juce::Colour(0xff0b0e14);

            p.headerBrand = juce::Colour(0xff60d0ff);
            p.headerTitle = juce::Colour(0xffe4f4ff);
            p.headerSub = juce::Colour(0xff82a4c4);
            p.headerDivider = juce::Colour(0x5530a8df);

            p.sectionWellBg = juce::Colour(0x35000000);
            p.sectionBorder = juce::Colour(0xff283850);
            p.sectionHeaderPlate = juce::Colour(0x66080e18);
            p.sectionTitleText = juce::Colour(0xff60d0ff);
            p.labelText = juce::Colour(0xffb8dcf4);

            p.knobTick = juce::Colour(0xaa50c4f8);
            p.knobDropShadow = juce::Colour(0x77000000);
            p.knobCollarTop = juce::Colour(0xff283240);
            p.knobCollarMid = juce::Colour(0xff1c2430);
            p.knobCollarBottom = juce::Colour(0xff10141c);
            p.knobCollarBorder = juce::Colour(0xff3d4a5e);
            p.knobCoreTop = juce::Colour(0xff2a3444);
            p.knobCoreMid1 = juce::Colour(0xff3e4c62);
            p.knobCoreMid2 = juce::Colour(0xff202836);
            p.knobCoreBottom = juce::Colour(0xff141a24);
            p.knobCoreBorder = juce::Colour(0xff35455d);
            p.knobPointer = juce::Colour(0xff00f0ff);
            p.knobPointerShadow = juce::Colour(0xaa000000);

            p.switchPlateBg = juce::Colour(0xff0f1520);
            p.switchPlateBorder = juce::Colour(0xff2e405a);
            p.switchSlotBg = juce::Colour(0xff06090e);
            p.switchLeverTop = juce::Colour(0xffbcc8d8);
            p.switchLeverBottom = juce::Colour(0xff4c586a);
            p.switchLeverBorder = juce::Colour(0xff2c3848);
            p.switchActiveText = juce::Colour(0xff70d8ff);
            p.switchInactiveText = juce::Colour(0xff506880);

            p.jewelBezelBg = juce::Colour(0xff283446);
            p.jewelBezelBorder = juce::Colour(0xff506888);
            p.jewelHalo = juce::Colour(0x5500f0ff);
            p.jewelCoreOn = juce::Colour(0xffe0ffff);
            p.jewelEdgeOn = juce::Colour(0xff00c8e8);
            p.jewelCoreOff = juce::Colour(0xff06202c);
            p.jewelEdgeOff = juce::Colour(0xff020d12);

            p.comboBg = juce::Colour(0xff111824);
            p.comboBorder = juce::Colour(0xff2c3e56);
            p.comboText = juce::Colour(0xffe4f4ff);
            p.comboArrow = juce::Colour(0xff60d0ff);
            p.popupBg = juce::Colour(0xff0d141e);
            p.popupHighlightBg = juce::Colour(0xff1c2c40);
            p.popupHighlightText = juce::Colour(0xff00f0ff);

            p.meterFrameBg = juce::Colour(0xff0a0d14);
            p.meterFrameBorder = juce::Colour(0xff283850);
            p.meterFaceTop = juce::Colour(0xffe2f4ff);
            p.meterFaceMid = juce::Colour(0xffbcdff5);
            p.meterFaceBottom = juce::Colour(0xff94ccee);
            p.meterFaceShadow = juce::Colour(0x33000000);
            p.meterScaleText = juce::Colour(0xff0a1c2c);
            p.meterRedZone = juce::Colour(0xffee2844);
            p.meterNeedle = juce::Colour(0xff081420);
            p.meterNeedleGlow = juce::Colour(0x4400f0ff);
            break;
        }
        case Theme::ClassicCharcoal:
        default: {
            p.panelGradTop = juce::Colour(0xff22252a);
            p.panelGradMid1 = juce::Colour(0xff2a2e34);
            p.panelGradMid2 = juce::Colour(0xff1d2024);
            p.panelGradBottom = juce::Colour(0xff141618);
            p.panelStripe = juce::Colour(0x06ffffff);
            p.panelBorder = juce::Colour(0xff3f3b33);
            p.panelInnerBorder = juce::Colour(0xff111214);

            p.screwTop = juce::Colour(0xff555a60);
            p.screwBottom = juce::Colour(0xff222528);
            p.screwBorder = juce::Colour(0xff777d85);
            p.screwSlot = juce::Colour(0xff111315);

            p.headerBrand = juce::Colour(0xffd4b475);
            p.headerTitle = juce::Colour(0xffeedcb3);
            p.headerSub = juce::Colour(0xffa8977a);
            p.headerDivider = juce::Colour(0x55dfc488);

            p.sectionWellBg = juce::Colour(0x33000000);
            p.sectionBorder = juce::Colour(0xff443f36);
            p.sectionHeaderPlate = juce::Colour(0x55111315);
            p.sectionTitleText = juce::Colour(0xffdfc488);
            p.labelText = juce::Colour(0xffdfc488);

            p.knobTick = juce::Colour(0x99dfb872);
            p.knobDropShadow = juce::Colour(0x77000000);
            p.knobCollarTop = juce::Colour(0xff2b2f33);
            p.knobCollarMid = juce::Colour(0xff454a50);
            p.knobCollarBottom = juce::Colour(0xff121416);
            p.knobCollarBorder = juce::Colour(0xff60584c);
            p.knobCoreTop = juce::Colour(0xffc8a86b);
            p.knobCoreMid1 = juce::Colour(0xffdfc488);
            p.knobCoreMid2 = juce::Colour(0xff8a6f3e);
            p.knobCoreBottom = juce::Colour(0xff4a3b22);
            p.knobCoreBorder = juce::Colour(0x22ffffff);
            p.knobPointer = juce::Colour(0xfffff2d6);
            p.knobPointerShadow = juce::Colour(0x99000000);

            p.switchPlateBg = juce::Colour(0xff181a1c);
            p.switchPlateBorder = juce::Colour(0xff4a453d);
            p.switchSlotBg = juce::Colour(0xff090a0b);
            p.switchLeverTop = juce::Colour(0xffdcd6cb);
            p.switchLeverBottom = juce::Colour(0xff6a6458);
            p.switchLeverBorder = juce::Colour(0xffffffff);
            p.switchActiveText = juce::Colour(0xffeedcb3);
            p.switchInactiveText = juce::Colour(0xff888277);

            p.jewelBezelBg = juce::Colour(0xff3a3834);
            p.jewelBezelBorder = juce::Colour(0xff6e675b);
            p.jewelHalo = juce::Colour(0x44ff9900);
            p.jewelCoreOn = juce::Colour(0xffffefb8);
            p.jewelEdgeOn = juce::Colour(0xffff7700);
            p.jewelCoreOff = juce::Colour(0xff442810);
            p.jewelEdgeOff = juce::Colour(0xff180e06);

            p.comboBg = juce::Colour(0xff181a1d);
            p.comboBorder = juce::Colour(0xff524b42);
            p.comboText = juce::Colour(0xffeedcb3);
            p.comboArrow = juce::Colour(0xffdfb872);
            p.popupBg = juce::Colour(0xff16171a);
            p.popupHighlightBg = juce::Colour(0xff3a352a);
            p.popupHighlightText = juce::Colour(0xffffffff);

            p.meterFrameBg = juce::Colour(0xff121416);
            p.meterFrameBorder = juce::Colour(0xff443f36);
            p.meterFaceTop = juce::Colour(0xffeeddbb);
            p.meterFaceMid = juce::Colour(0xffdfc495);
            p.meterFaceBottom = juce::Colour(0xffcaa877);
            p.meterFaceShadow = juce::Colour(0x33000000);
            p.meterScaleText = juce::Colour(0xff281e14);
            p.meterRedZone = juce::Colour(0xffbb2222);
            p.meterNeedle = juce::Colour(0xff111111);
            p.meterNeedleGlow = juce::Colour(0x44ffaa22);
            break;
        }
    }
    return p;
}

AnalogLookAndFeel::AnalogLookAndFeel() {
    setTheme(Theme::ClassicCharcoal);
}

void AnalogLookAndFeel::setTheme(Theme theme) {
    theme_ = theme;
    palette_ = createPalette(theme);

    setColour(juce::Slider::textBoxTextColourId, palette_.labelText);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x22000000));
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
    juce::Slider& /*slider*/
) {
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(width), static_cast<float>(height)).reduced(6.0f);

    const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;

    // 1. Draw outer tick marks around the knob arc
    constexpr int numTicks = 11;
    for (int i = 0; i < numTicks; ++i) {
        const float tickAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (static_cast<float>(i) / (numTicks - 1));
        const float cosA = std::cos(tickAngle - std::numbers::pi_v<float> * 0.5f);
        const float sinA = std::sin(tickAngle - std::numbers::pi_v<float> * 0.5f);

        const float innerR = radius + 1.0f;
        const float outerR = radius + ((i == 0 || i == numTicks - 1 || i == 5) ? 5.5f : 3.5f);

        g.setColour(palette_.knobTick);
        g.drawLine(centreX + innerR * cosA, centreY + innerR * sinA,
                   centreX + outerR * cosA, centreY + outerR * sinA, 1.2f);
    }

    // 2. Drop shadow under knob
    g.setColour(palette_.knobDropShadow);
    g.fillEllipse(rx + 2.0f, ry + 3.0f, rw, rw);

    // 3. Outer knurled metal collar
    juce::ColourGradient collarGrad(palette_.knobCollarTop, rx, ry,
                                   palette_.knobCollarBottom, rx + rw, ry + rw, false);
    collarGrad.addColour(0.5, palette_.knobCollarMid);
    g.setGradientFill(collarGrad);
    g.fillEllipse(rx, ry, rw, rw);

    g.setColour(palette_.knobCollarBorder);
    g.drawEllipse(rx, ry, rw, rw, 1.2f);

    // 4. Inner machined core
    const float innerRadius = radius * 0.78f;
    const float irx = centreX - innerRadius;
    const float iry = centreY - innerRadius;
    const float irw = innerRadius * 2.0f;

    juce::ColourGradient coreGrad(palette_.knobCoreTop, irx, iry,
                                  palette_.knobCoreBottom, irx + irw, iry + irw, false);
    coreGrad.addColour(0.3, palette_.knobCoreMid1);
    coreGrad.addColour(0.7, palette_.knobCoreMid2);
    g.setGradientFill(coreGrad);
    g.fillEllipse(irx, iry, irw, irw);

    // Subtle radial accent
    g.setColour(palette_.knobCoreBorder);
    g.drawEllipse(irx + 2.0f, iry + 2.0f, irw - 4.0f, irw - 4.0f, 0.8f);

    // 5. Knob pointer line
    const float currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const float pCos = std::cos(currentAngle - std::numbers::pi_v<float> * 0.5f);
    const float pSin = std::sin(currentAngle - std::numbers::pi_v<float> * 0.5f);

    juce::Path pointer;
    const float pStart = innerRadius * 0.25f;
    const float pEnd = innerRadius * 0.92f;
    pointer.startNewSubPath(centreX + pStart * pCos, centreY + pStart * pSin);
    pointer.lineTo(centreX + pEnd * pCos, centreY + pEnd * pSin);

    // Pointer shadow and line
    g.setColour(palette_.knobPointerShadow);
    g.strokePath(pointer, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(palette_.knobPointer);
    g.strokePath(pointer, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void AnalogLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    bool /*shouldDrawButtonAsHighlighted*/,
    bool /*shouldDrawButtonAsDown*/
) {
    auto bounds = button.getLocalBounds().toFloat();
    const float switchWidth = 24.0f;
    const float switchHeight = 36.0f;
    const float switchX = bounds.getX() + 4.0f;
    const float switchY = bounds.getCentreY() - switchHeight * 0.5f;

    // Switch bezel plate
    juce::Rectangle<float> switchPlate(switchX, switchY, switchWidth, switchHeight);
    g.setColour(palette_.switchPlateBg);
    g.fillRoundedRectangle(switchPlate, 3.0f);
    g.setColour(palette_.switchPlateBorder);
    g.drawRoundedRectangle(switchPlate, 3.0f, 1.0f);

    // Switch slot
    juce::Rectangle<float> slot(switchPlate.getCentreX() - 3.0f, switchPlate.getY() + 4.0f, 6.0f, switchHeight - 8.0f);
    g.setColour(palette_.switchSlotBg);
    g.fillRect(slot);

    // Toggle bat (lever)
    const bool isToggled = button.getToggleState();
    const float leverY = isToggled ? (slot.getY() + 3.0f) : (slot.getBottom() - 11.0f);
    juce::Rectangle<float> lever(switchPlate.getCentreX() - 6.0f, leverY, 12.0f, 8.0f);

    juce::ColourGradient leverGrad(palette_.switchLeverTop, lever.getX(), lever.getY(),
                                  palette_.switchLeverBottom, lever.getRight(), lever.getBottom(), false);
    g.setGradientFill(leverGrad);
    g.fillRoundedRectangle(lever, 2.0f);
    g.setColour(palette_.switchLeverBorder);
    g.drawRoundedRectangle(lever, 2.0f, 0.8f);

    // Jewel LED indicator
    const float ledSize = 10.0f;
    const float ledX = switchPlate.getRight() + 8.0f;
    const float ledY = bounds.getCentreY() - ledSize * 0.5f;
    drawVintageJewelLed(g, juce::Rectangle<float>(ledX, ledY, ledSize, ledSize), isToggled, palette_);

    // Text label
    g.setColour(isToggled ? palette_.switchActiveText : palette_.switchInactiveText);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText(button.getButtonText(),
               static_cast<int>(ledX + ledSize + 8.0f),
               0,
               static_cast<int>(bounds.getWidth() - (ledX + ledSize + 8.0f)),
               static_cast<int>(bounds.getHeight()),
               juce::Justification::centredLeft);
}

void AnalogLookAndFeel::drawVintageJewelLed(juce::Graphics& g, juce::Rectangle<float> bounds, bool isActive) const {
    drawVintageJewelLed(g, bounds, isActive, palette_);
}

void AnalogLookAndFeel::drawVintageJewelLed(juce::Graphics& g, juce::Rectangle<float> bounds, bool isActive, const ThemePalette& palette) {
    // Outer metal bezel
    g.setColour(palette.jewelBezelBg);
    g.fillEllipse(bounds);
    g.setColour(palette.jewelBezelBorder);
    g.drawEllipse(bounds, 1.0f);

    // Inner glowing jewel
    auto inner = bounds.reduced(1.8f);
    if (isActive) {
        g.setColour(palette.jewelHalo);
        g.fillEllipse(bounds.expanded(3.0f));

        juce::ColourGradient ledGrad(palette.jewelCoreOn, inner.getCentreX(), inner.getCentreY(),
                                    palette.jewelEdgeOn, inner.getX(), inner.getY(), true);
        g.setGradientFill(ledGrad);
        g.fillEllipse(inner);
    } else {
        juce::ColourGradient offGrad(palette.jewelCoreOff, inner.getX(), inner.getY(),
                                    palette.jewelEdgeOff, inner.getRight(), inner.getBottom(), false);
        g.setGradientFill(offGrad);
        g.fillEllipse(inner);
    }
}

void AnalogLookAndFeel::drawRackScrew(juce::Graphics& g, float centerX, float centerY, float radius) const {
    drawRackScrew(g, centerX, centerY, radius, palette_);
}

void AnalogLookAndFeel::drawRackScrew(juce::Graphics& g, float centerX, float centerY, float radius, const ThemePalette& palette) {
    const float x = centerX - radius;
    const float y = centerY - radius;
    const float d = radius * 2.0f;

    // Screw head shadow & outer circle
    g.setColour(juce::Colour(0x99000000));
    g.fillEllipse(x + 1.0f, y + 1.0f, d, d);

    juce::ColourGradient screwGrad(palette.screwTop, x, y,
                                  palette.screwBottom, x + d, y + d, false);
    g.setGradientFill(screwGrad);
    g.fillEllipse(x, y, d, d);

    g.setColour(palette.screwBorder);
    g.drawEllipse(x, y, d, d, 0.8f);

    // Phillips slot
    g.setColour(palette.screwSlot);
    const float slotLen = radius * 0.65f;
    g.drawLine(centerX - slotLen, centerY, centerX + slotLen, centerY, 1.2f);
    g.drawLine(centerX, centerY - slotLen, centerX, centerY + slotLen, 1.2f);
}

void AnalogLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width, int height,
    bool /*isButtonDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& /*box*/
) {
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));

    // Vintage beveled inset
    g.setColour(palette_.comboBg);
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(palette_.comboBorder);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    // Vintage downward triangle arrow
    const float arrowX = static_cast<float>(width - 16);
    const float arrowY = static_cast<float>(height) * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowX, arrowY - 2.5f, arrowX + 8.0f, arrowY - 2.5f, arrowX + 4.0f, arrowY + 3.0f);
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

} // namespace analog_master::ui
