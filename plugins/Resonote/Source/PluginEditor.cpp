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
