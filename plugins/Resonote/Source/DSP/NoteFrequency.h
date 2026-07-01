#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

/** Pure note <-> frequency helpers. 12-TET, A4 = 440 Hz, MIDI 69 = A4.
    Scientific pitch notation: MIDI 60 = C4 (middle C). */
namespace NoteFrequency
{
    inline constexpr double a4Hz   = 440.0;
    inline constexpr int    a4Midi = 69;

    inline double midiToHz (int midiNote) noexcept
    {
        return a4Hz * std::pow (2.0, (midiNote - a4Midi) / 12.0);
    }

    /** Continuous MIDI note number for a frequency (may be fractional). hz must be > 0. */
    inline double hzToMidi (double hz) noexcept
    {
        jassert (hz > 0.0);
        return a4Midi + 12.0 * std::log2 (hz / a4Hz);
    }

    inline int nearestMidi (double hz) noexcept
    {
        return (int) std::lround (hzToMidi (hz));
    }

    inline juce::String noteName (int midiNote)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F",
                                       "F#", "G", "G#", "A", "A#", "B" };
        const int pitchClass = ((midiNote % 12) + 12) % 12;
        const int octave     = midiNote / 12 - 1;          // MIDI 60 -> C4
        return juce::String (names[pitchClass]) + juce::String (octave);
    }

    /** Deviation in cents [-50, +50) of hz from its nearest note (same rounding
        as nearestMidi: half rounds away from zero / up to the higher note). */
    inline double centsFromNearest (double hz) noexcept
    {
        const double m = hzToMidi (hz);
        return (m - std::round (m)) * 100.0;
    }

    /** Snaps a frequency to the nearest chromatic note frequency. */
    inline double snapToNote (double hz) noexcept
    {
        return midiToHz (nearestMidi (hz));
    }
}
