#include "MainPanel.h"

MainPanel::MainPanel (juce::AudioProcessorValueTreeState& apvts,
                      FuzzEffect& fuzz, UnivibeEffect& vibe, TapeDelayEffect& delay)
    : fuzzPanel (apvts, fuzz),
      vibePanel (apvts, vibe),
      delayPanel (apvts, delay)
{
    addAndMakeVisible (fuzzPanel);
    addAndMakeVisible (vibePanel);
    addAndMakeVisible (delayPanel);
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111111));

    // Title
    g.setColour (juce::Colour (0xffcc8833));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("WOMACK FX", getLocalBounds().removeFromTop (36), juce::Justification::centred);
}

void MainPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);
    bounds.removeFromTop (32); // Title space

    int panelW = bounds.getWidth() / 3;
    int gap = 6;

    fuzzPanel.setBounds  (bounds.removeFromLeft (panelW).reduced (gap, 0));
    delayPanel.setBounds (bounds.removeFromRight (panelW).reduced (gap, 0));
    vibePanel.setBounds  (bounds.reduced (gap, 0));
}
