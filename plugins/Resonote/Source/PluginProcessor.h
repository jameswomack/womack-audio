#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "DSP/ResonantSVF.h"

class ResonoteProcessor : public juce::AudioProcessor
{
public:
    ResonoteProcessor();
    ~ResonoteProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Womack Resonote"; }
    bool acceptsMidi() const override  { return true; }
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

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    /** Most-recently-applied filter frequency (post snap / MIDI). UI readout. */
    float getCurrentFrequencyHz() const noexcept { return currentFreqHz.load(); }

    /** Linear magnitude of the live filter at hz. UI response curve. */
    float getMagnitudeAt (float hz) const noexcept { return svf.magnitudeAt (hz); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    ResonantSVF svf;

    std::atomic<float>* modeParam      = nullptr;
    std::atomic<float>* freqParam      = nullptr;
    std::atomic<float>* snapParam      = nullptr;
    std::atomic<float>* resParam       = nullptr;
    std::atomic<float>* gainParam      = nullptr;
    std::atomic<float>* midiTrackParam = nullptr;
    std::atomic<float>* outputParam    = nullptr;

    int lastMidiNote = -1;
    std::atomic<float> currentFreqHz { 220.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonoteProcessor)
};
