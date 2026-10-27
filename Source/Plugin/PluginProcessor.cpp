#include "PluginProcessor.h"
#include "PluginEditor.h"

AnalogMasterProcessor::AnalogMasterProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("OpenMasterParameters"),
                 analog_master::params::createParameterLayout()) {
    for (auto *parameter : getParameters()) {
        if (const auto *identified = dynamic_cast<juce::AudioProcessorParameterWithID *>(parameter))
            rawParameters_.emplace(identified->paramID.toStdString(),
                                   parameters.getRawParameterValue(identified->paramID));
    }
}

void AnalogMasterProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
    syncParameters();
    core_.prepare(sampleRate, getTotalNumInputChannels());
    setLatencySamples(core_.latencySamples());
}

void AnalogMasterProcessor::releaseResources() {
    core_.reset();
}

bool AnalogMasterProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
    const auto &mainInput = layouts.getMainInputChannelSet();
    const auto &mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != juce::AudioChannelSet::mono() && mainInput != juce::AudioChannelSet::stereo()) {
        return false;
    }

    return mainInput == mainOutput;
}

void AnalogMasterProcessor::syncParameters() {
    using namespace analog_master::params;
    const auto value = [this](std::string_view id) {
        return rawParameters_.find(id)->second->load(std::memory_order_relaxed);
    };

    // 1. Low Cut
    const float lowCutFreq = value(kLowCutFreq);
    const int lowCutSlopeIdx = static_cast<int>(value(kLowCutSlope));
    const bool lowCutIn = value(kLowCutIn) > 0.5f;

    core_.setLowCutFrequency(lowCutFreq);
    core_.setLowCutSlope(static_cast<analog_master::core::FilterSlope>(lowCutSlopeIdx));
    core_.setLowCutBypassed(!lowCutIn);

    // 2. Stereo Field
    const float stereoFreq = value(kStereoFreq);
    const float stereoWidth = value(kStereoWidth) / 100.0f; // 0..200% -> 0..2.0
    const bool stereoIn = value(kStereoIn) > 0.5f;
    const bool stereoSolo = value(kStereoSoloLow) > 0.5f;

    core_.setStereoFieldFrequency(stereoFreq);
    core_.setStereoFieldWidth(stereoWidth);
    core_.setStereoFieldBypassed(!stereoIn);
    core_.setStereoFieldSoloLow(stereoSolo);

    // 3. High Cut
    const float highCutFreq = value(kHighCutFreq);
    const int highCutSlopeIdx = static_cast<int>(value(kHighCutSlope));
    const bool highCutIn = value(kHighCutIn) > 0.5f;

    core_.setHighCutFrequency(highCutFreq);
    core_.setHighCutSlope(static_cast<analog_master::core::FilterSlope>(highCutSlopeIdx));
    core_.setHighCutBypassed(!highCutIn);

    // 4. Master
    const float masterGain = value(kMasterGain);
    const bool masterPower = value(kMasterPower) > 0.5f;

    core_.setMasterGainDb(masterGain);
    core_.setMasterBypassed(!masterPower);
    using namespace analog_master::core;
    ToneSettings tone;
    tone.enabled = value("tone_in") > .5f;
    tone.lowDb = value("tone_low");
    tone.highDb = value("tone_high");
    tone.tiltDb = value("tone_tilt");
    tone.lowHz = value("tone_low_hz");
    tone.highHz = value("tone_high_hz");
    core_.setTone(tone);
    CompressorSettings comp;
    comp.enabled = value("comp_in") > .5f;
    comp.thresholdDb = value("comp_threshold");
    comp.ratio = value("comp_ratio");
    comp.attackMs = value("comp_attack");
    comp.releaseMs = value("comp_release");
    comp.kneeDb = value("comp_knee");
    comp.mix = value("comp_mix") * .01f;
    comp.makeupDb = value("comp_makeup");
    comp.sidechainHz = value("comp_sc_hz");
    core_.setCompressor(comp);
    DynamicEqSettings dyn;
    dyn.enabled = value("dyn_in") > .5f;
    dyn.amount = value("dyn_amount") * .01f;
    dyn.soloBand = static_cast<int>(value("dyn_solo")) - 1;
    static const std::array<std::array<const char *, 6>, 3> bandIds{
        {{{"dyn_0_freq", "dyn_0_q", "dyn_0_threshold", "dyn_0_range", "dyn_0_attack", "dyn_0_release"}},
         {{"dyn_1_freq", "dyn_1_q", "dyn_1_threshold", "dyn_1_range", "dyn_1_attack", "dyn_1_release"}},
         {{"dyn_2_freq", "dyn_2_q", "dyn_2_threshold", "dyn_2_range", "dyn_2_attack", "dyn_2_release"}}}};
    for (size_t i = 0; i < dyn.bands.size(); ++i) {
        const auto &ids = bandIds[i];
        dyn.bands[i] = {value(ids[0]), value(ids[1]), value(ids[2]),
                        value(ids[3]), value(ids[4]), value(ids[5])};
    }
    core_.setDynamicEq(dyn);
    SaturationSettings sat;
    sat.enabled = value("sat_in") > .5f;
    sat.autoGain = value("sat_auto") > .5f;
    sat.character = static_cast<int>(value("sat_character"));
    sat.driveDb = value("sat_drive");
    sat.mix = value("sat_mix") * .01f;
    sat.outputDb = value("sat_output");
    core_.setSaturation(sat);
    ClipperSettings clip;
    clip.enabled = value("clip_in") > .5f;
    clip.shape = static_cast<int>(value("clip_shape"));
    clip.driveDb = value("clip_drive");
    clip.ceilingDb = value("clip_ceiling");
    clip.mix = value("clip_mix") * .01f;
    core_.setClipper(clip);
    LimiterSettings limit;
    limit.enabled = value("limit_in") > .5f;
    limit.driveDb = value("limit_drive");
    limit.ceilingDb = value("limit_ceiling");
    limit.releaseMs = value("limit_release");
    core_.setLimiter(limit);
    core_.setMatchBypass(value("match_bypass") > .5f);
}

void AnalogMasterProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer & /*midi*/) {
    processAudio(buffer, false);
}

void AnalogMasterProcessor::processBlockBypassed(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) {
    processAudio(buffer, true);
}

void AnalogMasterProcessor::processAudio(juce::AudioBuffer<float> &buffer, bool hostBypassed) {
    juce::ScopedNoDenormals noDenormals;
    core_.setHostBypassed(hostBypassed);

    syncParameters();

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear unused output channels to prevent emitting garbage/uninitialized memory
    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch) {
        buffer.clear(ch, 0, numSamples);
    }

    if (numSamples == 0 || totalNumInputChannels == 0) {
        return;
    }

    float *leftChannel = buffer.getWritePointer(0);
    float *rightChannel = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    core_.processBlock(leftChannel, rightChannel, numSamples);
}

juce::AudioProcessorEditor *AnalogMasterProcessor::createEditor() {
    return new AnalogMasterEditor(*this);
}

void AnalogMasterProcessor::getStateInformation(juce::MemoryBlock &destination) {
    if (auto xmlState = parameters.copyState().createXml()) {
        copyXmlToBinary(*xmlState, destination);
    }
}

void AnalogMasterProcessor::setStateInformation(const void *data, int sizeInBytes) {
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes)) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            auto incoming = juce::ValueTree::fromXml(*xmlState);
            // Old sessions contain no mastering parameters: restore their defaults even
            // when loading into an instance whose new stages were already enabled.
            for (auto *parameter : getParameters()) {
                if (const auto *identified = dynamic_cast<juce::RangedAudioParameter *>(parameter)) {
                    if (!incoming.getChildWithProperty("id", identified->paramID).isValid()) {
                        juce::ValueTree value("PARAM");
                        value.setProperty("id", identified->paramID, nullptr);
                        value.setProperty("value", identified->convertFrom0to1(identified->getDefaultValue()),
                                          nullptr);
                        incoming.addChild(value, -1, nullptr);
                    }
                }
            }
            parameters.replaceState(incoming);
            // Audio state is updated by the next callback, never from the state-loading thread.
        }
    }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new AnalogMasterProcessor();
}
