#include "ResonantSVF.h"
#include <cmath>

void ResonantSVF::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    const double rampSeconds = 0.02;
    freqSmoothed.reset (sampleRate, rampSeconds);
    resSmoothed.reset  (sampleRate, rampSeconds);
    gainSmoothed.reset (sampleRate, rampSeconds);

    reset();
}

void ResonantSVF::reset()
{
    for (auto& s : chState) { s.ic1eq = 0.0f; s.ic2eq = 0.0f; }
}

void ResonantSVF::process (juce::dsp::AudioBlock<float>& block) noexcept
{
    const auto numCh      = juce::jmin (chState.size(), block.getNumChannels());
    const auto numSamples = block.getNumSamples();
    jassert (block.getNumChannels() <= chState.size());

    std::array<float*, 2> ptrs {};
    for (size_t ch = 0; ch < numCh; ++ch)
        ptrs[ch] = block.getChannelPointer (ch);

    // sentinels force a compute on the first sample
    float lastFc = -1.0f, lastR = -1.0f, lastDb = -1000.0f;
    float k = 0.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, satDrive = 1.0f, bellScale = 0.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float fc = freqSmoothed.getNextValue();
        const float r  = resSmoothed.getNextValue();
        const float db = gainSmoothed.getNextValue();

        if (fc != lastFc || r != lastR || db != lastDb)   // recompute only on change
        {
            lastFc = fc; lastR = r; lastDb = db;

            const float Q = resonanceToQ (r);
            k = 1.0f / Q;

            const float A = std::pow (10.0f, db / 40.0f);   // only needed/used for bell
            bellScale = (A * A - 1.0f) * k;

            float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sampleRate);
            g = juce::jlimit (0.0f, 100.0f, g);
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;

            satDrive = 1.0f + r * 3.0f;   // warmth scales with resonance
        }

        for (size_t ch = 0; ch < numCh; ++ch)
        {
            auto& st    = chState[ch];
            float* data = ptrs[ch];
            const float v0 = data[i];

            float v3 = v0 - st.ic2eq;
            v3 = std::tanh (v3 * satDrive) / satDrive;   // nonlinear feedback

            const float v1 = a1 * st.ic1eq + a2 * v3;
            const float v2 = st.ic2eq + a2 * st.ic1eq + a3 * v3;

            st.ic1eq = 2.0f * v1 - st.ic1eq;
            st.ic2eq = 2.0f * v2 - st.ic2eq;

            const float low  = v2;
            const float band = v1;
            const float high = v0 - k * v1 - v2;

            float out;
            switch (mode)
            {
                case Mode::lowpass:  out = low;  break;
                case Mode::highpass: out = high; break;
                case Mode::bell:
                default:             out = v0 + bellScale * band; break;
            }

            if (! std::isfinite (out))
            {
                out = 0.0f;
                st.ic1eq = 0.0f;
                st.ic2eq = 0.0f;
            }

            data[i] = out;
        }
    }
}

float ResonantSVF::magnitudeAt (float hz) const noexcept
{
    // Analog-prototype magnitude; accurate well below Nyquist (UI response curve only).
    const float fc = juce::jmax (1.0f, freqSmoothed.getCurrentValue());
    const float Q  = resonanceToQ (resSmoothed.getCurrentValue());
    const float A  = std::pow (10.0f, gainSmoothed.getCurrentValue() / 40.0f);

    const float wn  = hz / fc;
    const float wn2 = wn * wn;
    const float base = (1.0f - wn2) * (1.0f - wn2);

    switch (mode)
    {
        case Mode::lowpass:
            return 1.0f / std::sqrt (base + (wn / Q) * (wn / Q));
        case Mode::highpass:
            return wn2 / std::sqrt (base + (wn / Q) * (wn / Q));
        case Mode::bell:
        default:
        {
            const float num = base + (wn * A / Q) * (wn * A / Q);
            const float den = base + (wn / (A * Q)) * (wn / (A * Q));
            return std::sqrt (num / den);
        }
    }
}
