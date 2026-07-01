#include <juce_core/juce_core.h>
#include "../Source/DSP/NoteFrequency.h"
#include "../Source/DSP/ResonantSVF.h"

class NoteFrequencyTests : public juce::UnitTest
{
public:
    NoteFrequencyTests() : juce::UnitTest ("NoteFrequency") {}

    void runTest() override
    {
        beginTest ("midiToHz reference pitches");
        expectWithinAbsoluteError (NoteFrequency::midiToHz (69), 440.0, 1.0e-6);   // A4
        expectWithinAbsoluteError (NoteFrequency::midiToHz (81), 880.0, 1.0e-6);   // A5
        expectWithinAbsoluteError (NoteFrequency::midiToHz (60), 261.6255653, 1.0e-3); // C4

        beginTest ("nearestMidi rounds to closest note");
        expect (NoteFrequency::nearestMidi (440.0) == 69);
        expect (NoteFrequency::nearestMidi (445.0) == 69);
        expect (NoteFrequency::nearestMidi (466.2) == 70); // A#4 = 466.16 Hz

        beginTest ("noteName scientific pitch notation");
        expect (NoteFrequency::noteName (69) == "A4");
        expect (NoteFrequency::noteName (60) == "C4");
        expect (NoteFrequency::noteName (61) == "C#4");
        expect (NoteFrequency::noteName (21) == "A0");
        expect (NoteFrequency::noteName (12) == "C0");

        beginTest ("centsFromNearest");
        expectWithinAbsoluteError (NoteFrequency::centsFromNearest (440.0), 0.0, 1.0e-4);
        // 40 cents above A4 -> nearest note is A4, +40 cents
        const double sharpOfA4 = 440.0 * std::pow (2.0, 0.4 / 12.0);
        expectWithinAbsoluteError (NoteFrequency::centsFromNearest (sharpOfA4), 40.0, 1.0e-3);
        // 60 cents above A4 -> nearest note is A#4, ~-40 cents
        const double flatOfASharp4 = 440.0 * std::pow (2.0, 0.6 / 12.0);
        expectWithinAbsoluteError (NoteFrequency::centsFromNearest (flatOfASharp4), -40.0, 1.0e-3);

        beginTest ("snapToNote returns exact note frequency");
        expectWithinAbsoluteError (NoteFrequency::snapToNote (445.0), 440.0, 1.0e-6);
        expectWithinAbsoluteError (NoteFrequency::snapToNote (261.0), NoteFrequency::midiToHz (60), 1.0e-6);

        beginTest ("inScale membership");
        // C major (root 0): C,D,E,F,G,A,B in; C#,D#,F#,G#,A# out
        expect (  NoteFrequency::inScale (60, 0, NoteFrequency::Scale::major)); // C4
        expect (! NoteFrequency::inScale (61, 0, NoteFrequency::Scale::major)); // C#4
        expect (  NoteFrequency::inScale (62, 0, NoteFrequency::Scale::major)); // D4
        // A natural minor (root 9) = white keys: A,B,C,D,E,F,G
        expect (  NoteFrequency::inScale (69, 9, NoteFrequency::Scale::minor)); // A4
        expect (! NoteFrequency::inScale (70, 9, NoteFrequency::Scale::minor)); // A#4
        expect (  NoteFrequency::inScale (72, 9, NoteFrequency::Scale::minor)); // C5
        // chromatic: everything in
        expect (  NoteFrequency::inScale (61, 0, NoteFrequency::Scale::chromatic));

        beginTest ("snapToScale");
        // Chromatic path matches snapToNote
        expectWithinAbsoluteError (NoteFrequency::snapToScale (445.0, 0, NoteFrequency::Scale::chromatic),
                                   NoteFrequency::snapToNote (445.0), 1.0e-6);
        // In C major, C#5 (midi 73) is out of key and exactly equidistant from
        // C5 (72) and D5 (74); the ascending search resolves ties to the lower note.
        {
            const double cSharp5 = NoteFrequency::midiToHz (73);
            const double snapped = NoteFrequency::snapToScale (cSharp5, 0, NoteFrequency::Scale::major);
            expect (NoteFrequency::nearestMidi (snapped) == 72);  // C5 (lower note wins on a tie)
        }
        // An in-key note snaps to itself (F4 in C major)
        expectWithinAbsoluteError (NoteFrequency::snapToScale (NoteFrequency::midiToHz (65), 0, NoteFrequency::Scale::major),
                                   NoteFrequency::midiToHz (65), 1.0e-6);
        // Root-relative snapping: D#4 (midi 63) is out of A natural minor; nearest
        // in-key notes are D4 (62) and E4 (64), tie resolves to D4.
        {
            const double dSharp4 = NoteFrequency::midiToHz (63);
            const double snapped = NoteFrequency::snapToScale (dSharp4, 9, NoteFrequency::Scale::minor);
            const int    snappedMidi = NoteFrequency::nearestMidi (snapped);
            expect (snappedMidi == 62);
            expect (NoteFrequency::inScale (snappedMidi, 9, NoteFrequency::Scale::minor));
        }
    }
};

static NoteFrequencyTests noteFrequencyTests;

class ResonantSVFTests : public juce::UnitTest
{
public:
    ResonantSVFTests() : juce::UnitTest ("ResonantSVF") {}

    void runTest() override
    {
        beginTest ("stable & finite at max resonance, all modes");

        const ResonantSVF::Mode modes[] = { ResonantSVF::Mode::bell,
                                             ResonantSVF::Mode::lowpass,
                                             ResonantSVF::Mode::highpass };

        for (auto mode : modes)
        {
            ResonantSVF svf;
            juce::dsp::ProcessSpec spec { 48000.0, 512, 1 };
            svf.prepare (spec);
            svf.setMode (mode);
            svf.setFrequency (1000.0f);
            svf.setResonance (1.0f);
            svf.setGainDb (12.0f);

            juce::AudioBuffer<float> buffer (1, 512);
            juce::Random rng (1234);
            bool ok = true;

            for (int blk = 0; blk < 400; ++blk)
            {
                for (int n = 0; n < 512; ++n)
                    buffer.setSample (0, n, rng.nextFloat() * 2.0f - 1.0f);

                juce::dsp::AudioBlock<float> block (buffer);
                svf.process (block);

                for (int n = 0; n < 512; ++n)
                {
                    const float s = buffer.getSample (0, n);
                    if (! std::isfinite (s) || std::abs (s) > 100.0f)
                        ok = false;
                }
            }
            expect (ok);
        }

        beginTest ("magnitudeAt peaks near cutoff for low-pass");
        {
            ResonantSVF svf;
            juce::dsp::ProcessSpec spec { 48000.0, 512, 1 };
            svf.prepare (spec);
            svf.setMode (ResonantSVF::Mode::lowpass);
            svf.setFrequency (1000.0f);
            svf.setResonance (0.9f);
            juce::AudioBuffer<float> silence (1, 512);
            silence.clear();
            for (int i = 0; i < 8; ++i) { juce::dsp::AudioBlock<float> b (silence); svf.process (b); }

            const float atCutoff = svf.magnitudeAt (1000.0f);
            const float wayAbove = svf.magnitudeAt (8000.0f);
            expect (atCutoff > wayAbove);
        }
    }
};

static ResonantSVFTests resonantSVFTests;

int main()
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    return failures > 0 ? 1 : 0;
}
