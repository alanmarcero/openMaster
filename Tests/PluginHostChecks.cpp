#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <set>
static void require(bool b, const char *message) {
    if (!b)
        throw std::runtime_error(message);
}
static void pump() {
    juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
}
static juce::Component *find(juce::Component &c, const juce::String &id) {
    if (c.getComponentID() == id)
        return &c;
    for (auto *child : c.getChildren())
        if (auto *found = find(*child, id))
            return found;
    return nullptr;
}
static void save(juce::Component &c, const juce::String &path) {
    juce::File f(path);
    f.deleteFile();
    juce::FileOutputStream out(f);
    require(juce::PNGImageFormat().writeImageToStream(
                c.createComponentSnapshot(c.getLocalBounds(), true, 1.5f), out),
            "capture");
}
int main(int argc, char **argv) {
    const juce::File captureDir = argc > 1 ? juce::File(argv[1]) : juce::File{};
    if (argc > 1)
        captureDir.createDirectory();
    juce::ScopedJuceInitialiser_GUI gui;
    AnalogMasterProcessor p;
    p.setRateAndBufferSizeDetails(48000, 256);
    p.prepareToPlay(48000, 256);
    require(p.getLatencySamples() > 0, "host latency reported");
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    pump();
    require(editor->getWidth() == 1120 && editor->getHeight() == 720, "editor size");
    std::set<juce::String> seen;
    for (int section = 0; section < 6; ++section) {
        auto *b = dynamic_cast<juce::Button *>(find(*editor, "section_" + juce::String(section)));
        require(b != nullptr, "section");
        b->triggerClick();
        pump();
        auto *mode = dynamic_cast<juce::ComboBox *>(find(*editor, "detail_mode"));
        for (int variant = 1; variant <= (section == 3 || section == 4 ? 2 : 1); ++variant) {
            if (mode && mode->isVisible()) {
                mode->setSelectedId(variant);
                pump();
            }
            for (int band = 1; band <= (section == 3 && variant == 2 ? 3 : 1); ++band) {
                if (auto *bandBox = dynamic_cast<juce::ComboBox *>(find(*editor, "dynamic_band"));
                    bandBox && bandBox->isVisible()) {
                    bandBox->setSelectedId(band);
                    pump();
                }
                for (auto *child : editor->getChildren())
                    if (child->isVisible() && !child->getComponentID().isEmpty()) {
                        require(editor->getLocalBounds().contains(child->getBounds()),
                                "control outside editor");
                        seen.insert(child->getComponentID());
                        if (auto *slider = dynamic_cast<juce::Slider *>(child)) {
                            auto *parameter = p.parameters.getParameter(slider->getComponentID());
                            require(parameter != nullptr, "slider parameter");
                            const auto before = parameter->getValue();
                            slider->setValue(slider->getRange().getStart() +
                                                 (slider->getRange().getLength() * .4),
                                             juce::sendNotificationSync);
                            pump();
                            require(std::abs(parameter->convertFrom0to1(parameter->getValue()) -
                                             slider->getValue()) < .05,
                                    "slider attachment");
                            parameter->setValueNotifyingHost(before);
                            pump();
                        }
                    }
            }
            if (argc > 1)
                save(*editor, captureDir
                                  .getChildFile("section-" + juce::String(section) + "-" +
                                                juce::String(variant) + ".png")
                                  .getFullPathName());
        }
    }
    for (auto *base : p.getParameters()) {
        auto *parameter = dynamic_cast<juce::AudioProcessorParameterWithID *>(base);
        require(parameter && seen.count(parameter->paramID), "parameter not reachable in UI");
    }
    auto *high = dynamic_cast<juce::Slider *>(find(*editor, "high_cut_freq"));
    require(high, "highcut");
    require(std::abs(high->getValueFromText("16 kHz") - 16000) < .01, "highcut typed kHz");
    for (auto *parameter : p.getParameters()) {
        auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(parameter);
        require(ranged, "ranged parameter");
        ranged->setValueNotifyingHost(ranged->convertTo0to1(ranged->convertFrom0to1(.61f)));
    }
    juce::MemoryBlock state;
    p.getStateInformation(state);
    AnalogMasterProcessor q;
    q.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    for (auto *base : p.getParameters()) {
        auto *parameter = dynamic_cast<juce::AudioProcessorParameterWithID *>(base);
        const auto restored = q.parameters.getParameter(parameter->paramID)->getValue();
        if (std::abs(parameter->getValue() - restored) >= 1e-6)
            std::cerr << parameter->paramID << " before=" << parameter->getValue() << " restored=" << restored
                      << "\n";
        require(std::abs(parameter->getValue() - restored) < 1e-6, "state roundtrip");
    }
    auto legacy = p.parameters.copyState();
    const std::set<juce::String> oldIds{"low_cut_freq",   "low_cut_slope", "low_cut_in",      "stereo_freq",
                                        "stereo_width",   "stereo_in",     "stereo_solo_low", "high_cut_freq",
                                        "high_cut_slope", "high_cut_in",   "master_gain",     "master_power",
                                        "theme",          "meter_ref"};
    for (int i = legacy.getNumChildren() - 1; i >= 0; --i)
        if (!oldIds.count(legacy.getChild(i).getProperty("id").toString()))
            legacy.removeChild(i, nullptr);
    juce::MemoryBlock oldState;
    auto xml = legacy.createXml();
    juce::AudioProcessor::copyXmlToBinary(*xml, oldState);
    q.setStateInformation(oldState.getData(), static_cast<int>(oldState.getSize()));
    require(q.parameters.getRawParameterValue("comp_in")->load() < .5f &&
                q.parameters.getRawParameterValue("limit_in")->load() < .5f,
            "old sessions turn new stages off");
    require(std::abs(q.parameters.getParameter("low_cut_freq")->getValue() -
                     p.parameters.getParameter("low_cut_freq")->getValue()) < 1e-6,
            "old session original values");
    for (auto *parameter : p.getParameters())
        parameter->setValueNotifyingHost(parameter->getDefaultValue());
    auto *first = dynamic_cast<juce::Button *>(find(*editor, "section_0"));
    first->triggerClick();
    pump();
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    for (int block = 0; block < 620; ++block) {
        for (int i = 0; i < 256; ++i) {
            const auto x = .2f * std::sin(float(block * 256 + i) * .13f);
            audio.setSample(0, i, x);
            audio.setSample(1, i, x * .8f);
        }
        p.processBlock(audio, midi);
    }
    pump();
    if (argc > 1)
        save(*editor, captureDir.getChildFile("overview.png").getFullPathName());
    auto *theme = dynamic_cast<juce::ComboBox *>(find(*editor, "theme"));
    require(theme, "theme selector");
    for (int i = 1; i <= 5; ++i) {
        theme->setSelectedId(i);
        pump();
        if (argc > 1)
            save(*editor, captureDir.getChildFile("theme-" + juce::String(i) + ".png").getFullPathName());
    }
    p.reset();
    for (int block = 0; block < 12; ++block) {
        audio.clear();
        p.processBlock(audio, midi);
        require(audio.getMagnitude(0, audio.getNumSamples()) < 1e-9,
                "host reset flushes previous transport audio");
    }
    std::cout << "Native UI: all " << p.getParameters().size()
              << " parameters reachable, sliders wired, 8 detail views captured; state roundtrip, legacy "
                 "restore, five themes and typed kHz passed. Latency "
              << p.getLatencySamples() << " samples.\n";
}
