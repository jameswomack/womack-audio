#include "UnivibeEffect.h"
#include "../Parameters.h"

UnivibeEffect::UnivibeEffect (juce::AudioProcessorValueTreeState& apvts)
    : state (apvts)
{
    enabledParam  = state.getRawParameterValue (ParamIDs::vibeEnabled);
    rateParam     = state.getRawParameterValue (ParamIDs::vibeRate);
    depthParam    = state.getRawParameterValue (ParamIDs::vibeDepth);
    feedbackParam = state.getRawParameterValue (ParamIDs::vibeFeedback);
    mixParam      = state.getRawParameterValue (ParamIDs::vibeMix);
}

void UnivibeEffect::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    phaser.prepare (spec);
    dryWet.prepare (spec);
}

void UnivibeEffect::process (juce::dsp::AudioBlock<float>& block)
{
    if (enabledParam->load() < 0.5f)
        return;

    auto rate     = rateParam->load();
    auto depth    = depthParam->load();
    auto feedback = feedbackParam->load();
    auto mix      = mixParam->load();

    phaser.setRate (rate);
    phaser.setDepth (depth);
    phaser.setFeedback (feedback);
    phaser.setMix (mix);

    // Track LFO phase for visualization
    float phaseInc = static_cast<float> (rate * juce::MathConstants<double>::twoPi
                                          * block.getNumSamples() / sampleRate);
    float newPhase = lfoPhase.load() + phaseInc;
    if (newPhase >= juce::MathConstants<float>::twoPi)
        newPhase -= juce::MathConstants<float>::twoPi;
    lfoPhase.store (newPhase);

    auto context = juce::dsp::ProcessContextReplacing<float> (block);
    phaser.process (context);
}

void UnivibeEffect::reset()
{
    phaser.reset();
    dryWet.reset();
    lfoPhase.store (0.0f);
}
