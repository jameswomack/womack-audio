# Womack Resonote — Scale Snapping — Design

**Date:** 2026-06-30
**Status:** Approved

## Concept

Extend the existing SNAP behavior so the frequency can snap to notes within a
selected musical key (root + scale), not just the nearest chromatic note. v1
supports Chromatic (current behavior), Major, and Natural Minor. In-key notes are
highlighted on the response curve.

## Decisions (locked)

- **Scales v1:** Chromatic, Major, Natural Minor. (More can be added later by
  extending one enum + interval table.)
- **UI:** keep the existing SNAP checkbox as the master on/off; add a Root
  dropdown (C–B) and a Scale dropdown. Root greys out when Scale = Chromatic.
- **Viz:** when SNAP is on and Scale ≠ Chromatic, in-key gridlines draw brighter
  than out-of-key ones.
- **MIDI:** unchanged — MIDI-set pitch stays exact; snapping/scale only affects
  the frequency-knob path.
- **Backward compatibility:** new params default to Root=C, Scale=Chromatic, so
  behavior and existing presets are unchanged.

## Note math (`NoteFrequency.h`)

- `enum class Scale { chromatic = 0, major = 1, minor = 2 };`
- `bool inScale (int midiNote, int rootPc, Scale s)` — pitch-class membership.
  Major = `{0,2,4,5,7,9,11}`, Natural Minor = `{0,2,3,5,7,8,10}`, Chromatic = true.
- `double snapToScale (double hz, int rootPc, Scale s)` — nearest in-key note.
  Computes the continuous MIDI value, searches candidates ±7 semitones, returns
  `midiToHz` of the nearest one whose pitch class is in the scale. Chromatic
  delegates to the existing `snapToNote`.

These are pure and unit-tested.

## Parameters (APVTS)

| ID      | Type   | Values                          | Default   |
|---------|--------|---------------------------------|-----------|
| `root`  | choice | C, C#, D, D#, E, F, F#, G, G#, A, A#, B | C (index 0) |
| `scale` | choice | Chromatic, Major, Minor         | Chromatic (index 0) |

## Processor

- Cache `rootParam`, `scaleParam` pointers.
- In `processBlock`, when SNAP is on and MIDI-track is off:
  `freq = snapToScale(freqKnob, (int)root, (Scale)scale)`.
  (When scale = Chromatic this equals the current `snapToNote`.)
- Add message-thread accessors for the viz:
  `int getScaleRoot()`, `int getScaleType()`, `bool isSnapEnabled()`.

## UI (`ResonotePanel`)

- Top row: `Mode | Root | Scale` grouped on the left, `SNAP | MIDI` on the right;
  each dropdown has a bold centered label and an APVTS `ComboBoxAttachment`.
- `rootSelector` greys out (disabled) when Scale = Chromatic (handled in the
  existing 30 Hz `timerCallback`, guarded so `setEnabled` only fires on change).

## Viz (`ResponseCurve`)

- Read `proc.isSnapEnabled()`, `proc.getScaleRoot()`, `proc.getScaleType()`.
- When snapping is active and Scale ≠ Chromatic, draw in-key gridlines at the
  brighter alpha (like the C lines today) and out-of-key lines faint; otherwise
  keep current behavior. Still under the fisheye/bow lens; still labels Cs.

## Testing

- Unit tests for `inScale` (membership for C major / A minor) and `snapToScale`
  (e.g. in C Major a frequency between C#5 and D5 snaps to D5; F4 stays F4;
  Chromatic path matches `snapToNote`).
- Rebuild, run `ResonoteTests` (exit 0), `auval -v aumf Rnt1 Wmck`.

## Out of scope (v1)

- Pentatonics, modes, harmonic/melodic minor (future — extend the interval table).
- Snapping applied to incoming MIDI notes.
- Persisting UI window size.
