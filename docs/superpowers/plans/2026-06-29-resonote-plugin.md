# Womack Resonote Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a new JUCE plugin, "Womack Resonote" — a single-band note-aware resonant filter (bell / low-pass / high-pass) with a warm nonlinear core, continuous frequency with a live note-name readout + optional snap-to-note, ridden by hand / host automation / MIDI notes.

**Architecture:** A new self-contained plugin under `plugins/Resonote/`, mirroring the WomackFX structure (APVTS hub processor, attachment-driven UI panel, Timer-repainted visualizer over an OpenGL context). DSP is one unified zero-delay TPT state-variable filter (`ResonantSVF`) with a `tanh` nonlinearity in the resonance feedback path; all three modes derive from it. Note math lives in a pure, unit-tested header (`NoteFrequency.h`).

**Tech Stack:** JUCE 8.0.12, C++20, CMake (Xcode generator), `juce::dsp`, `juce::AudioProcessorValueTreeState`, JUCE `UnitTest` console-app test target.

**Reference spec:** `docs/superpowers/specs/2026-06-29-resonote-plugin-design.md`

**Conventions matched from WomackFX:**
- `ParamIDs` namespace of `inline constexpr const char*` IDs in `Parameters.h`.
- DSP units take `juce::AudioProcessorValueTreeState&` and cache `std::atomic<float>*` via `getRawParameterValue`. (Resonote's processor reads params directly and pushes them into the SVF, since there is a single DSP unit.)
- UI panels use `SliderAttachment` / `ComboBoxAttachment` / `ButtonAttachment`.
- Visualizers extend a `Visualizer` base (Component + SettableTooltipClient + Timer at 30 Hz, `timerCallback` → `repaint`).
- Editor attaches a `juce::OpenGLContext` and a `juce::TooltipWindow`.
- Compile defs: `JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0`.

**Build/verify commands used throughout:**
```bash
# from repo root of the worktree
cmake -B build -G Xcode
cmake --build build --config Release --target Resonote_All
cmake --build build --config Release --target ResonoteTests
```

---

## Task 1: NoteFrequency utility + test harness (TDD)

**Files:**
- Create: `plugins/Resonote/Source/DSP/NoteFrequency.h`
- Create: `plugins/Resonote/Tests/NoteFrequencyTests.cpp`
- Create: `plugins/Resonote/CMakeLists.txt`
- Modify: `plugins/CMakeLists.txt`

- [ ] **Step 1: Register the new plugin directory**

Edit `plugins/CMakeLists.txt` to:
```cmake
add_subdirectory(WomackFX)
add_subdirectory(Resonote)
```

- [ ] **Step 2: Write the failing test**

Create `plugins/Resonote/Tests/NoteFrequencyTests.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include <cmath>
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
        // a quarter-tone (50 cents) above A4:
        const double quarterTone = 440.0 * std::pow (2.0, 0.5 / 12.0);
        expect (NoteFrequency::centsFromNearest (quarterTone) > 49.0);

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
```

- [ ] **Step 3: Create the test CMake target**

Create `plugins/Resonote/CMakeLists.txt`:
```cmake
# --- Unit test console app -------------------------------------------------
juce_add_console_app(ResonoteTests
    PRODUCT_NAME "ResonoteTests")

target_sources(ResonoteTests
    PRIVATE
        Tests/NoteFrequencyTests.cpp)

target_compile_definitions(ResonoteTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(ResonoteTests
    PRIVATE
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
```
(`juce::juce_dsp` transitively brings `juce_core` / `juce_audio_basics`, needed in later tasks; linking it now keeps the CMake stable.)

- [ ] **Step 4: Run the test to verify it fails to compile (header missing)**

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target ResonoteTests
```
Expected: FAIL — `'../Source/DSP/NoteFrequency.h' file not found`.

- [ ] **Step 5: Write the implementation**

Create `plugins/Resonote/Source/DSP/NoteFrequency.h`:
```cpp
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

    /** Continuous MIDI note number for a frequency (may be fractional). */
    inline double hzToMidi (double hz) noexcept
    {
        return a4Midi + 12.0 * std::log2 (hz / a4Hz);
    }

    inline int nearestMidi (double hz) noexcept
    {
        return (int) std::lround (hzToMidi (hz));
    }

    inline juce::String noteName (int midiNote) noexcept
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F",
                                       "F#", "G", "G#", "A", "A#", "B" };
        const int pitchClass = ((midiNote % 12) + 12) % 12;
        const int octave     = midiNote / 12 - 1;          // MIDI 60 -> C4
        return juce::String (names[pitchClass]) + juce::String (octave);
    }

    /** Deviation in cents (-50..+50) of hz from its nearest note. */
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
```

- [ ] **Step 6: Run the tests to verify they pass**

```bash
cmake --build build --config Release --target ResonoteTests
./build/plugins/Resonote/ResonoteTests_artefacts/Release/ResonoteTests; echo "exit=$?"
```
Expected: test output ending with "All tests completed successfully" and `exit=0`.

- [ ] **Step 7: Commit**

```bash
git add plugins/CMakeLists.txt plugins/Resonote/CMakeLists.txt \
        plugins/Resonote/Source/DSP/NoteFrequency.h \
        plugins/Resonote/Tests/NoteFrequencyTests.cpp
git commit -m "feat(resonote): note<->frequency utility with unit tests"
```

---

## Task 2: ResonantSVF DSP core + stability test

**Files:**
- Create: `plugins/Resonote/Source/DSP/ResonantSVF.h`
- Create: `plugins/Resonote/Source/DSP/ResonantSVF.cpp`
- Modify: `plugins/Resonote/Tests/NoteFrequencyTests.cpp` (add a DSP stability test)
- Modify: `plugins/Resonote/CMakeLists.txt` (add `ResonantSVF.cpp` to test sources)

- [ ] **Step 1: Write the failing stability test**

In `plugins/Resonote/Tests/NoteFrequencyTests.cpp`, add the include near the top (after the existing includes):
```cpp
#include "../Source/DSP/ResonantSVF.h"
```
Then add a new test class above `int main()`:
```cpp
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
            // Pump the smoothed values to target by processing silence.
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
```

- [ ] **Step 2: Add ResonantSVF.cpp to the test target**

In `plugins/Resonote/CMakeLists.txt`, change the `ResonoteTests` sources block to:
```cmake
target_sources(ResonoteTests
    PRIVATE
        Tests/NoteFrequencyTests.cpp
        Source/DSP/ResonantSVF.cpp)
```

- [ ] **Step 3: Run the test to verify it fails to compile**

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target ResonoteTests
```
Expected: FAIL — `'../Source/DSP/ResonantSVF.h' file not found`.

- [ ] **Step 4: Write the DSP header**

Create `plugins/Resonote/Source/DSP/ResonantSVF.h`:
```cpp
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
```

- [ ] **Step 5: Write the DSP implementation**

Create `plugins/Resonote/Source/DSP/ResonantSVF.cpp`:
```cpp
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
    const auto numCh      = juce::jmin ((size_t) chState.size(), block.getNumChannels());
    const auto numSamples = block.getNumSamples();

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float fc = freqSmoothed.getNextValue();
        const float r  = resSmoothed.getNextValue();
        const float db = gainSmoothed.getNextValue();

        const float Q = resonanceToQ (r);
        const float k = 1.0f / Q;
        const float A = std::pow (10.0f, db / 40.0f);

        float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sampleRate);
        g = juce::jlimit (0.0f, 100.0f, g);

        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;

        const float satDrive = 1.0f + r * 3.0f;   // warmth scales with resonance

        for (size_t ch = 0; ch < numCh; ++ch)
        {
            auto& st    = chState[ch];
            float* data = block.getChannelPointer (ch);
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
                default:             out = v0 + (A * A - 1.0f) * k * band; break;
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
```

- [ ] **Step 6: Run the tests to verify they pass**

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target ResonoteTests
./build/plugins/Resonote/ResonoteTests_artefacts/Release/ResonoteTests; echo "exit=$?"
```
Expected: all tests pass, `exit=0`.

- [ ] **Step 7: Commit**

```bash
git add plugins/Resonote/Source/DSP/ResonantSVF.h \
        plugins/Resonote/Source/DSP/ResonantSVF.cpp \
        plugins/Resonote/Tests/NoteFrequencyTests.cpp \
        plugins/Resonote/CMakeLists.txt
git commit -m "feat(resonote): nonlinear TPT state-variable filter core"
```

---

## Task 3: Parameters + PluginProcessor (audio + MIDI) with early auval checkpoint

**Files:**
- Create: `plugins/Resonote/Source/Parameters.h`
- Create: `plugins/Resonote/Source/PluginProcessor.h`
- Create: `plugins/Resonote/Source/PluginProcessor.cpp`
- Modify: `plugins/Resonote/CMakeLists.txt` (add the `juce_add_plugin` target)

This task uses a temporary `juce::GenericAudioProcessorEditor` so the plugin builds and passes `auval` before the custom UI exists. Task 4 replaces it.

- [ ] **Step 1: Write Parameters.h**

Create `plugins/Resonote/Source/Parameters.h`:
```cpp
#pragma once

namespace ParamIDs
{
    inline constexpr const char* mode      = "mode";
    inline constexpr const char* frequency = "frequency";
    inline constexpr const char* snap      = "snap";
    inline constexpr const char* resonance = "resonance";
    inline constexpr const char* gain      = "gain";
    inline constexpr const char* midiTrack = "midiTrack";
    inline constexpr const char* output    = "output";
}
```

- [ ] **Step 2: Write PluginProcessor.h**

Create `plugins/Resonote/Source/PluginProcessor.h`:
```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "DSP/ResonantSVF.h"

class ResonoteProcessor : public juce::AudioProcessor
{
public:
    ResonoteProcessor();
    ~ResonoteProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Womack Resonote"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    /** Most-recently-applied filter frequency (post snap / MIDI). UI readout. */
    float getCurrentFrequencyHz() const noexcept { return currentFreqHz.load(); }

    /** Linear magnitude of the live filter at hz. UI response curve. */
    float getMagnitudeAt (float hz) const noexcept { return svf.magnitudeAt (hz); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    ResonantSVF svf;

    std::atomic<float>* modeParam      = nullptr;
    std::atomic<float>* freqParam      = nullptr;
    std::atomic<float>* snapParam      = nullptr;
    std::atomic<float>* resParam       = nullptr;
    std::atomic<float>* gainParam      = nullptr;
    std::atomic<float>* midiTrackParam = nullptr;
    std::atomic<float>* outputParam    = nullptr;

    int lastMidiNote = -1;
    std::atomic<float> currentFreqHz { 220.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonoteProcessor)
};
```

- [ ] **Step 3: Write PluginProcessor.cpp**

Create `plugins/Resonote/Source/PluginProcessor.cpp`:
```cpp
#include "PluginProcessor.h"
#include "Parameters.h"
#include "DSP/NoteFrequency.h"

ResonoteProcessor::ResonoteProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    modeParam      = apvts.getRawParameterValue (ParamIDs::mode);
    freqParam      = apvts.getRawParameterValue (ParamIDs::frequency);
    snapParam      = apvts.getRawParameterValue (ParamIDs::snap);
    resParam       = apvts.getRawParameterValue (ParamIDs::resonance);
    gainParam      = apvts.getRawParameterValue (ParamIDs::gain);
    midiTrackParam = apvts.getRawParameterValue (ParamIDs::midiTrack);
    outputParam    = apvts.getRawParameterValue (ParamIDs::output);
}

juce::AudioProcessorValueTreeState::ParameterLayout ResonoteProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::mode, 1 }, "Mode",
        juce::StringArray { "Bell", "Low-Pass", "High-Pass" }, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::frequency, 1 }, "Frequency",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.01f, 0.25f), 220.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::snap, 1 }, "Snap", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::resonance, 1 }, "Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::gain, 1 }, "Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::midiTrack, 1 }, "MIDI Track", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::output, 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));

    return layout;
}

void ResonoteProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    svf.prepare (spec);
    lastMidiNote = -1;
}

bool ResonoteProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == mainOut;
}

void ResonoteProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const bool midiTrack = midiTrackParam->load() > 0.5f;

    if (midiTrack)
    {
        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
                lastMidiNote = msg.getNoteNumber();
        }
    }

    float freq;
    if (midiTrack && lastMidiNote >= 0)
    {
        freq = (float) NoteFrequency::midiToHz (lastMidiNote);
    }
    else
    {
        freq = freqParam->load();
        if (snapParam->load() > 0.5f)
            freq = (float) NoteFrequency::snapToNote (freq);
    }

    currentFreqHz.store (freq);

    svf.setMode (static_cast<ResonantSVF::Mode> ((int) modeParam->load()));
    svf.setFrequency (freq);
    svf.setResonance (resParam->load());
    svf.setGainDb (gainParam->load());

    juce::dsp::AudioBlock<float> block (buffer);
    svf.process (block);

    block.multiplyBy (juce::Decibels::decibelsToGain (outputParam->load()));
}

void ResonoteProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ResonoteProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* ResonoteProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ResonoteProcessor();
}
```

- [ ] **Step 4: Add the plugin target to CMake**

Replace the entire contents of `plugins/Resonote/CMakeLists.txt` with:
```cmake
# --- Resonote plugin -------------------------------------------------------
juce_add_plugin(Resonote
    COMPANY_NAME "Womack"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Wmck
    PLUGIN_CODE Rnt1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Womack Resonote")

target_sources(Resonote
    PRIVATE
        Source/PluginProcessor.cpp
        Source/DSP/ResonantSVF.cpp)

target_compile_definitions(Resonote
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Resonote
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Unit test console app -------------------------------------------------
juce_add_console_app(ResonoteTests
    PRODUCT_NAME "ResonoteTests")

target_sources(ResonoteTests
    PRIVATE
        Tests/NoteFrequencyTests.cpp
        Source/DSP/ResonantSVF.cpp)

target_compile_definitions(ResonoteTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(ResonoteTests
    PRIVATE
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
```

- [ ] **Step 5: Build the plugin and run auval**

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target Resonote_All
killall -9 AudioComponentRegistrar 2>/dev/null
auval -v aumf Rnt1 Wmck  # aumf = Music Effect; JUCE selects kAudioUnitType_MusicEffect because NEEDS_MIDI_INPUT TRUE
```
Expected: build succeeds; `auval` ends with "AU VALIDATION SUCCEEDED".

- [ ] **Step 6: Re-run unit tests (ensure nothing regressed)**

```bash
cmake --build build --config Release --target ResonoteTests
./build/plugins/Resonote/ResonoteTests_artefacts/Release/ResonoteTests; echo "exit=$?"
```
Expected: `exit=0`.

- [ ] **Step 7: Commit**

```bash
git add plugins/Resonote/Source/Parameters.h \
        plugins/Resonote/Source/PluginProcessor.h \
        plugins/Resonote/Source/PluginProcessor.cpp \
        plugins/Resonote/CMakeLists.txt
git commit -m "feat(resonote): processor with APVTS, MIDI tracking, snap; auval passes"
```

---

## Task 4: Custom editor + ResonotePanel (controls & attachments)

**Files:**
- Create: `plugins/Resonote/Source/UI/Visualizer.h`
- Create: `plugins/Resonote/Source/UI/Visualizer.cpp`
- Create: `plugins/Resonote/Source/UI/ResonotePanel.h`
- Create: `plugins/Resonote/Source/UI/ResonotePanel.cpp`
- Create: `plugins/Resonote/Source/PluginEditor.h`
- Create: `plugins/Resonote/Source/PluginEditor.cpp`
- Modify: `plugins/Resonote/Source/PluginProcessor.cpp` (use the new editor)
- Modify: `plugins/Resonote/CMakeLists.txt` (add UI sources)

The `ResponseCurve` visualizer is added in Task 5; this task lays out controls and a placeholder area for it.

- [ ] **Step 1: Create the Visualizer base class**

Create `plugins/Resonote/Source/UI/Visualizer.h`:
```cpp
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class Visualizer : public juce::Component,
                   public juce::SettableTooltipClient,
                   public juce::Timer
{
public:
    Visualizer() { startTimerHz (30); }
    ~Visualizer() override { stopTimer(); }

    void timerCallback() override { repaint(); }
};
```

Create `plugins/Resonote/Source/UI/Visualizer.cpp`:
```cpp
#include "Visualizer.h"
// Header-only base class; this .cpp exists to satisfy the CMake source list.
```

- [ ] **Step 2: Write ResonotePanel.h**

Create `plugins/Resonote/Source/UI/ResonotePanel.h`:
```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"
#include "../Parameters.h"

class ResonotePanel : public juce::Component,
                      public juce::Timer
{
public:
    explicit ResonotePanel (ResonoteProcessor& processor);
    ~ResonotePanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    ResonoteProcessor& proc;

    juce::ComboBox modeSelector;
    juce::Slider   freqKnob, resKnob, gainKnob, outKnob;
    juce::Label    freqLabel { {}, "Frequency" },
                   resLabel  { {}, "Resonance" },
                   gainLabel { {}, "Gain" },
                   outLabel  { {}, "Output" },
                   modeLabel { {}, "Mode" };
    juce::Label    noteReadout;   // big "A4  +3¢" display
    juce::ToggleButton snapBtn { "SNAP" };
    juce::ToggleButton midiBtn { "MIDI" };

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment>   freqAtt, resAtt, gainAtt, outAtt;
    std::unique_ptr<ButtonAttachment>   snapAtt, midiAtt;
    std::unique_ptr<ComboBoxAttachment> modeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonotePanel)
};
```

- [ ] **Step 3: Write ResonotePanel.cpp**

Create `plugins/Resonote/Source/UI/ResonotePanel.cpp`:
```cpp
#include "ResonotePanel.h"
#include "../DSP/NoteFrequency.h"

ResonotePanel::ResonotePanel (ResonoteProcessor& processor)
    : proc (processor)
{
    auto& apvts = proc.getAPVTS();

    auto setupKnob = [this] (juce::Slider& s, const juce::String& suffix = "")
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        if (suffix.isNotEmpty())
            s.setTextValueSuffix (suffix);
        addAndMakeVisible (s);
    };

    setupKnob (freqKnob, " Hz");
    setupKnob (resKnob);
    setupKnob (gainKnob, " dB");
    setupKnob (outKnob,  " dB");

    auto setupLabel = [this] (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
        label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    };

    setupLabel (freqLabel);
    setupLabel (resLabel);
    setupLabel (gainLabel);
    setupLabel (outLabel);
    setupLabel (modeLabel);

    noteReadout.setJustificationType (juce::Justification::centred);
    noteReadout.setColour (juce::Label::textColourId, juce::Colour (0xff66ddff));
    noteReadout.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    noteReadout.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (noteReadout);

    modeSelector.addItemList ({ "Bell", "Low-Pass", "High-Pass" }, 1);
    addAndMakeVisible (modeSelector);
    addAndMakeVisible (snapBtn);
    addAndMakeVisible (midiBtn);

    freqKnob.setTooltip ("Filter frequency. Snaps to the nearest note when SNAP is on; set by MIDI notes when MIDI is on.");
    resKnob.setTooltip  ("Resonance / Q. Higher values sharpen the band and add warm harmonic character.");
    gainKnob.setTooltip ("Boost or cut at the center frequency (Bell mode only).");
    outKnob.setTooltip  ("Output level trim.");
    modeSelector.setTooltip ("Filter shape: Bell, Low-Pass, or High-Pass.");
    snapBtn.setTooltip ("Snap the frequency to the nearest chromatic note.");
    midiBtn.setTooltip ("Let incoming MIDI notes set the frequency (last note wins).");

    modeAtt = std::make_unique<ComboBoxAttachment> (apvts, ParamIDs::mode, modeSelector);
    freqAtt = std::make_unique<SliderAttachment>   (apvts, ParamIDs::frequency, freqKnob);
    resAtt  = std::make_unique<SliderAttachment>   (apvts, ParamIDs::resonance, resKnob);
    gainAtt = std::make_unique<SliderAttachment>   (apvts, ParamIDs::gain, gainKnob);
    outAtt  = std::make_unique<SliderAttachment>   (apvts, ParamIDs::output, outKnob);
    snapAtt = std::make_unique<ButtonAttachment>   (apvts, ParamIDs::snap, snapBtn);
    midiAtt = std::make_unique<ButtonAttachment>   (apvts, ParamIDs::midiTrack, midiBtn);

    startTimerHz (30);
}

ResonotePanel::~ResonotePanel()
{
    stopTimer();
}

void ResonotePanel::timerCallback()
{
    const float hz = proc.getCurrentFrequencyHz();
    const int   midi = NoteFrequency::nearestMidi (hz);
    const double cents = NoteFrequency::centsFromNearest (hz);

    juce::String txt = NoteFrequency::noteName (midi);
    txt << "   " << (cents >= 0.0 ? "+" : "") << juce::String (cents, 0) << " c";
    noteReadout.setText (txt, juce::dontSendNotification);

    const bool isBell = (int) *proc.getAPVTS().getRawParameterValue (ParamIDs::mode) == 0;
    gainKnob.setEnabled (isBell);
    gainLabel.setEnabled (isBell);

    const bool midiOn = *proc.getAPVTS().getRawParameterValue (ParamIDs::midiTrack) > 0.5f;
    freqKnob.setEnabled (! midiOn);
}

void ResonotePanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111111));

    g.setColour (juce::Colour (0xff66ddff));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("WOMACK RESONOTE", getLocalBounds().removeFromTop (36),
                juce::Justification::centred);
}

void ResonotePanel::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    bounds.removeFromTop (32);    // title

    // Top row: mode selector + toggles
    auto topRow = bounds.removeFromTop (56);
    auto modeArea = topRow.removeFromLeft (200);
    modeLabel.setBounds (modeArea.removeFromTop (16));
    modeSelector.setBounds (modeArea.reduced (0, 4));
    midiBtn.setBounds (topRow.removeFromRight (90).reduced (4));
    snapBtn.setBounds (topRow.removeFromRight (90).reduced (4));

    // Response curve area (filled in Task 5) + note readout overlay region
    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    noteReadout.setBounds (vizArea.removeFromBottom (40));

    // Knob row
    auto knobRow = bounds;
    const int knobW = knobRow.getWidth() / 4;

    auto layoutKnob = [] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& knob)
    {
        label.setBounds (area.removeFromTop (16));
        knob.setBounds (area);
    };

    layoutKnob (knobRow.removeFromLeft (knobW), freqLabel, freqKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), resLabel,  resKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), gainLabel, gainKnob);
    layoutKnob (knobRow,                        outLabel,  outKnob);
}
```

- [ ] **Step 4: Write PluginEditor.h**

Create `plugins/Resonote/Source/PluginEditor.h`:
```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>
#include "PluginProcessor.h"
#include "UI/ResonotePanel.h"

class ResonoteEditor : public juce::AudioProcessorEditor
{
public:
    explicit ResonoteEditor (ResonoteProcessor& p);
    ~ResonoteEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ResonoteProcessor& processor;
    juce::OpenGLContext glContext;
    ResonotePanel mainPanel;
    juce::TooltipWindow tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonoteEditor)
};
```

- [ ] **Step 5: Write PluginEditor.cpp**

Create `plugins/Resonote/Source/PluginEditor.cpp`:
```cpp
#include "PluginEditor.h"

ResonoteEditor::ResonoteEditor (ResonoteProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      mainPanel (p),
      tooltipWindow (this, 500)
{
    glContext.attachTo (*this);
    addAndMakeVisible (mainPanel);
    tooltipWindow.toFront (false);
    setSize (720, 400);
}

ResonoteEditor::~ResonoteEditor()
{
    glContext.detach();
}

void ResonoteEditor::paint (juce::Graphics&) {}

void ResonoteEditor::resized()
{
    mainPanel.setBounds (getLocalBounds());
}
```

- [ ] **Step 6: Switch the processor to the custom editor**

In `plugins/Resonote/Source/PluginProcessor.cpp`, add the include near the top:
```cpp
#include "PluginEditor.h"
```
and replace `createEditor()`:
```cpp
juce::AudioProcessorEditor* ResonoteProcessor::createEditor()
{
    return new ResonoteEditor (*this);
}
```

- [ ] **Step 7: Add UI sources to CMake**

In `plugins/Resonote/CMakeLists.txt`, update the plugin `target_sources(Resonote ...)` block to:
```cmake
target_sources(Resonote
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/ResonantSVF.cpp
        Source/UI/Visualizer.cpp
        Source/UI/ResonotePanel.cpp)
```

- [ ] **Step 8: Build, run, and validate**

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target Resonote_All
killall -9 AudioComponentRegistrar 2>/dev/null
auval -v aumf Rnt1 Wmck  # aumf = Music Effect; JUCE selects kAudioUnitType_MusicEffect because NEEDS_MIDI_INPUT TRUE
```
Expected: build succeeds; `auval` ends with "AU VALIDATION SUCCEEDED".

- [ ] **Step 9: Visually confirm the Standalone**

```bash
open ./build/plugins/Resonote/Resonote_artefacts/Release/Standalone/Womack\ Resonote.app
```
Expected: window opens (720×400) showing the title, mode selector, SNAP/MIDI toggles, four knobs, and a live note readout (e.g. "A3  +0 c") that updates when dragging the Frequency knob. Gain knob greys out unless Mode = Bell; Frequency knob greys out when MIDI is on.

- [ ] **Step 10: Commit**

```bash
git add plugins/Resonote/Source/UI/Visualizer.h \
        plugins/Resonote/Source/UI/Visualizer.cpp \
        plugins/Resonote/Source/UI/ResonotePanel.h \
        plugins/Resonote/Source/UI/ResonotePanel.cpp \
        plugins/Resonote/Source/PluginEditor.h \
        plugins/Resonote/Source/PluginEditor.cpp \
        plugins/Resonote/Source/PluginProcessor.cpp \
        plugins/Resonote/CMakeLists.txt
git commit -m "feat(resonote): custom editor with controls, note readout, and toggles"
```

---

## Task 5: ResponseCurve visualization with note gridlines

**Files:**
- Create: `plugins/Resonote/Source/UI/ResponseCurve.h`
- Create: `plugins/Resonote/Source/UI/ResponseCurve.cpp`
- Modify: `plugins/Resonote/Source/UI/ResonotePanel.h` (add the viz member)
- Modify: `plugins/Resonote/Source/UI/ResonotePanel.cpp` (construct, add, position the viz)
- Modify: `plugins/Resonote/CMakeLists.txt` (add `ResponseCurve.cpp`)

- [ ] **Step 1: Write ResponseCurve.h**

Create `plugins/Resonote/Source/UI/ResponseCurve.h`:
```cpp
#pragma once

#include "Visualizer.h"
#include "../PluginProcessor.h"

/** Log-frequency response curve with faint vertical note gridlines and a
    marker at the current cutoff labelled with its locked note. */
class ResponseCurve : public Visualizer
{
public:
    explicit ResponseCurve (ResonoteProcessor& processor) : proc (processor) {}

    void paint (juce::Graphics& g) override;

private:
    static constexpr float minHz = 20.0f;
    static constexpr float maxHz = 20000.0f;

    static float hzToX (float hz, juce::Rectangle<float> area) noexcept;

    ResonoteProcessor& proc;
};
```

- [ ] **Step 2: Write ResponseCurve.cpp**

Create `plugins/Resonote/Source/UI/ResponseCurve.cpp`:
```cpp
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

    // --- Note gridlines: draw every C, label octaves. ---
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

    const int   midi  = NoteFrequency::nearestMidi (cutoffHz);
    g.setColour (juce::Colour (0xffffaa33));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (NoteFrequency::noteName (midi),
                juce::Rectangle<float> (cx + 3.0f, area.getY() + 2.0f, 40.0f, 14.0f),
                juce::Justification::left);
}
```

- [ ] **Step 3: Add the viz to ResonotePanel.h**

In `plugins/Resonote/Source/UI/ResonotePanel.h`, add the include after the existing includes:
```cpp
#include "ResponseCurve.h"
```
and add a member in the `private:` section (next to the other widgets):
```cpp
    ResponseCurve responseCurve;
```

- [ ] **Step 4: Construct, add, and position the viz in ResonotePanel.cpp**

In `plugins/Resonote/Source/UI/ResonotePanel.cpp`, change the constructor initializer list to construct the viz:
```cpp
ResonotePanel::ResonotePanel (ResonoteProcessor& processor)
    : proc (processor),
      responseCurve (processor)
```
Add it to the component tree (place near the `addAndMakeVisible (noteReadout);` line):
```cpp
    addAndMakeVisible (responseCurve);
    responseCurve.setTooltip ("Filter response over a log-frequency axis with note gridlines; the orange marker is the locked note.");
```
Position it in `resized()` — replace the existing `vizArea` block with:
```cpp
    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    noteReadout.setBounds (vizArea.removeFromBottom (40));
    responseCurve.setBounds (vizArea.reduced (2));
```

- [ ] **Step 5: Add ResponseCurve.cpp to CMake**

In `plugins/Resonote/CMakeLists.txt`, update the plugin `target_sources(Resonote ...)` block to include it:
```cmake
target_sources(Resonote
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/ResonantSVF.cpp
        Source/UI/Visualizer.cpp
        Source/UI/ResonotePanel.cpp
        Source/UI/ResponseCurve.cpp)
```

- [ ] **Step 6: Build and validate**

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target Resonote_All
killall -9 AudioComponentRegistrar 2>/dev/null
auval -v aumf Rnt1 Wmck  # aumf = Music Effect; JUCE selects kAudioUnitType_MusicEffect because NEEDS_MIDI_INPUT TRUE
```
Expected: build succeeds; `auval` ends with "AU VALIDATION SUCCEEDED".

- [ ] **Step 7: Visually confirm the response curve**

```bash
open ./build/plugins/Resonote/Resonote_artefacts/Release/Standalone/Womack\ Resonote.app
```
Expected: the upper area shows a live filter curve with faint vertical note gridlines (Cs labelled), an orange cutoff marker labelled with the current note. Switching Mode changes the curve shape (bell vs low-pass vs high-pass); raising Resonance sharpens the peak; dragging Frequency moves the marker and snaps to note lines when SNAP is on.

- [ ] **Step 8: Commit**

```bash
git add plugins/Resonote/Source/UI/ResponseCurve.h \
        plugins/Resonote/Source/UI/ResponseCurve.cpp \
        plugins/Resonote/Source/UI/ResonotePanel.h \
        plugins/Resonote/Source/UI/ResonotePanel.cpp \
        plugins/Resonote/CMakeLists.txt
git commit -m "feat(resonote): response curve viz with note gridlines and cutoff marker"
```

---

## Task 6: Final verification (auval + pluginval) and docs

**Files:**
- Modify: `README.md` (add Resonote to the plugin list, if a plugin list exists)

- [ ] **Step 1: Clean build of everything**

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```
Expected: WomackFX and Resonote both build with no errors.

- [ ] **Step 2: Run unit tests**

```bash
./build/plugins/Resonote/ResonoteTests_artefacts/Release/ResonoteTests; echo "exit=$?"
```
Expected: `exit=0`.

- [ ] **Step 3: auval**

```bash
killall -9 AudioComponentRegistrar 2>/dev/null
auval -v aumf Rnt1 Wmck  # aumf = Music Effect; JUCE selects kAudioUnitType_MusicEffect because NEEDS_MIDI_INPUT TRUE
```
Expected: "AU VALIDATION SUCCEEDED".

- [ ] **Step 4: pluginval (matching the suite's CI)**

```bash
# Adjust the pluginval path to wherever the repo's CI fetches it; if pluginval
# is not installed locally, skip and note it.
pluginval --strictness-level 8 --validate \
  "$HOME/Library/Audio/Plug-Ins/Components/Womack Resonote.component"; echo "exit=$?"
```
Expected: `exit=0` ("ALL TESTS PASSED"). If pluginval is unavailable locally, note that it runs in CI and proceed.

- [ ] **Step 5: Update README plugin list (only if one exists)**

If `README.md` lists the suite's plugins, add a bullet:
```markdown
- **Womack Resonote** — single-band note-aware resonant filter (bell / low-pass / high-pass) with a warm nonlinear core, snap-to-note, and MIDI pitch tracking.
```
If no such list exists, skip this step.

- [ ] **Step 6: Commit**

```bash
git add README.md
git commit -m "docs: add Womack Resonote to plugin list"
```
(Skip the commit if README was not changed.)

---

## Self-Review Notes

- **Spec coverage:** mode (bell/LP/HP) ✓ Task 3 params + Task 2 DSP; continuous frequency + note readout ✓ Tasks 3/4; snap-to-note (chromatic) ✓ Tasks 1/3; resonance drives Q + warmth ✓ Task 2; gain bell-only with grey-out ✓ Tasks 3/4; MIDI track replaces knob + grey-out ✓ Tasks 3/4; output trim ✓ Task 3; warmth = filter self-character only (tanh feedback, no drive knob/mix) ✓ Task 2; response curve w/ note gridlines ✓ Task 5; NoteFrequency unit tests + DSP smoke tests ✓ Tasks 1/2; auval + pluginval ✓ Task 6; AU codes Wmck/Rnt1 ✓ Task 3.
- **Out-of-scope honored:** no LFO/env/sequencer, no scale picker, no separate drive/mix, fixed A4=440, no MIDI-offset behavior.
- **Type consistency:** `ResonantSVF::Mode { bell=0, lowpass=1, highpass=2 }` matches the `mode` choice order ("Bell","Low-Pass","High-Pass") so `static_cast<Mode>((int)modeParam)` is correct; `getCurrentFrequencyHz` / `getMagnitudeAt` / `getAPVTS` used consistently across processor, panel, and response curve; `NoteFrequency` function names match between header, tests, panel, and viz.
