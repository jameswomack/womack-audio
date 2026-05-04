#include "TapeDelayPanel.h"
#include "../Parameters.h"

TapeDelayPanel::TapeDelayPanel (juce::AudioProcessorValueTreeState& apvts, TapeDelayEffect& delay)
    : delayEffect (delay), reelViz (delay)
{
    auto setupKnob = [] (juce::Slider& s, const juce::String& suffix = "")
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
        if (suffix.isNotEmpty())
            s.setTextValueSuffix (suffix);
    };

    setupKnob (timeKnob, " ms");
    setupKnob (feedbackKnob);
    setupKnob (modKnob);
    setupKnob (satKnob);
    setupKnob (mixKnob);

    addAndMakeVisible (timeKnob);
    addAndMakeVisible (feedbackKnob);
    addAndMakeVisible (modKnob);
    addAndMakeVisible (satKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (enableBtn);
    addAndMakeVisible (reelViz);

    timeAtt     = std::make_unique<SliderAttachment> (apvts, ParamIDs::delayTime,     timeKnob);
    feedbackAtt = std::make_unique<SliderAttachment> (apvts, ParamIDs::delayFeedback, feedbackKnob);
    modAtt      = std::make_unique<SliderAttachment> (apvts, ParamIDs::delayMod,      modKnob);
    satAtt      = std::make_unique<SliderAttachment> (apvts, ParamIDs::delaySat,      satKnob);
    mixAtt      = std::make_unique<SliderAttachment> (apvts, ParamIDs::delayMix,      mixKnob);
    enableAtt   = std::make_unique<ButtonAttachment> (apvts, ParamIDs::delayEnabled,  enableBtn);
}

void TapeDelayPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    g.setColour (juce::Colour (0xff6688cc));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 8.0f, 1.5f);
}

void TapeDelayPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto topRow = bounds.removeFromTop (28);
    enableBtn.setBounds (topRow.removeFromLeft (80));

    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    reelViz.setBounds (vizArea.reduced (4));

    auto knobRow = bounds;
    int knobW = knobRow.getWidth() / 5;
    timeKnob.setBounds     (knobRow.removeFromLeft (knobW));
    feedbackKnob.setBounds (knobRow.removeFromLeft (knobW));
    modKnob.setBounds      (knobRow.removeFromLeft (knobW));
    satKnob.setBounds      (knobRow.removeFromLeft (knobW));
    mixKnob.setBounds      (knobRow);
}

void TapeDelayPanel::TapeReelViz::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0d0d));

    auto bounds = getLocalBounds().toFloat().reduced (8.0f);
    float tapePos = delayRef.getTapePosition();

    // Draw two tape reels
    float reelRadius = juce::jmin (bounds.getWidth() * 0.2f, bounds.getHeight() * 0.35f);
    float leftCx  = bounds.getX() + bounds.getWidth() * 0.3f;
    float rightCx = bounds.getX() + bounds.getWidth() * 0.7f;
    float cy = bounds.getCentreY();

    auto drawReel = [&] (float cx, float direction)
    {
        float angle = tapePos * juce::MathConstants<float>::twoPi * direction;

        // Reel outline
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawEllipse (cx - reelRadius, cy - reelRadius, reelRadius * 2, reelRadius * 2, 1.0f);

        // Spokes
        g.setColour (juce::Colour (0xff6688cc).withAlpha (0.6f));
        for (int i = 0; i < 3; ++i)
        {
            float a = angle + i * juce::MathConstants<float>::twoPi / 3.0f;
            float x2 = cx + std::cos (a) * reelRadius * 0.8f;
            float y2 = cy + std::sin (a) * reelRadius * 0.8f;
            g.drawLine (cx, cy, x2, y2, 1.5f);
        }

        // Hub
        g.setColour (juce::Colour (0xff6688cc));
        g.fillEllipse (cx - 4, cy - 4, 8, 8);
    };

    drawReel (leftCx, 1.0f);
    drawReel (rightCx, -1.0f);

    // Tape path between reels
    g.setColour (juce::Colour (0xff6688cc).withAlpha (0.3f));
    g.drawLine (leftCx + reelRadius, cy, rightCx - reelRadius, cy, 2.0f);
}
