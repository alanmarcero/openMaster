#include "AnalogLookAndFeel.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace analog_master::ui {

ThemePalette AnalogLookAndFeel::createPalette(Theme theme) {
    ThemePalette p;
    switch (theme) {
        case Theme::ChromeDeck: {
            p.panelGradTop = juce::Colour(0xff232a36);
            p.panelGradMid1 = juce::Colour(0xff2b3444);
            p.panelGradMid2 = juce::Colour(0xff19202b);
            p.panelGradBottom = juce::Colour(0xff10151e);
            p.panelStripe = juce::Colour(0x14ffffff);
            p.panelBorder = juce::Colour(0xff6b82a6);
            p.panelInnerBorder = juce::Colour(0x55a8c4eb);

            p.accentPrimary = juce::Colour(0xff38e8ff);
            p.accentSecondary = juce::Colour(0xffffb428);
            p.hudGrid = juce::Colour(0x1e38e8ff);

            p.screwTop = juce::Colour(0xff9eb0c9);
            p.screwBottom = juce::Colour(0xff3d4a5d);
            p.screwBorder = juce::Colour(0xffd0e0f5);
            p.screwSlot = juce::Colour(0xff131922);

            p.headerBrand = juce::Colour(0xff38e8ff);
            p.headerTitle = juce::Colour(0xfff4f9ff);
            p.headerSub = juce::Colour(0xff9eb6d4);
            p.headerDivider = juce::Colour(0x7738e8ff);

            p.sectionWellBg = juce::Colour(0xd8141a24);
            p.sectionBorder = juce::Colour(0xff4c6282);
            p.sectionHeaderPlate = juce::Colour(0xff1e2736);
            p.sectionTitleText = juce::Colour(0xff38e8ff);
            p.labelText = juce::Colour(0xffd6e6fa);

            p.knobTick = juce::Colour(0xcc9eb6d4);
            p.knobDropShadow = juce::Colour(0x88000000);
            p.knobCollarTop = juce::Colour(0xff52627a);
            p.knobCollarMid = juce::Colour(0xff384558);
            p.knobCollarBottom = juce::Colour(0xff1d2530);
            p.knobCollarBorder = juce::Colour(0xff849bb8);
            p.knobCoreTop = juce::Colour(0xff344052);
            p.knobCoreMid1 = juce::Colour(0xff46566e);
            p.knobCoreMid2 = juce::Colour(0xff232c3b);
            p.knobCoreBottom = juce::Colour(0xff141923);
            p.knobCoreBorder = juce::Colour(0xff38e8ff);
            p.knobPointer = juce::Colour(0xfff5fbff);
            p.knobPointerShadow = juce::Colour(0xaa000000);

            p.switchPlateBg = juce::Colour(0xff161d29);
            p.switchPlateBorder = juce::Colour(0xff4a6080);
            p.switchSlotBg = juce::Colour(0xff0b0f16);
            p.switchLeverTop = juce::Colour(0xffe0eeff);
            p.switchLeverBottom = juce::Colour(0xff6b82a4);
            p.switchLeverBorder = juce::Colour(0xff38e8ff);
            p.switchActiveText = juce::Colour(0xfff4f9ff);
            p.switchInactiveText = juce::Colour(0xff7d93b0);

            p.jewelBezelBg = juce::Colour(0xff344256);
            p.jewelBezelBorder = juce::Colour(0xff849bb8);
            p.jewelHalo = juce::Colour(0x6638e8ff);
            p.jewelCoreOn = juce::Colour(0xffe8ffff);
            p.jewelEdgeOn = juce::Colour(0xff00b8ff);
            p.jewelCoreOff = juce::Colour(0xff0b2030);
            p.jewelEdgeOff = juce::Colour(0xff050e16);

            p.comboBg = juce::Colour(0xff18202e);
            p.comboBorder = juce::Colour(0xff4c6282);
            p.comboText = juce::Colour(0xfff0f7ff);
            p.comboArrow = juce::Colour(0xff38e8ff);
            p.popupBg = juce::Colour(0xff141b26);
            p.popupHighlightBg = juce::Colour(0xff2a3950);
            p.popupHighlightText = juce::Colour(0xff38e8ff);

            p.meterFrameBg = juce::Colour(0xff0d121a);
            p.meterFrameBorder = juce::Colour(0xff4c6282);
            p.meterFaceTop = juce::Colour(0xff182232);
            p.meterFaceMid = juce::Colour(0xff111926);
            p.meterFaceBottom = juce::Colour(0xff0a0f18);
            p.meterFaceShadow = juce::Colour(0x66000000);
            p.meterScaleText = juce::Colour(0xff9eb6d4);
            p.meterRedZone = juce::Colour(0xffff3860);
            p.meterNeedle = juce::Colour(0xff38e8ff);
            p.meterNeedleGlow = juce::Colour(0x6638e8ff);
            break;
        }
        case Theme::AcidBioGrid: {
            p.panelGradTop = juce::Colour(0xff0d1711);
            p.panelGradMid1 = juce::Colour(0xff122118);
            p.panelGradMid2 = juce::Colour(0xff0a130e);
            p.panelGradBottom = juce::Colour(0xff050a07);
            p.panelStripe = juce::Colour(0x0c29ff70);
            p.panelBorder = juce::Colour(0xff265c3a);
            p.panelInnerBorder = juce::Colour(0x4429ff70);

            p.accentPrimary = juce::Colour(0xff29ff70);
            p.accentSecondary = juce::Colour(0xffd4ff28);
            p.hudGrid = juce::Colour(0x1c29ff70);

            p.screwTop = juce::Colour(0xff3d6b4c);
            p.screwBottom = juce::Colour(0xff16291c);
            p.screwBorder = juce::Colour(0xff5ca372);
            p.screwSlot = juce::Colour(0xff060c08);

            p.headerBrand = juce::Colour(0xff29ff70);
            p.headerTitle = juce::Colour(0xffe8ffee);
            p.headerSub = juce::Colour(0xff7ec492);
            p.headerDivider = juce::Colour(0x7729ff70);

            p.sectionWellBg = juce::Colour(0xd80b140e);
            p.sectionBorder = juce::Colour(0xff255235);
            p.sectionHeaderPlate = juce::Colour(0xff122418);
            p.sectionTitleText = juce::Colour(0xff29ff70);
            p.labelText = juce::Colour(0xffc8fad6);

            p.knobTick = juce::Colour(0xbb7ec492);
            p.knobDropShadow = juce::Colour(0x88000000);
            p.knobCollarTop = juce::Colour(0xff243d2c);
            p.knobCollarMid = juce::Colour(0xff182b1e);
            p.knobCollarBottom = juce::Colour(0xff0d1710);
            p.knobCollarBorder = juce::Colour(0xff386e4a);
            p.knobCoreTop = juce::Colour(0xff1d3324);
            p.knobCoreMid1 = juce::Colour(0xff284732);
            p.knobCoreMid2 = juce::Colour(0xff132218);
            p.knobCoreBottom = juce::Colour(0xff0a130d);
            p.knobCoreBorder = juce::Colour(0xff29ff70);
            p.knobPointer = juce::Colour(0xffeafff0);
            p.knobPointerShadow = juce::Colour(0xbb000000);

            p.switchPlateBg = juce::Colour(0xff0d1a12);
            p.switchPlateBorder = juce::Colour(0xff2d6340);
            p.switchSlotBg = juce::Colour(0xff050b07);
            p.switchLeverTop = juce::Colour(0xffb8ffd0);
            p.switchLeverBottom = juce::Colour(0xff29a854);
            p.switchLeverBorder = juce::Colour(0xff29ff70);
            p.switchActiveText = juce::Colour(0xffe8ffee);
            p.switchInactiveText = juce::Colour(0xff5e9470);

            p.jewelBezelBg = juce::Colour(0xff1d3826);
            p.jewelBezelBorder = juce::Colour(0xff468259);
            p.jewelHalo = juce::Colour(0x6629ff70);
            p.jewelCoreOn = juce::Colour(0xffe6ffed);
            p.jewelEdgeOn = juce::Colour(0xff00e650);
            p.jewelCoreOff = juce::Colour(0xff092612);
            p.jewelEdgeOff = juce::Colour(0xff031007);

            p.comboBg = juce::Colour(0xff0f1e14);
            p.comboBorder = juce::Colour(0xff2b5e3d);
            p.comboText = juce::Colour(0xffe6ffed);
            p.comboArrow = juce::Colour(0xff29ff70);
            p.popupBg = juce::Colour(0xff0b160f);
            p.popupHighlightBg = juce::Colour(0xff1b3b26);
            p.popupHighlightText = juce::Colour(0xff29ff70);

            p.meterFrameBg = juce::Colour(0xff070e09);
            p.meterFrameBorder = juce::Colour(0xff28593a);
            p.meterFaceTop = juce::Colour(0xff112217);
            p.meterFaceMid = juce::Colour(0xff0b1710);
            p.meterFaceBottom = juce::Colour(0xff060d09);
            p.meterFaceShadow = juce::Colour(0x66000000);
            p.meterScaleText = juce::Colour(0xff84cc98);
            p.meterRedZone = juce::Colour(0xffffc828);
            p.meterNeedle = juce::Colour(0xff29ff70);
            p.meterNeedleGlow = juce::Colour(0x6629ff70);
            break;
        }
        case Theme::DeepNetIce: {
            p.panelGradTop = juce::Colour(0xff121829);
            p.panelGradMid1 = juce::Colour(0xff172036);
            p.panelGradMid2 = juce::Colour(0xff0e1424);
            p.panelGradBottom = juce::Colour(0xff080b14);
            p.panelStripe = juce::Colour(0x0c66d9ff);
            p.panelBorder = juce::Colour(0xff384d78);
            p.panelInnerBorder = juce::Colour(0x4466d9ff);

            p.accentPrimary = juce::Colour(0xff5ce1ff);
            p.accentSecondary = juce::Colour(0xff9d7aff);
            p.hudGrid = juce::Colour(0x1c5ce1ff);

            p.screwTop = juce::Colour(0xff4a5e85);
            p.screwBottom = juce::Colour(0xff1c2538);
            p.screwBorder = juce::Colour(0xff7590c2);
            p.screwSlot = juce::Colour(0xff090d16);

            p.headerBrand = juce::Colour(0xff5ce1ff);
            p.headerTitle = juce::Colour(0xfff0f8ff);
            p.headerSub = juce::Colour(0xff8ca8d4);
            p.headerDivider = juce::Colour(0x775ce1ff);

            p.sectionWellBg = juce::Colour(0xd80d1322);
            p.sectionBorder = juce::Colour(0xff2e426b);
            p.sectionHeaderPlate = juce::Colour(0xff162038);
            p.sectionTitleText = juce::Colour(0xff5ce1ff);
            p.labelText = juce::Colour(0xffd0e8ff);

            p.knobTick = juce::Colour(0xbb8ca8d4);
            p.knobDropShadow = juce::Colour(0x88000000);
            p.knobCollarTop = juce::Colour(0xff2a3854);
            p.knobCollarMid = juce::Colour(0xff1e283d);
            p.knobCollarBottom = juce::Colour(0xff101624);
            p.knobCollarBorder = juce::Colour(0xff465c87);
            p.knobCoreTop = juce::Colour(0xff222e47);
            p.knobCoreMid1 = juce::Colour(0xff304061);
            p.knobCoreMid2 = juce::Colour(0xff182033);
            p.knobCoreBottom = juce::Colour(0xff0e1320);
            p.knobCoreBorder = juce::Colour(0xff5ce1ff);
            p.knobPointer = juce::Colour(0xfff2fbff);
            p.knobPointerShadow = juce::Colour(0xbb000000);

            p.switchPlateBg = juce::Colour(0xff101729);
            p.switchPlateBorder = juce::Colour(0xff364e7d);
            p.switchSlotBg = juce::Colour(0xff070a12);
            p.switchLeverTop = juce::Colour(0xffd8f4ff);
            p.switchLeverBottom = juce::Colour(0xff4888b8);
            p.switchLeverBorder = juce::Colour(0xff5ce1ff);
            p.switchActiveText = juce::Colour(0xfff0f8ff);
            p.switchInactiveText = juce::Colour(0xff6880a6);

            p.jewelBezelBg = juce::Colour(0xff24324f);
            p.jewelBezelBorder = juce::Colour(0xff5672a3);
            p.jewelHalo = juce::Colour(0x665ce1ff);
            p.jewelCoreOn = juce::Colour(0xffe8fbff);
            p.jewelEdgeOn = juce::Colour(0xff28b8ff);
            p.jewelCoreOff = juce::Colour(0xff0a1e33);
            p.jewelEdgeOff = juce::Colour(0xff040c17);

            p.comboBg = juce::Colour(0xff121b2e);
            p.comboBorder = juce::Colour(0xff354c78);
            p.comboText = juce::Colour(0xfff0f8ff);
            p.comboArrow = juce::Colour(0xff5ce1ff);
            p.popupBg = juce::Colour(0xff0d1424);
            p.popupHighlightBg = juce::Colour(0xff1f2f4d);
            p.popupHighlightText = juce::Colour(0xff5ce1ff);

            p.meterFrameBg = juce::Colour(0xff090d18);
            p.meterFrameBorder = juce::Colour(0xff324873);
            p.meterFaceTop = juce::Colour(0xff141e36);
            p.meterFaceMid = juce::Colour(0xff0e1526);
            p.meterFaceBottom = juce::Colour(0xff080c18);
            p.meterFaceShadow = juce::Colour(0x66000000);
            p.meterScaleText = juce::Colour(0xff94b4e0);
            p.meterRedZone = juce::Colour(0xffff427a);
            p.meterNeedle = juce::Colour(0xff5ce1ff);
            p.meterNeedleGlow = juce::Colour(0x665ce1ff);
            break;
        }
        case Theme::SolarOverdrive: {
            p.panelGradTop = juce::Colour(0xff1c1310);
            p.panelGradMid1 = juce::Colour(0xff261914);
            p.panelGradMid2 = juce::Colour(0xff160f0c);
            p.panelGradBottom = juce::Colour(0xff0c0806);
            p.panelStripe = juce::Colour(0x0cff9d1c);
            p.panelBorder = juce::Colour(0xff6b3f22);
            p.panelInnerBorder = juce::Colour(0x44ff9d1c);

            p.accentPrimary = juce::Colour(0xffff9d1c);
            p.accentSecondary = juce::Colour(0xffff2a55);
            p.hudGrid = juce::Colour(0x1cff9d1c);

            p.screwTop = juce::Colour(0xff805536);
            p.screwBottom = juce::Colour(0xff331e10);
            p.screwBorder = juce::Colour(0xffb87d52);
            p.screwSlot = juce::Colour(0xff120a05);

            p.headerBrand = juce::Colour(0xffff9d1c);
            p.headerTitle = juce::Colour(0xfffff2e0);
            p.headerSub = juce::Colour(0xffc99b78);
            p.headerDivider = juce::Colour(0x77ff9d1c);

            p.sectionWellBg = juce::Colour(0xd817100c);
            p.sectionBorder = juce::Colour(0xff59341c);
            p.sectionHeaderPlate = juce::Colour(0xff261812);
            p.sectionTitleText = juce::Colour(0xffff9d1c);
            p.labelText = juce::Colour(0xffffe4c4);

            p.knobTick = juce::Colour(0xbbc99b78);
            p.knobDropShadow = juce::Colour(0x88000000);
            p.knobCollarTop = juce::Colour(0xff472d1d);
            p.knobCollarMid = juce::Colour(0xff301e13);
            p.knobCollarBottom = juce::Colour(0xff1a100a);
            p.knobCollarBorder = juce::Colour(0xff75482b);
            p.knobCoreTop = juce::Colour(0xff382216);
            p.knobCoreMid1 = juce::Colour(0xff4f3120);
            p.knobCoreMid2 = juce::Colour(0xff26170f);
            p.knobCoreBottom = juce::Colour(0xff140c08);
            p.knobCoreBorder = juce::Colour(0xffff9d1c);
            p.knobPointer = juce::Colour(0xfffff6eb);
            p.knobPointerShadow = juce::Colour(0xbb000000);

            p.switchPlateBg = juce::Colour(0xff1c120d);
            p.switchPlateBorder = juce::Colour(0xff663c22);
            p.switchSlotBg = juce::Colour(0xff0d0805);
            p.switchLeverTop = juce::Colour(0xffffe0b8);
            p.switchLeverBottom = juce::Colour(0xffb8621b);
            p.switchLeverBorder = juce::Colour(0xffff9d1c);
            p.switchActiveText = juce::Colour(0xfffff2e0);
            p.switchInactiveText = juce::Colour(0xff946f54);

            p.jewelBezelBg = juce::Colour(0xff422818);
            p.jewelBezelBorder = juce::Colour(0xff875534);
            p.jewelHalo = juce::Colour(0x66ff9d1c);
            p.jewelCoreOn = juce::Colour(0xfffff2d6);
            p.jewelEdgeOn = juce::Colour(0xffff6600);
            p.jewelCoreOff = juce::Colour(0xff2e1506);
            p.jewelEdgeOff = juce::Colour(0xff140802);

            p.comboBg = juce::Colour(0xff1f140e);
            p.comboBorder = juce::Colour(0xff613920);
            p.comboText = juce::Colour(0xfffff2e0);
            p.comboArrow = juce::Colour(0xffff9d1c);
            p.popupBg = juce::Colour(0xff170f0a);
            p.popupHighlightBg = juce::Colour(0xff3b2314);
            p.popupHighlightText = juce::Colour(0xffff9d1c);

            p.meterFrameBg = juce::Colour(0xff100a07);
            p.meterFrameBorder = juce::Colour(0xff59341c);
            p.meterFaceTop = juce::Colour(0xff24160f);
            p.meterFaceMid = juce::Colour(0xff190f0a);
            p.meterFaceBottom = juce::Colour(0xff0f0906);
            p.meterFaceShadow = juce::Colour(0x66000000);
            p.meterScaleText = juce::Colour(0xffdeb08a);
            p.meterRedZone = juce::Colour(0xffff2a55);
            p.meterNeedle = juce::Colour(0xffff9d1c);
            p.meterNeedleGlow = juce::Colour(0x66ff9d1c);
            break;
        }
        case Theme::CyberNeon:
        default: {
            p.panelGradTop = juce::Colour(0xff0e121c);
            p.panelGradMid1 = juce::Colour(0xff141a29);
            p.panelGradMid2 = juce::Colour(0xff0b0f18);
            p.panelGradBottom = juce::Colour(0xff06080d);
            p.panelStripe = juce::Colour(0x0c00f0ff);
            p.panelBorder = juce::Colour(0xff283856);
            p.panelInnerBorder = juce::Colour(0x5500f0ff);

            p.accentPrimary = juce::Colour(0xff00f0ff);
            p.accentSecondary = juce::Colour(0xffff2a85);
            p.hudGrid = juce::Colour(0x1c00f0ff);

            p.screwTop = juce::Colour(0xff3e4d6b);
            p.screwBottom = juce::Colour(0xff161c29);
            p.screwBorder = juce::Colour(0xff647ba8);
            p.screwSlot = juce::Colour(0xff080b11);

            p.headerBrand = juce::Colour(0xff00f0ff);
            p.headerTitle = juce::Colour(0xfff0f8ff);
            p.headerSub = juce::Colour(0xff8ca4c8);
            p.headerDivider = juce::Colour(0x8800f0ff);

            p.sectionWellBg = juce::Colour(0xd80b0f19);
            p.sectionBorder = juce::Colour(0xff253552);
            p.sectionHeaderPlate = juce::Colour(0xff131b2e);
            p.sectionTitleText = juce::Colour(0xff00f0ff);
            p.labelText = juce::Colour(0xffd6ecff);

            p.knobTick = juce::Colour(0xbb8ca4c8);
            p.knobDropShadow = juce::Colour(0x88000000);
            p.knobCollarTop = juce::Colour(0xff28334a);
            p.knobCollarMid = juce::Colour(0xff1b2334);
            p.knobCollarBottom = juce::Colour(0xff0e131d);
            p.knobCollarBorder = juce::Colour(0xff3d5075);
            p.knobCoreTop = juce::Colour(0xff1f283b);
            p.knobCoreMid1 = juce::Colour(0xff2c3954);
            p.knobCoreMid2 = juce::Colour(0xff151c2b);
            p.knobCoreBottom = juce::Colour(0xff0c101a);
            p.knobCoreBorder = juce::Colour(0xff00f0ff);
            p.knobPointer = juce::Colour(0xffffffff);
            p.knobPointerShadow = juce::Colour(0xbb000000);

            p.switchPlateBg = juce::Colour(0xff0e1422);
            p.switchPlateBorder = juce::Colour(0xff2d4269);
            p.switchSlotBg = juce::Colour(0xff060910);
            p.switchLeverTop = juce::Colour(0xffd6faff);
            p.switchLeverBottom = juce::Colour(0xff0099bb);
            p.switchLeverBorder = juce::Colour(0xff00f0ff);
            p.switchActiveText = juce::Colour(0xfff0f8ff);
            p.switchInactiveText = juce::Colour(0xff647a9e);

            p.jewelBezelBg = juce::Colour(0xff202b42);
            p.jewelBezelBorder = juce::Colour(0xff4c6594);
            p.jewelHalo = juce::Colour(0x6600f0ff);
            p.jewelCoreOn = juce::Colour(0xffe8ffff);
            p.jewelEdgeOn = juce::Colour(0xff00d8ff);
            p.jewelCoreOff = juce::Colour(0xff09222c);
            p.jewelEdgeOff = juce::Colour(0xff030c12);

            p.comboBg = juce::Colour(0xff111829);
            p.comboBorder = juce::Colour(0xff2e446b);
            p.comboText = juce::Colour(0xfff0f8ff);
            p.comboArrow = juce::Colour(0xff00f0ff);
            p.popupBg = juce::Colour(0xff0c111d);
            p.popupHighlightBg = juce::Colour(0xff1c2b47);
            p.popupHighlightText = juce::Colour(0xff00f0ff);

            p.meterFrameBg = juce::Colour(0xff080b13);
            p.meterFrameBorder = juce::Colour(0xff283b5c);
            p.meterFaceTop = juce::Colour(0xff121a2d);
            p.meterFaceMid = juce::Colour(0xff0c1220);
            p.meterFaceBottom = juce::Colour(0xff070b14);
            p.meterFaceShadow = juce::Colour(0x66000000);
            p.meterScaleText = juce::Colour(0xff8eb2de);
            p.meterRedZone = juce::Colour(0xffff2a85);
            p.meterNeedle = juce::Colour(0xff00f0ff);
            p.meterNeedleGlow = juce::Colour(0x6600f0ff);
            break;
        }
    }
    return p;
}

AnalogLookAndFeel::AnalogLookAndFeel() {
    setTheme(Theme::CyberNeon);
}

void AnalogLookAndFeel::setTheme(Theme theme) {
    theme_ = theme;
    palette_ = createPalette(theme);

    setColour(juce::Slider::textBoxTextColourId, palette_.headerTitle);
    setColour(juce::Slider::textBoxBackgroundColourId, palette_.panelGradBottom.withAlpha(0.88f));
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

    // 1. Minor tick ring
    constexpr int kMinorTicks = 31;
    for (int i = 0; i < kMinorTicks; ++i) {
        const float norm = static_cast<float>(i) / static_cast<float>(kMinorTicks - 1);
        const float angle = rotaryStartAngle + norm * (rotaryEndAngle - rotaryStartAngle);
        const bool lit = norm <= sliderPosProportional + 0.002f;
        g.setColour(lit ? palette_.accentPrimary.interpolatedWith(palette_.accentSecondary, norm).withAlpha(0.8f)
                        : palette_.knobTick.withAlpha(0.28f));
        g.drawLine(juce::Line<float>(pointAt(radius + 4.0f, angle), pointAt(radius + 7.0f, angle)), 1.0f);
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

            g.setColour(lit ? palette_.accentPrimary : palette_.knobTick.withAlpha(0.7f));
            g.drawLine(juce::Line<float>(pointAt(radius + 3.5f, angle), pointAt(radius + 10.0f, angle)), 1.6f);

            g.setColour(lit ? palette_.headerTitle : palette_.knobTick);
            g.drawText((*scaleLabels)[i].toString(),
                       juce::Rectangle<float>(34.0f, 10.0f).withCentre(pointAt(radius + 15.5f, angle)),
                       juce::Justification::centred, false);
        }
    }

    // 3. Value arc
    const float arcRadius = radius + 1.0f;
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(palette_.switchSlotBg.brighter(0.15f));
    g.strokePath(bgArc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPosProportional > 0.004f) {
        juce::Path valArc;
        valArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, currentAngle, true);
        g.setColour(palette_.accentPrimary.withAlpha(0.24f));
        g.strokePath(valArc, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::ColourGradient arcGrad(palette_.accentPrimary, centreX - arcRadius, centreY,
                                     palette_.accentSecondary, centreX + arcRadius, centreY, false);
        g.setGradientFill(arcGrad);
        g.strokePath(valArc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 4. Knob body: drop shadow, knurled collar, inner cap
    const float bodyRadius = radius * 0.84f;
    const auto body = juce::Rectangle<float>(bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre({ centreX, centreY });

    g.setColour(palette_.knobDropShadow);
    g.fillEllipse(body.translated(1.5f, 3.0f));

    juce::ColourGradient collarGrad(palette_.knobCollarTop, body.getX(), body.getY(),
                                    palette_.knobCollarBottom, body.getRight(), body.getBottom(), false);
    collarGrad.addColour(0.5, palette_.knobCollarMid);
    g.setGradientFill(collarGrad);
    g.fillEllipse(body);
    g.setColour(palette_.knobCollarBorder);
    g.drawEllipse(body, 1.2f);

    constexpr int kGrooves = 24;
    g.setColour(juce::Colours::black.withAlpha(0.42f));
    for (int i = 0; i < kGrooves; ++i) {
        const float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(kGrooves);
        g.drawLine(juce::Line<float>(pointAt(bodyRadius * 0.82f, angle), pointAt(bodyRadius * 0.97f, angle)), 1.0f);
    }

    const float capRadius = bodyRadius * 0.72f;
    const auto cap = juce::Rectangle<float>(capRadius * 2.0f, capRadius * 2.0f).withCentre({ centreX, centreY });
    juce::ColourGradient coreGrad(palette_.knobCoreTop, cap.getX(), cap.getY(),
                                  palette_.knobCoreBottom, cap.getRight(), cap.getBottom(), false);
    coreGrad.addColour(0.35, palette_.knobCoreMid1);
    coreGrad.addColour(0.75, palette_.knobCoreMid2);
    g.setGradientFill(coreGrad);
    g.fillEllipse(cap);
    g.setColour(palette_.accentPrimary.withAlpha(0.45f));
    g.drawEllipse(cap.reduced(1.0f), 1.0f);

    // 5. Pointer
    const juce::Line<float> pointer(pointAt(capRadius * 0.22f, currentAngle), pointAt(bodyRadius * 0.9f, currentAngle));
    g.setColour(palette_.accentPrimary.withAlpha(0.45f));
    g.drawLine(pointer, 4.2f);
    g.setColour(palette_.knobPointer);
    g.drawLine(pointer, 2.0f);
    g.setColour(palette_.accentPrimary);
    g.fillEllipse(juce::Rectangle<float>(4.6f, 4.6f).withCentre(pointer.getEnd()));
}

void AnalogLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted,
    bool /*shouldDrawButtonAsDown*/
) {
    const auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const bool isOn = button.getToggleState();
    const auto& props = button.getProperties();
    const juce::Colour accent = props["useSecondaryAccent"] ? palette_.accentSecondary : palette_.accentPrimary;

    // The text names the current state
    const juce::String text = isOn ? props.getWithDefault("onText", button.getButtonText()).toString()
                                   : props.getWithDefault("offText", button.getButtonText()).toString();

    juce::Colour fill = isOn ? accent.withAlpha(0.22f) : palette_.switchPlateBg;
    if (shouldDrawButtonAsHighlighted) {
        fill = fill.brighter(0.15f);
    }
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 4.5f);

    if (isOn) {
        g.setColour(accent.withAlpha(0.28f));
        g.drawRoundedRectangle(bounds.expanded(1.2f), 5.5f, 2.2f);
    }
    g.setColour(isOn ? accent : palette_.switchPlateBorder);
    g.drawRoundedRectangle(bounds, 4.5f, isOn ? 1.5f : 1.0f);

    const auto led = juce::Rectangle<float>(9.0f, 9.0f).withCentre({ bounds.getX() + 14.0f, bounds.getCentreY() });
    if (isOn) {
        g.setColour(accent.withAlpha(0.42f));
        g.fillEllipse(led.expanded(3.0f));
        g.setColour(palette_.headerTitle);
        g.fillEllipse(led.reduced(1.5f));
        g.setColour(accent);
        g.drawEllipse(led, 1.5f);
    } else {
        g.setColour(palette_.switchSlotBg);
        g.fillEllipse(led);
        g.setColour(palette_.switchPlateBorder);
        g.drawEllipse(led, 1.0f);
    }

    g.setColour(isOn ? palette_.switchActiveText : palette_.switchInactiveText);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawFittedText(text, bounds.withTrimmedLeft(24.0f).withTrimmedRight(4.0f).toNearestInt(), juce::Justification::centred, 1);
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

    // Cyber Hex Socket
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
    g.setColour(palette.accentPrimary.withAlpha(0.35f));
    g.strokePath(hex, juce::PathStrokeType(0.8f));
}

void AnalogLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width, int height,
    bool /*isButtonDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& /*box*/
) {
    auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height)).reduced(1.0f);

    g.setColour(palette_.comboBg);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(palette_.comboBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

    // Subtle left neon accent bar
    g.setColour(palette_.accentPrimary.withAlpha(0.75f));
    g.fillRoundedRectangle(bounds.getX() + 2.5f, bounds.getY() + 4.0f, 2.5f, bounds.getHeight() - 8.0f, 1.0f);

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
    g.setColour(palette_.accentPrimary.withAlpha(0.6f));
    g.drawRect(bounds, 1.2f);
}

} // namespace analog_master::ui
