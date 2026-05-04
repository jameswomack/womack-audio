#include "PluginEditor.h"

WomackFXEditor::WomackFXEditor (WomackFXProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      mainPanel (p.getAPVTS(), p.getFuzz(), p.getUnivibe(), p.getTapeDelay()),
      tooltipWindow (this, 500)
{
    glContext.attachTo (*this);
    addAndMakeVisible (mainPanel);
    tooltipWindow.toFront (false);
    setSize (900, 420);
}

WomackFXEditor::~WomackFXEditor()
{
    glContext.detach();
}

void WomackFXEditor::paint (juce::Graphics&)
{
}

void WomackFXEditor::resized()
{
    mainPanel.setBounds (getLocalBounds());
}
