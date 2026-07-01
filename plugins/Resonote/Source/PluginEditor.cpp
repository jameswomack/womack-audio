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

    setResizable (true, true);
    setResizeLimits (600, 380, 1600, 1100);
    setSize (760, 480);
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
