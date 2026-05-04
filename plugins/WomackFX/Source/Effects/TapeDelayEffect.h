#pragma once

#include "EffectBase.h"
#include <juce_audio_processors/juce_audio_processors.h>

class TapeDelayEffect : public EffectBase
{
public:
    TapeDelayEffect (juce::AudioProcessorValueTreeState& apvts);

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::dsp::AudioBlock<float>& block) override;
    void reset() override;

    // For visualization: current tape position [0, 1)
    float getTapePosition() const { return tapePosition.load(); }

private:
    juce::AudioProcessorValueTreeState& state;

    std::atomic<float>* enabledParam   = nullptr;
    std::atomic<float>* timeParam      = nullptr;
    std::atomic<float>* feedbackParam  = nullptr;
    std::atomic<float>* modParam       = nullptr;
    std::atomic<float>* satParam       = nullptr;
    std::atomic<float>* mixParam       = nullptr;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 192000 };
    juce::dsp::IIR::Filter<float> dampFilterL, dampFilterR;
    juce::dsp::DryWetMixer<float> dryWet;

    float lfoPhase = 0.0f;
    std::atomic<float> tapePosition { 0.0f };
    double sampleRate = 44100.0;

    // Feedback buffer
    float feedbackSampleL = 0.0f;
    float feedbackSampleR = 0.0f;
};
