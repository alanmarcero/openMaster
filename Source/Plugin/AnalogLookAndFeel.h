#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace analog_master::ui {

enum class Theme : int {
    CyberNeon = 0,
    ChromeDeck = 1,
    AcidBioGrid = 2,
    DeepNetIce = 3,
    SolarOverdrive = 4
};

struct ThemePalette {
    // Faceplate panel gradients & cyber HUD grid
    juce::Colour panelGradTop;
    juce::Colour panelGradMid1;
    juce::Colour panelGradMid2;
    juce::Colour panelGradBottom;
    juce::Colour panelStripe;
    juce::Colour panelBorder;
    juce::Colour panelInnerBorder;

    // Cyber Neon Accents
    juce::Colour accentPrimary;
    juce::Colour accentSecondary;
    juce::Colour hudGrid;

    // Screws / Hex Bolts
    juce::Colour screwTop;
    juce::Colour screwBottom;
    juce::Colour screwBorder;
    juce::Colour screwSlot;

    // Header & Brand
    juce::Colour headerBrand;
    juce::Colour headerTitle;
    juce::Colour headerSub;
    juce::Colour headerDivider;

    // Sections / Cyber Cards
    juce::Colour sectionWellBg;
    juce::Colour sectionBorder;
    juce::Colour sectionHeaderPlate;
    juce::Colour sectionTitleText;
    juce::Colour labelText;

    // Knobs
    juce::Colour knobTick;
    juce::Colour knobDropShadow;
    juce::Colour knobCollarTop;
    juce::Colour knobCollarMid;
    juce::Colour knobCollarBottom;
    juce::Colour knobCollarBorder;
    juce::Colour knobCoreTop;
    juce::Colour knobCoreMid1;
    juce::Colour knobCoreMid2;
    juce::Colour knobCoreBottom;
    juce::Colour knobCoreBorder;
    juce::Colour knobPointer;
    juce::Colour knobPointerShadow;

    // Toggles
    juce::Colour switchPlateBg;
    juce::Colour switchPlateBorder;
    juce::Colour switchSlotBg;
    juce::Colour switchLeverTop;
    juce::Colour switchLeverBottom;
    juce::Colour switchLeverBorder;
    juce::Colour switchActiveText;
    juce::Colour switchInactiveText;

    // Jewel LED
    juce::Colour jewelBezelBg;
    juce::Colour jewelBezelBorder;
    juce::Colour jewelHalo;
    juce::Colour jewelCoreOn;
    juce::Colour jewelEdgeOn;
    juce::Colour jewelCoreOff;
    juce::Colour jewelEdgeOff;

    // ComboBox / Popup
    juce::Colour comboBg;
    juce::Colour comboBorder;
    juce::Colour comboText;
    juce::Colour comboArrow;
    juce::Colour popupBg;
    juce::Colour popupHighlightBg;
    juce::Colour popupHighlightText;

    // Holographic Cyber VU Meter
    juce::Colour meterFrameBg;
    juce::Colour meterFrameBorder;
    juce::Colour meterFaceTop;
    juce::Colour meterFaceMid;
    juce::Colour meterFaceBottom;
    juce::Colour meterFaceShadow;
    juce::Colour meterScaleText;
    juce::Colour meterRedZone;
    juce::Colour meterNeedle;
    juce::Colour meterNeedleGlow;
};

class AnalogLookAndFeel : public juce::LookAndFeel_V4 {
public:
    AnalogLookAndFeel();

    void setTheme(Theme theme);
    Theme getTheme() const noexcept { return theme_; }
    const ThemePalette& getPalette() const noexcept { return palette_; }

    void drawRotarySlider(
        juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider
    ) override;

    void drawToggleButton(
        juce::Graphics& g,
        juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown
    ) override;

    void drawComboBox(
        juce::Graphics& g,
        int width, int height,
        bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox& box
    ) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawRackScrew(juce::Graphics& g, float centerX, float centerY, float radius) const;

    static void drawRackScrew(juce::Graphics& g, float centerX, float centerY, float radius, const ThemePalette& palette);

    static ThemePalette createPalette(Theme theme);

private:
    Theme theme_ { Theme::CyberNeon };
    ThemePalette palette_;
};

} // namespace analog_master::ui
