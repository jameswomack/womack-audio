#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Effects/FuzzEffect.h"
#include "Effects/UnivibeEffect.h"
#include "Effects/TapeDelayEffect.h"

class WomackFXProcessor : public juce::AudioProcessor
{
public:
    WomackFXProcessor();
    ~WomackFXProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    FuzzEffect&      getFuzz()      { return fuzz; }
    UnivibeEffect&   getUnivibe()   { return univibe; }
    TapeDelayEffect& getTapeDelay() { return tapeDelay; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    FuzzEffect      fuzz;
    UnivibeEffect   univibe;
    TapeDelayEffect tapeDelay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WomackFXProcessor)
};
