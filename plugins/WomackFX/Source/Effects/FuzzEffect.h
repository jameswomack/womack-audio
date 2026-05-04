#pragma once

#include "EffectBase.h"
#include <juce_audio_processors/juce_audio_processors.h>

class FuzzEffect : public EffectBase
{
public:
    FuzzEffect (juce::AudioProcessorValueTreeState& apvts);

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::dsp::AudioBlock<float>& block) override;
    void reset() override;

    // For visualization: get the transfer function output for a given input
    float getTransferValue (float input) const;

private:
    void updateParameters();

    juce::AudioProcessorValueTreeState& state;

    std::atomic<float>* enabledParam  = nullptr;
    std::atomic<float>* driveParam    = nullptr;
    std::atomic<float>* toneParam     = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* typeParam     = nullptr;

    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    juce::dsp::IIR::Filter<float> toneFilterL, toneFilterR;
    juce::dsp::DryWetMixer<float> dryWet;

    float currentDrive = 1.0f;
    int   currentType  = 0;
    float sampleRate   = 44100.0;
};
