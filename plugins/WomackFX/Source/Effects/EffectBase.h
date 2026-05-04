#pragma once

#include <juce_dsp/juce_dsp.h>

class EffectBase
{
public:
    virtual ~EffectBase() = default;

    virtual void prepare (const juce::dsp::ProcessSpec& spec) = 0;
    virtual void process (juce::dsp::AudioBlock<float>& block) = 0;
    virtual void reset() = 0;
};
