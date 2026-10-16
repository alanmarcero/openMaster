#include "PluginProcessor.h"
#include "PluginEditor.h"

AnalogMasterProcessor::AnalogMasterProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("AnalogMasterParameters"), analog_master::params::createParameterLayout()) {
}

void AnalogMasterProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
    core_.prepare(sampleRate);
    syncParameters();
}

void AnalogMasterProcessor::releaseResources() {
    core_.reset();
}

bool AnalogMasterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != juce::AudioChannelSet::mono() && mainInput != juce::AudioChannelSet::stereo()) {
        return false;
    }

    return mainInput == mainOutput;
}

void AnalogMasterProcessor::syncParameters() {
    using namespace analog_master::params;

    // 1. Low Cut
    const float lowCutFreq = *parameters.getRawParameterValue(kLowCutFreq);
    const int lowCutSlopeIdx = static_cast<int>(*parameters.getRawParameterValue(kLowCutSlope));
    const bool lowCutIn = *parameters.getRawParameterValue(kLowCutIn) > 0.5f;

    core_.setLowCutFrequency(lowCutFreq);
    core_.setLowCutSlope(static_cast<analog_master::core::FilterSlope>(lowCutSlopeIdx));
    core_.setLowCutBypassed(!lowCutIn);

    // 2. Stereo Field
    const float stereoFreq = *parameters.getRawParameterValue(kStereoFreq);
    const float stereoWidth = *parameters.getRawParameterValue(kStereoWidth) / 100.0f; // 0..200% -> 0..2.0
    const bool stereoIn = *parameters.getRawParameterValue(kStereoIn) > 0.5f;
    const bool stereoSolo = *parameters.getRawParameterValue(kStereoSoloLow) > 0.5f;

    core_.setStereoFieldFrequency(stereoFreq);
    core_.setStereoFieldWidth(stereoWidth);
    core_.setStereoFieldBypassed(!stereoIn);
    core_.setStereoFieldSoloLow(stereoSolo);

    // 3. High Cut
    const float highCutFreq = *parameters.getRawParameterValue(kHighCutFreq);
    const int highCutSlopeIdx = static_cast<int>(*parameters.getRawParameterValue(kHighCutSlope));
    const bool highCutIn = *parameters.getRawParameterValue(kHighCutIn) > 0.5f;

    core_.setHighCutFrequency(highCutFreq);
    core_.setHighCutSlope(static_cast<analog_master::core::FilterSlope>(highCutSlopeIdx));
    core_.setHighCutBypassed(!highCutIn);

    // 4. Master
    const float masterGain = *parameters.getRawParameterValue(kMasterGain);
    const bool masterPower = *parameters.getRawParameterValue(kMasterPower) > 0.5f;

    core_.setMasterGainDb(masterGain);
    core_.setMasterBypassed(!masterPower);
}

void AnalogMasterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/) {
    juce::ScopedNoDenormals noDenormals;

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

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    core_.processBlock(leftChannel, rightChannel, numSamples);
}

juce::AudioProcessorEditor* AnalogMasterProcessor::createEditor() {
    return new AnalogMasterEditor(*this);
}

void AnalogMasterProcessor::getStateInformation(juce::MemoryBlock& destination) {
    if (auto xmlState = parameters.copyState().createXml()) {
        copyXmlToBinary(*xmlState, destination);
    }
}

void AnalogMasterProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes)) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
            syncParameters();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AnalogMasterProcessor();
}
