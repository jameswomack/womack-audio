#include "FuzzEffect.h"
#include "../Parameters.h"

FuzzEffect::FuzzEffect (juce::AudioProcessorValueTreeState& apvts)
    : state (apvts)
{
    enabledParam = state.getRawParameterValue (ParamIDs::fuzzEnabled);
    driveParam   = state.getRawParameterValue (ParamIDs::fuzzDrive);
    toneParam    = state.getRawParameterValue (ParamIDs::fuzzTone);
    mixParam     = state.getRawParameterValue (ParamIDs::fuzzMix);
    typeParam    = state.getRawParameterValue (ParamIDs::fuzzType);
}

void FuzzEffect::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    oversampling.initProcessing (spec.maximumBlockSize);

    dryWet.prepare (spec);
    dryWet.setWetLatency (oversampling.getLatencyInSamples());

    toneFilterL.reset();
    toneFilterR.reset();
}

void FuzzEffect::process (juce::dsp::AudioBlock<float>& block)
{
    if (enabledParam->load() < 0.5f)
        return;

    updateParameters();

    dryWet.setWetMixProportion (mixParam->load());
    dryWet.pushDrySamples (block);

    // Upsample
    auto oversampledBlock = oversampling.processSamplesUp (block);

    // Apply waveshaping
    for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer (ch);
        for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
            data[i] = getTransferValue (data[i]);
    }

    // Downsample
    oversampling.processSamplesDown (block);

    // Tone filter
    auto toneFreq = toneParam->load();
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, toneFreq);
    *toneFilterL.coefficients = *coeffs;
    *toneFilterR.coefficients = *coeffs;

    for (int i = 0; i < (int) block.getNumSamples(); ++i)
    {
        if (block.getNumChannels() > 0)
            block.getChannelPointer (0)[i] = toneFilterL.processSample (block.getChannelPointer (0)[i]);
        if (block.getNumChannels() > 1)
            block.getChannelPointer (1)[i] = toneFilterR.processSample (block.getChannelPointer (1)[i]);
    }

    dryWet.mixWetSamples (block);
}

void FuzzEffect::reset()
{
    oversampling.reset();
    toneFilterL.reset();
    toneFilterR.reset();
    dryWet.reset();
}

void FuzzEffect::updateParameters()
{
    currentDrive = driveParam->load();
    currentType  = static_cast<int> (typeParam->load());
}

float FuzzEffect::getTransferValue (float input) const
{
    float driven = input * currentDrive;

    switch (currentType)
    {
        case 0: // Tanh soft clip
            return std::tanh (driven);

        case 1: // Hard clip
            return juce::jlimit (-1.0f, 1.0f, driven);

        case 2: // Asymmetric
        {
            if (driven >= 0.0f)
                return std::tanh (driven);
            else
                return std::tanh (driven * 0.5f);
        }

        default:
            return std::tanh (driven);
    }
}
