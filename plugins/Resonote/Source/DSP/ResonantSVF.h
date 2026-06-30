#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

/** Unified zero-delay TPT state-variable filter with a nonlinear (tanh)
    resonance-feedback path. One core yields bell / low-pass / high-pass.
    The nonlinearity is the sole "warmth" source: near-clean at low resonance,
    increasingly colored (and self-oscillation-like) as resonance rises. */
class ResonantSVF
{
public:
    enum class Mode { bell = 0, lowpass = 1, highpass = 2 };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setMode (Mode m) noexcept        { mode = m; }
    void setFrequency (float hz) noexcept { freqSmoothed.setTargetValue (juce::jlimit (20.0f, 20000.0f, hz)); }
    void setResonance (float r01) noexcept{ resSmoothed.setTargetValue  (juce::jlimit (0.0f, 1.0f, r01)); }
    void setGainDb (float db) noexcept    { gainSmoothed.setTargetValue (db); }

    void process (juce::dsp::AudioBlock<float>& block) noexcept;

    /** Linear analog magnitude (gain) at hz for current settings. UI only. */
    float magnitudeAt (float hz) const noexcept;

private:
    static float resonanceToQ (float r01) noexcept { return 0.5f + r01 * r01 * 19.5f; }

    double sampleRate = 44100.0;
    Mode   mode = Mode::lowpass;

    juce::SmoothedValue<float> freqSmoothed { 220.0f };
    juce::SmoothedValue<float> resSmoothed  { 0.3f };
    juce::SmoothedValue<float> gainSmoothed { 0.0f };

    struct State { float ic1eq = 0.0f, ic2eq = 0.0f; };
    std::array<State, 2> chState;
};
