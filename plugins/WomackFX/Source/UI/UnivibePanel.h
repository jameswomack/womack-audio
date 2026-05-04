#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Visualizer.h"
#include "../Effects/UnivibeEffect.h"

class UnivibePanel : public juce::Component
{
public:
    UnivibePanel (juce::AudioProcessorValueTreeState& apvts, UnivibeEffect& vibe);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class RotorViz : public Visualizer
    {
    public:
        RotorViz (UnivibeEffect& v) : vibeRef (v) {}
        void paint (juce::Graphics& g) override;
    private:
        UnivibeEffect& vibeRef;
    };

    UnivibeEffect& vibeEffect;

    juce::Slider rateKnob, depthKnob, feedbackKnob, mixKnob;
	    juce::Label rateLabel { {}, "Rate" }, depthLabel { {}, "Depth" }, feedbackLabel { {}, "Feedback" }, mixLabel { {}, "Mix" };
	    juce::Label rotorLabel { {}, "Rotor" };
    juce::ToggleButton enableBtn { "VIBE" };
    RotorViz rotorViz;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> rateAtt, depthAtt, feedbackAtt, mixAtt;
    std::unique_ptr<ButtonAttachment> enableAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnivibePanel)
};
