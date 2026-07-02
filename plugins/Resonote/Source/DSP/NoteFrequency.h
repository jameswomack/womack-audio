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

    enum class Scale { chromatic = 0, major = 1, minor = 2 };

    /** Pitch-class membership test for a scale rooted at rootPc (0=C..11=B). */
    inline bool inScale (int midiNote, int rootPc, Scale s) noexcept
    {
        if (s == Scale::chromatic)
            return true;

        static const int majorSet[7] = { 0, 2, 4, 5, 7, 9, 11 };
        static const int minorSet[7] = { 0, 2, 3, 5, 7, 8, 10 }; // natural minor
        const int* set = (s == Scale::major) ? majorSet : minorSet;

        const int pc = (((midiNote - rootPc) % 12) + 12) % 12;
        for (int i = 0; i < 7; ++i)
            if (set[i] == pc)
                return true;
        return false;
    }

    /** Snaps hz to the nearest note within the given scale (rooted at rootPc).
        Chromatic delegates to snapToNote. */
    inline double snapToScale (double hz, int rootPc, Scale s) noexcept
    {
        if (s == Scale::chromatic)
            return snapToNote (hz);

        const double m = hzToMidi (hz);
        const int center = (int) std::lround (m);
        int    best = center;
        double bestDist = 1.0e9;
        for (int cand = center - 7; cand <= center + 7; ++cand)
        {
            if (! inScale (cand, rootPc, s))
                continue;
            const double d = std::abs ((double) cand - m);
            if (d < bestDist) { bestDist = d; best = cand; }
        }
        return midiToHz (best);
    }
}
