#include <juce_core/juce_core.h>
#include "../Source/DSP/NoteFrequency.h"

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
    }
};

static NoteFrequencyTests noteFrequencyTests;

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
