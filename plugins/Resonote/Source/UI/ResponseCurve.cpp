#include "ResponseCurve.h"
#include "../DSP/NoteFrequency.h"

float ResponseCurve::hzToX (float hz, juce::Rectangle<float> area) noexcept
{
    const float t = std::log (hz / minHz) / std::log (maxHz / minHz);
    return area.getX() + t * area.getWidth();
}

void ResponseCurve::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0d0d));

    auto area = getLocalBounds().toFloat().reduced (2.0f);

    // --- Note gridlines: draw every note, label C octaves. ---
    g.setFont (juce::FontOptions (9.0f));
    for (int midi = 12; midi <= 120; ++midi)               // C0..C9 region
    {
        const float hz = (float) NoteFrequency::midiToHz (midi);
        if (hz < minHz || hz > maxHz)
            continue;

        const bool isC = (((midi % 12) + 12) % 12) == 0;
        const float x = hzToX (hz, area);

        g.setColour (juce::Colours::white.withAlpha (isC ? 0.18f : 0.06f));
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());

        if (isC)
        {
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.drawText (NoteFrequency::noteName (midi),
                        juce::Rectangle<float> (x + 2.0f, area.getBottom() - 12.0f, 28.0f, 11.0f),
                        juce::Justification::left);
        }
    }

    // --- Magnitude curve (dB), -30..+30 dB range. ---
    juce::Path curve;
    bool started = false;
    const float dbMin = -30.0f, dbMax = 30.0f;

    for (float px = 0.0f; px <= area.getWidth(); px += 1.0f)
    {
        const float t  = px / area.getWidth();
        const float hz = minHz * std::pow (maxHz / minHz, t);
        const float mag = juce::jmax (1.0e-6f, proc.getMagnitudeAt (hz));
        const float db  = juce::Decibels::gainToDecibels (mag);

        const float y = juce::jmap (juce::jlimit (dbMin, dbMax, db),
                                    dbMin, dbMax, area.getBottom(), area.getY());

        if (! started) { curve.startNewSubPath (area.getX() + px, y); started = true; }
        else           { curve.lineTo (area.getX() + px, y); }
    }

    g.setColour (juce::Colour (0xff66ddff));
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    // --- Cutoff marker at the live frequency. ---
    const float cutoffHz = proc.getCurrentFrequencyHz();
    const float cx = hzToX (juce::jlimit (minHz, maxHz, cutoffHz), area);
    g.setColour (juce::Colour (0xffffaa33));
    g.drawVerticalLine ((int) cx, area.getY(), area.getBottom());

    const int midi = NoteFrequency::nearestMidi (cutoffHz);
    g.setColour (juce::Colour (0xffffaa33));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (NoteFrequency::noteName (midi),
                juce::Rectangle<float> (cx + 3.0f, area.getY() + 2.0f, 40.0f, 14.0f),
                juce::Justification::left);
}
