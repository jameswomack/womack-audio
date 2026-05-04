#pragma once

#include "EffectBase.h"
#include <juce_audio_processors/juce_audio_processors.h>

class UnivibeEffect : public EffectBase
{
public:
    UnivibeEffect (juce::AudioProcessorValueTreeState& apvts);

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::dsp::AudioBlock<float>& block) override;
    void reset() override;

    // For visualization: current LFO phase [0, 2pi)
    float getLfoPhase() const { return lfoPhase.load(); }

private:
    juce::AudioProcessorValueTreeState& state;

    std::atomic<float>* enabledParam   = nullptr;
    std::atomic<float>* rateParam      = nullptr;
    std::atomic<float>* depthParam     = nullptr;
    std::atomic<float>* feedbackParam  = nullptr;
    std::atomic<float>* mixParam       = nullptr;

    juce::dsp::Phaser<float> phaser;
    juce::dsp::DryWetMixer<float> dryWet;

    std::atomic<float> lfoPhase { 0.0f };
    double sampleRate = 44100.0;
};
