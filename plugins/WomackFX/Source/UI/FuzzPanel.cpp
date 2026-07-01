#include "FuzzPanel.h"
#include "../Parameters.h"
#include "../../../Shared/UI/WomackSkin.h"

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

    auto setupLabel = [] (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
        label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        label.setInterceptsMouseClicks (false, false);
    };

    setupLabel (driveLabel);
    setupLabel (toneLabel);
    setupLabel (mixLabel);
    setupLabel (typeLabel);
    setupLabel (curveLabel);

    addAndMakeVisible (driveKnob);
    addAndMakeVisible (toneKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (driveLabel);
    addAndMakeVisible (toneLabel);
    addAndMakeVisible (mixLabel);
    addAndMakeVisible (typeSelector);
    addAndMakeVisible (typeLabel);
    addAndMakeVisible (enableBtn);
    addAndMakeVisible (curveViz);
    addAndMakeVisible (curveLabel);

    driveKnob.setTooltip ("Drive amount feeding the fuzz stage.");
    toneKnob.setTooltip ("Tone filter cutoff shaping the fuzz brightness.");
    mixKnob.setTooltip ("Blend between the dry signal and fuzzed signal.");
    typeSelector.setTooltip ("Select the clipping circuit used by the fuzz.");
    enableBtn.setTooltip ("Enable or bypass the fuzz effect.");
    curveViz.setTooltip ("Live transfer-curve display showing how the fuzz is shaping the signal.");

    typeSelector.addItemList ({ "Tanh", "Hard Clip", "Asymmetric" }, 1);

    driveAtt  = std::make_unique<SliderAttachment> (apvts, ParamIDs::fuzzDrive, driveKnob);
    toneAtt   = std::make_unique<SliderAttachment> (apvts, ParamIDs::fuzzTone,  toneKnob);
    mixAtt    = std::make_unique<SliderAttachment> (apvts, ParamIDs::fuzzMix,   mixKnob);
    enableAtt = std::make_unique<ButtonAttachment> (apvts, ParamIDs::fuzzEnabled, enableBtn);
    typeAtt   = std::make_unique<ComboBoxAttachment> (apvts, ParamIDs::fuzzType, typeSelector);
}

void FuzzPanel::paint (juce::Graphics& g)
{
    WomackSkin::paintPanelShell (g, getLocalBounds().toFloat(), WomackSkin::accentCopper());
}

void FuzzPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto topRow = bounds.removeFromTop (40);
    enableBtn.setBounds (topRow.removeFromLeft (80));
    auto typeArea = topRow.removeFromRight (100);
    typeLabel.setBounds (typeArea.removeFromTop (14));
    typeSelector.setBounds (typeArea);

    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    curveLabel.setBounds (vizArea.removeFromTop (16));
    curveViz.setBounds (vizArea.reduced (4));

    auto knobRow = bounds;
    int knobW = knobRow.getWidth() / 3;

    auto layoutKnob = [] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& knob)
    {
        label.setBounds (area.removeFromTop (16));
        knob.setBounds (area);
    };

    layoutKnob (knobRow.removeFromLeft (knobW), driveLabel, driveKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), toneLabel, toneKnob);
    layoutKnob (knobRow, mixLabel, mixKnob);
}

void FuzzPanel::TransferCurveViz::paint (juce::Graphics& g)
{
    auto frame = getLocalBounds().toFloat().reduced (1.0f);
    WomackSkin::paintDisplayWell (g, frame, WomackSkin::accentCopper());

    auto bounds = frame.reduced (8.0f);

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

    g.setColour (WomackSkin::accentAmber());
    g.strokePath (curve, juce::PathStrokeType (2.0f));
}
