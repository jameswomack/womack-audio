#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Visualizer.h"
#include "../Effects/TapeDelayEffect.h"

class TapeDelayPanel : public juce::Component
{
public:
    TapeDelayPanel (juce::AudioProcessorValueTreeState& apvts, TapeDelayEffect& delay);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class TapeReelViz : public Visualizer
    {
    public:
        TapeReelViz (TapeDelayEffect& d) : delayRef (d) {}
        void paint (juce::Graphics& g) override;
    private:
        TapeDelayEffect& delayRef;
    };

    TapeDelayEffect& delayEffect;

    juce::Slider timeKnob, feedbackKnob, modKnob, satKnob, mixKnob;
	    juce::Label timeLabel { {}, "Time" }, feedbackLabel { {}, "Feedback" }, modLabel { {}, "Mod" }, satLabel { {}, "Sat" }, mixLabel { {}, "Mix" };
	    juce::Label reelLabel { {}, "Reels" };
    juce::ToggleButton enableBtn { "DELAY" };
    TapeReelViz reelViz;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> timeAtt, feedbackAtt, modAtt, satAtt, mixAtt;
    std::unique_ptr<ButtonAttachment> enableAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeDelayPanel)
};
