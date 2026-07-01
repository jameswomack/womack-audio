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
