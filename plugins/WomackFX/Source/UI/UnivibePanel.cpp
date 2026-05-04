#include "UnivibePanel.h"
#include "../Parameters.h"

UnivibePanel::UnivibePanel (juce::AudioProcessorValueTreeState& apvts, UnivibeEffect& vibe)
    : vibeEffect (vibe), rotorViz (vibe)
{
    auto setupKnob = [] (juce::Slider& s, const juce::String& suffix = "")
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
        if (suffix.isNotEmpty())
            s.setTextValueSuffix (suffix);
    };

    setupKnob (rateKnob, " Hz");
    setupKnob (depthKnob);
    setupKnob (feedbackKnob);
    setupKnob (mixKnob);

    addAndMakeVisible (rateKnob);
    addAndMakeVisible (depthKnob);
    addAndMakeVisible (feedbackKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (enableBtn);
    addAndMakeVisible (rotorViz);

    rateAtt     = std::make_unique<SliderAttachment> (apvts, ParamIDs::vibeRate,     rateKnob);
    depthAtt    = std::make_unique<SliderAttachment> (apvts, ParamIDs::vibeDepth,    depthKnob);
    feedbackAtt = std::make_unique<SliderAttachment> (apvts, ParamIDs::vibeFeedback, feedbackKnob);
    mixAtt      = std::make_unique<SliderAttachment> (apvts, ParamIDs::vibeMix,      mixKnob);
    enableAtt   = std::make_unique<ButtonAttachment> (apvts, ParamIDs::vibeEnabled,  enableBtn);
}

void UnivibePanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    g.setColour (juce::Colour (0xff33aa88));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 8.0f, 1.5f);
}

void UnivibePanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto topRow = bounds.removeFromTop (28);
    enableBtn.setBounds (topRow.removeFromLeft (80));

    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    rotorViz.setBounds (vizArea.reduced (4));

    auto knobRow = bounds;
    int knobW = knobRow.getWidth() / 4;
    rateKnob.setBounds     (knobRow.removeFromLeft (knobW));
    depthKnob.setBounds    (knobRow.removeFromLeft (knobW));
    feedbackKnob.setBounds (knobRow.removeFromLeft (knobW));
    mixKnob.setBounds      (knobRow);
}

void UnivibePanel::RotorViz::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0d0d));

    auto bounds = getLocalBounds().toFloat().reduced (8.0f);
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.4f;

    // Draw rotor housing
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.drawEllipse (cx - radius, cy - radius, radius * 2, radius * 2, 1.0f);

    // Draw spinning rotor
    float phase = vibeRef.getLfoPhase();

    for (int i = 0; i < 2; ++i)
    {
        float angle = phase + i * juce::MathConstants<float>::pi;
        float x1 = cx + std::cos (angle) * radius * 0.85f;
        float y1 = cy + std::sin (angle) * radius * 0.85f;
        float x2 = cx - std::cos (angle) * radius * 0.85f;
        float y2 = cy - std::sin (angle) * radius * 0.85f;

        juce::ColourGradient grad (juce::Colour (0xff33aa88).withAlpha (0.8f), cx, cy,
                                    juce::Colour (0xff33aa88).withAlpha (0.1f), x1, y1, true);
        g.setGradientFill (grad);
        g.drawLine (x1, y1, x2, y2, 3.0f);
    }

    // Center dot
    g.setColour (juce::Colour (0xff33aa88));
    g.fillEllipse (cx - 3, cy - 3, 6, 6);
}
