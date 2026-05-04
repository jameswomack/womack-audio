#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>
#include "UI/MainPanel.h"
#include "PluginProcessor.h"

class WomackFXEditor : public juce::AudioProcessorEditor
{
public:
    explicit WomackFXEditor (WomackFXProcessor& p);
    ~WomackFXEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    WomackFXProcessor& processor;
    juce::OpenGLContext glContext;
    MainPanel mainPanel;
    juce::TooltipWindow tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WomackFXEditor)
};
