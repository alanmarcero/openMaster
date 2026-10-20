#pragma once

#include "MasterProcessorCore.h"
#include "Parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>

class AnalogMasterProcessor final : public juce::AudioProcessor {
public:
    AnalogMasterProcessor();
    ~AnalogMasterProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destination) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    [[nodiscard]] analog_master::core::MeterLevels takeMeterLevels() noexcept {
        return core_.takeMeterLevels();
    }

    juce::AudioProcessorValueTreeState parameters;

private:
    void syncParameters();

    analog_master::core::MasterProcessorCore core_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogMasterProcessor)
};
