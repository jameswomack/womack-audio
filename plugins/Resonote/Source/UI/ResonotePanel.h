#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"
#include "../Parameters.h"
#include "ResponseCurve.h"

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

    std::atomic<float>* modeParam      = nullptr;
    std::atomic<float>* midiTrackParam = nullptr;

    juce::ComboBox modeSelector;
    juce::ComboBox rootSelector;
    juce::ComboBox scaleSelector;
    juce::Slider   freqKnob, resKnob, gainKnob, outKnob;
    juce::Label    freqLabel  { {}, "Frequency" },
                   resLabel   { {}, "Resonance" },
                   gainLabel  { {}, "Gain" },
                   outLabel   { {}, "Output" },
                   modeLabel  { {}, "Mode" },
                   rootLabel  { {}, "Root" },
                   scaleLabel { {}, "Scale" };
    juce::Label    noteReadout;   // big "A4  +3 c" display
    ResponseCurve  responseCurve;
    juce::ToggleButton snapBtn { "SNAP" };
    juce::ToggleButton midiBtn { "MIDI" };

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment>   freqAtt, resAtt, gainAtt, outAtt;
    std::unique_ptr<ButtonAttachment>   snapAtt, midiAtt;
    std::unique_ptr<ComboBoxAttachment> modeAtt, rootAtt, scaleAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonotePanel)
};
