#include "ResponseCurve.h"
#include "../DSP/NoteFrequency.h"
#include <cmath>

void ResponseCurve::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0d0d));

    auto area = getLocalBounds().toFloat().reduced (2.0f);
    if (area.getWidth() < 4.0f || area.getHeight() < 4.0f)
        return;

    const float logRange = std::log (maxHz / minHz);
    auto hzToU = [logRange] (float hz) noexcept { return std::log (hz / minHz) / logRange; };
    auto uToHz = [logRange] (float u)  noexcept { return minHz * std::exp (u * logRange); };

    // Lens centered on the current cutoff (the note we care about).
    const float cutoffHz = juce::jlimit (minHz, maxHz, proc.getCurrentFrequencyHz());
    const float u0       = juce::jlimit (0.0f, 1.0f, hzToU (cutoffHz));

    // Horizontal fisheye: endpoints stay pinned to the frame; an erf "bump"
    // magnifies the region around u0 so nearby notes spread out and far ones
    // compress toward the edges (measuring tape behind a magnifying lens).
    constexpr float lensStrength = 0.45f;   // higher = stronger center zoom
    constexpr float lensWidth    = 0.09f;   // in u-units (~ a bit under one octave)
    const float bumpAt0 = std::erf ((0.0f - u0) / lensWidth);
    auto bump  = [=] (float u) noexcept { return lensStrength * (std::erf ((u - u0) / lensWidth) - bumpAt0); };
    const float denom = 1.0f + bump (1.0f);
    auto uToX  = [=] (float u) noexcept { return area.getX() + ((u + bump (u)) / denom) * area.getWidth(); };

    // Vertical convex bow: content lifts toward the lens center (subtle 3D glass).
    const float bowHeight = area.getHeight() * 0.10f;
    constexpr float bowWidth = 0.20f;
    auto bow = [=] (float u) noexcept { const float d = (u - u0) / bowWidth; return bowHeight * std::exp (-d * d); };

    // Depth fade: notes far from the lens recede (dimmer).
    auto depth = [=] (float u) noexcept { const float d = (u - u0) / 0.42f; return std::exp (-d * d); };

    // --- Note gridlines (every note; label the Cs). ---
    const bool  scaleActive = proc.isSnapEnabled() && proc.getScaleType() != 0;
    const int   scaleRoot   = proc.getScaleRoot();
    const auto  scaleType   = static_cast<NoteFrequency::Scale> (proc.getScaleType());

    g.setFont (juce::FontOptions (9.0f));
    const int midiMax = NoteFrequency::nearestMidi (maxHz) + 1;
    for (int midi = 12; midi <= midiMax; ++midi)
    {
        const float hz = (float) NoteFrequency::midiToHz (midi);
        if (hz < minHz || hz > maxHz)
            continue;

        const float u = hzToU (hz);
        const float x = uToX (u);
        const float b = bow (u);
        const float a = depth (u);
        const bool  isC = (((midi % 12) + 12) % 12) == 0;

        const bool highlight = isC || (scaleActive && NoteFrequency::inScale (midi, scaleRoot, scaleType));
        g.setColour (juce::Colours::white.withAlpha ((highlight ? 0.22f : 0.07f) * (0.35f + 0.65f * a)));
        g.drawVerticalLine ((int) x, area.getY() + (bowHeight - b), area.getBottom() - b * 0.4f);

        if (isC)
        {
            g.setColour (juce::Colours::white.withAlpha (0.28f + 0.32f * a));
            g.drawText (NoteFrequency::noteName (midi),
                        juce::Rectangle<float> (x + 2.0f, area.getBottom() - b * 0.4f - 12.0f, 30.0f, 11.0f),
                        juce::Justification::left);
        }
    }

    // --- Magnitude curve (dB, -30..+30) sampled across frequency, through the lens. ---
    constexpr float dbMin = -30.0f, dbMax = 30.0f;
    const int N = juce::jmax (400, (int) area.getWidth());
    juce::Path curve;
    for (int i = 0; i <= N; ++i)
    {
        const float u   = (float) i / (float) N;
        const float hz  = uToHz (u);
        const float mag = juce::jmax (1.0e-6f, proc.getMagnitudeAt (hz));
        const float db  = juce::Decibels::gainToDecibels (mag);
        const float x   = uToX (u);
        const float y   = juce::jmap (juce::jlimit (dbMin, dbMax, db),
                                      dbMin, dbMax, area.getBottom(), area.getY() + bowHeight) - bow (u);
        if (i == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }
    g.setColour (juce::Colour (0xff66ddff));
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    // --- Cutoff marker at the lens center. ---
    const float cx = uToX (u0);
    const float cb = bow (u0);
    g.setColour (juce::Colour (0xffffaa33));
    g.drawVerticalLine ((int) cx, area.getY() + (bowHeight - cb), area.getBottom() - cb * 0.4f);

    const int midi = NoteFrequency::nearestMidi (cutoffHz);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    const float labelX = juce::jlimit (area.getX() + 2.0f, area.getRight() - 44.0f, cx + 4.0f);
    g.drawText (NoteFrequency::noteName (midi),
                juce::Rectangle<float> (labelX, area.getY() + (bowHeight - cb) + 1.0f, 42.0f, 15.0f),
                juce::Justification::left);
}
