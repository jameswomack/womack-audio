#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "FuzzPanel.h"
#include "UnivibePanel.h"
#include "TapeDelayPanel.h"
#include "../Effects/FuzzEffect.h"
#include "../Effects/UnivibeEffect.h"
#include "../Effects/TapeDelayEffect.h"

class MainPanel : public juce::Component
{
public:
    MainPanel (juce::AudioProcessorValueTreeState& apvts,
               FuzzEffect& fuzz, UnivibeEffect& vibe, TapeDelayEffect& delay);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    FuzzPanel      fuzzPanel;
    UnivibePanel   vibePanel;
    TapeDelayPanel delayPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
