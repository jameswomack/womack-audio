#include "FuzzPanel.h"
#include "../Parameters.h"

FuzzPanel::FuzzPanel (juce::AudioProcessorValueTreeState& apvts, FuzzEffect& fuzz)
    : fuzzEffect (fuzz), curveViz (fuzz)
{
    auto setupKnob = [] (juce::Slider& s, const juce::String& suffix = "")
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
        if (suffix.isNotEmpty())
            s.setTextValueSuffix (suffix);
    };

    setupKnob (driveKnob);
    setupKnob (toneKnob, " Hz");
    setupKnob (mixKnob);

    addAndMakeVisible (driveKnob);
    addAndMakeVisible (toneKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (typeSelector);
    addAndMakeVisible (enableBtn);
    addAndMakeVisible (curveViz);

    typeSelector.addItemList ({ "Tanh", "Hard Clip", "Asymmetric" }, 1);

    driveAtt  = std::make_unique<SliderAttachment> (apvts, ParamIDs::fuzzDrive, driveKnob);
    toneAtt   = std::make_unique<SliderAttachment> (apvts, ParamIDs::fuzzTone,  toneKnob);
    mixAtt    = std::make_unique<SliderAttachment> (apvts, ParamIDs::fuzzMix,   mixKnob);
    enableAtt = std::make_unique<ButtonAttachment> (apvts, ParamIDs::fuzzEnabled, enableBtn);
    typeAtt   = std::make_unique<ComboBoxAttachment> (apvts, ParamIDs::fuzzType, typeSelector);
}

void FuzzPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    g.setColour (juce::Colour (0xffcc8833));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 8.0f, 1.5f);
}

void FuzzPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto topRow = bounds.removeFromTop (28);
    enableBtn.setBounds (topRow.removeFromLeft (80));
    typeSelector.setBounds (topRow.removeFromRight (100));

    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    curveViz.setBounds (vizArea.reduced (4));

    auto knobRow = bounds;
    int knobW = knobRow.getWidth() / 3;
    driveKnob.setBounds (knobRow.removeFromLeft (knobW));
    toneKnob.setBounds  (knobRow.removeFromLeft (knobW));
    mixKnob.setBounds   (knobRow);
}

void FuzzPanel::TransferCurveViz::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0d0d));

    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    // Draw grid lines
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.drawLine (bounds.getCentreX(), bounds.getY(), bounds.getCentreX(), bounds.getBottom());
    g.drawLine (bounds.getX(), bounds.getCentreY(), bounds.getRight(), bounds.getCentreY());

    // Draw transfer curve
    juce::Path curve;
    bool started = false;

    for (float px = 0; px < bounds.getWidth(); px += 1.0f)
    {
        float input = juce::jmap (px, 0.0f, bounds.getWidth(), -1.0f, 1.0f);
        float output = fuzzRef.getTransferValue (input);
        output = juce::jlimit (-1.0f, 1.0f, output);

        float y = juce::jmap (output, -1.0f, 1.0f, bounds.getBottom(), bounds.getY());

        if (! started)
        {
            curve.startNewSubPath (bounds.getX() + px, y);
            started = true;
        }
        else
        {
            curve.lineTo (bounds.getX() + px, y);
        }
    }

    g.setColour (juce::Colour (0xffff8800));
    g.strokePath (curve, juce::PathStrokeType (2.0f));
}
