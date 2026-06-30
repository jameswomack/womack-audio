# Womack Resonote — Design

**Date:** 2026-06-29
**Status:** Approved (pending implementation plan)

## Concept

A single-band, note-aware resonant filter for the Womack Audio suite. The filter
center frequency is continuous but always reports the nearest musical note (name +
cents), with an optional snap-to-note mode. Three selectable filter shapes share one
warm, nonlinear DSP core. There are no internal modulators — the cutoff is ridden by
hand, host automation, or MIDI notes. Warmth is produced entirely by the filter's own
nonlinear character (no separate drive/saturation knob).

## Decisions (locked)

- **DSP core:** Option B — one unified zero-delay TPT state-variable filter (SVF) with
  a `tanh` nonlinearity in the resonance feedback path. All three modes derive from this
  single core so the warm self-character is consistent across them, and frequency changes
  are zipper-free.
- **MIDI tracking:** when `midiTrack` is on, incoming MIDI notes set the cutoff
  (last-note priority) and the frequency knob greys out (MIDI replaces the knob — no
  semitone-offset behavior in v1).
- **Snap:** chromatic-only (12-TET, A4 = 440 Hz) for v1. No root/scale picker in v1.
- **Warmth source:** filter self-character only — no separate drive knob, no parallel mix.

## Architecture & File Layout

New self-contained plugin alongside `WomackFX`:

```
plugins/Resonote/
  CMakeLists.txt            # juce_add_plugin, PLUGIN_CODE Rnt1, NEEDS_MIDI_INPUT TRUE
  Source/
    PluginProcessor.{h,cpp} # APVTS, MIDI note handling, DSP graph
    PluginEditor.{h,cpp}
    Parameters.h            # APVTS layout (matches suite pattern)
    DSP/
      NoteFrequency.h       # header-only: note<->Hz, nearest-note, cents, snap (pure, unit-tested)
      ResonantSVF.{h,cpp}   # unified TPT state-variable core with nonlinear feedback
    UI/
      ResonotePanel.{h,cpp} # the single control panel
      ResponseCurve.{h,cpp} # OpenGL response viz with note gridlines
  Tests/NoteFrequencyTests.cpp
```

- AU codes: manufacturer `Wmck` (suite-consistent), plugin `Rnt1`, type `aumf` (Music Effect — JUCE selects `kAudioUnitType_MusicEffect` because `NEEDS_MIDI_INPUT TRUE`).
- Formats: AU / VST3 / Standalone, auto-copy to `~/Library/Audio/Plug-Ins/` on build,
  matching `WomackFX`.
- Registered via `plugins/CMakeLists.txt` (`add_subdirectory(Resonote)`).

## Parameters (APVTS)

| ID         | Type   | Range / values                | Notes |
|------------|--------|-------------------------------|-------|
| `mode`     | choice | Bell / Low-Pass / High-Pass   | selects filter shape |
| `frequency`| float  | 20 Hz – 20 kHz, log skew      | default 220 Hz (A3); displayed as note + cents |
| `snap`     | bool   | off / on                      | snaps frequency to nearest chromatic note |
| `resonance`| float  | 0 – 1                         | maps to Q / feedback; also drives warmth (more resonance → more harmonic color) |
| `gain`     | float  | −24 … +24 dB                  | bell mode only; greyed out otherwise |
| `midiTrack`| bool   | off / on                      | MIDI notes set cutoff (last-note priority); frequency knob greys out |
| `output`   | float  | −24 … +24 dB                  | output trim |
| bypass     | —      | host-provided                 | standard JUCE bypass |

## DSP Core (ResonantSVF)

- One zero-delay TPT state-variable filter instance per channel.
- LP / BP / HP outputs computed each sample; modes derived as:
  - **Low-Pass** = LP output
  - **High-Pass** = HP output
  - **Bell** = input + (gainLinear − 1) · BP output
- A `tanh` shaper in the resonance feedback path is the sole warmth source: near-clean at
  low resonance; adds harmonics and lets the low-pass sing / approach self-oscillation when
  pushed.
- `SmoothedValue` on frequency / resonance / gain so manual rides, automation, and MIDI
  note jumps are click-free.
- Stability clamp at high resonance; NaN guard on output.

## UI

- Single panel (~720×400), consistent with suite aesthetic (OpenGL context, animated viz).
- Center: large **frequency dial** with a bold note-name + cents readout beneath it.
- Surrounding controls: 3-way mode selector, resonance, gain (bell), output; snap and
  MIDI-track toggles.
- Signature visual: an **OpenGL response curve** over a log-frequency axis with faint
  vertical **note gridlines**, the live filter shape drawn on top, and a marker at the
  current cutoff labeled with its locked note — so the band is visibly sitting on a pitch.

## Testing

- **NoteFrequency** unit tests (pure math: note↔Hz, nearest-note, cents, snap edge cases)
  via JUCE `UnitTest`, run from a small `Tests` target.
- DSP smoke checks: impulse in → finite out; stable at max resonance; no NaN.
- `auval -v aumf Rnt1 Wmck` + pluginval, matching the suite's CI.

## Out of Scope (v1 / future)

- Internal LFO / envelope follower / step sequencer.
- Root + scale quantization (in-key snapping).
- Separate drive/saturation stage or parallel dry/wet mix.
- Adjustable tuning reference (fixed A4 = 440 Hz in v1).
- MIDI knob-as-offset behavior.
