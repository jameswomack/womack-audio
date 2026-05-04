#include "TapeDelayEffect.h"
#include "../Parameters.h"

TapeDelayEffect::TapeDelayEffect (juce::AudioProcessorValueTreeState& apvts)
    : state (apvts)
{
    enabledParam  = state.getRawParameterValue (ParamIDs::delayEnabled);
    timeParam     = state.getRawParameterValue (ParamIDs::delayTime);
    feedbackParam = state.getRawParameterValue (ParamIDs::delayFeedback);
    modParam      = state.getRawParameterValue (ParamIDs::delayMod);
    satParam      = state.getRawParameterValue (ParamIDs::delaySat);
    mixParam      = state.getRawParameterValue (ParamIDs::delayMix);
}

void TapeDelayEffect::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples (static_cast<int> (spec.sampleRate * 2.0));

    dryWet.prepare (spec);

    auto dampCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 4000.0f);
    *dampFilterL.coefficients = *dampCoeffs;
    *dampFilterR.coefficients = *dampCoeffs;
}

void TapeDelayEffect::process (juce::dsp::AudioBlock<float>& block)
{
    if (enabledParam->load() < 0.5f)
        return;

    auto delayMs  = timeParam->load();
    auto feedback = feedbackParam->load();
    auto modDepth = modParam->load();
    auto sat      = satParam->load();
    auto mix      = mixParam->load();

    dryWet.setWetMixProportion (mix);
    dryWet.pushDrySamples (block);

    float delaySamples = static_cast<float> (delayMs * 0.001 * sampleRate);
    float lfoFreq = 0.5f; // Wow/flutter rate in Hz

    auto numSamples  = block.getNumSamples();
    auto numChannels = block.getNumChannels();

    for (size_t i = 0; i < numSamples; ++i)
    {
        // LFO for wow/flutter modulation
        float lfo = std::sin (lfoPhase) * modDepth * delaySamples * 0.01f;
        lfoPhase += static_cast<float> (juce::MathConstants<double>::twoPi * lfoFreq / sampleRate);
        if (lfoPhase >= juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        float modulatedDelay = juce::jmax (1.0f, delaySamples + lfo);

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            float inputSample = block.getChannelPointer (ch)[i];
            float& fbSample = (ch == 0) ? feedbackSampleL : feedbackSampleR;

            // Write input + feedback to delay line
            delayLine.pushSample (static_cast<int> (ch), inputSample + fbSample);

            // Read from delay
            float delayed = delayLine.popSample (static_cast<int> (ch), modulatedDelay);

            // Tape saturation on feedback path
            float saturated = std::tanh (delayed * (1.0f + sat * 3.0f));

            // Damping filter on feedback path
            auto& filter = (ch == 0) ? dampFilterL : dampFilterR;
            float filtered = filter.processSample (saturated);

            fbSample = filtered * feedback;

            block.getChannelPointer (ch)[i] = delayed;
        }
    }

    dryWet.mixWetSamples (block);

    // Update tape position for visualization
    float pos = tapePosition.load() + static_cast<float> (numSamples) / static_cast<float> (sampleRate) * 0.5f;
    if (pos >= 1.0f) pos -= 1.0f;
    tapePosition.store (pos);
}

void TapeDelayEffect::reset()
{
    delayLine.reset();
    dampFilterL.reset();
    dampFilterR.reset();
    dryWet.reset();
    feedbackSampleL = 0.0f;
    feedbackSampleR = 0.0f;
    lfoPhase = 0.0f;
    tapePosition.store (0.0f);
}
