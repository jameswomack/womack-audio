#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Visualizer.h"
#include "../Effects/FuzzEffect.h"

class FuzzPanel : public juce::Component
{
public:
    FuzzPanel (juce::AudioProcessorValueTreeState& apvts, FuzzEffect& fuzz);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class TransferCurveViz : public Visualizer
    {
    public:
        TransferCurveViz (FuzzEffect& f) : fuzzRef (f) {}
        void paint (juce::Graphics& g) override;
    private:
        FuzzEffect& fuzzRef;
    };

    FuzzEffect& fuzzEffect;

    juce::Slider driveKnob, toneKnob, mixKnob;
	    juce::Label driveLabel { {}, "Drive" }, toneLabel { {}, "Tone" }, mixLabel { {}, "Mix" };
    juce::ComboBox typeSelector;
	    juce::Label typeLabel { {}, "Type" }, curveLabel { {}, "Curve" };
    juce::ToggleButton enableBtn { "FUZZ" };
    TransferCurveViz curveViz;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> driveAtt, toneAtt, mixAtt;
    std::unique_ptr<ButtonAttachment> enableAtt;
    std::unique_ptr<ComboBoxAttachment> typeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuzzPanel)
};
