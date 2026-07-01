#include "TapeDelayPanel.h"
#include "../Parameters.h"
#include "../../../Shared/UI/WomackSkin.h"

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

    auto setupLabel = [] (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
        label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        label.setInterceptsMouseClicks (false, false);
    };

    setupLabel (timeLabel);
    setupLabel (feedbackLabel);
    setupLabel (modLabel);
    setupLabel (satLabel);
    setupLabel (mixLabel);
    setupLabel (reelLabel);

    addAndMakeVisible (timeKnob);
    addAndMakeVisible (feedbackKnob);
    addAndMakeVisible (modKnob);
    addAndMakeVisible (satKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (timeLabel);
    addAndMakeVisible (feedbackLabel);
    addAndMakeVisible (modLabel);
    addAndMakeVisible (satLabel);
    addAndMakeVisible (mixLabel);
    addAndMakeVisible (enableBtn);
    addAndMakeVisible (reelViz);
    addAndMakeVisible (reelLabel);

    timeKnob.setTooltip ("Delay time in milliseconds.");
    feedbackKnob.setTooltip ("Amount of delayed signal fed back into the repeats.");
    modKnob.setTooltip ("Tape-style modulation depth applied to the repeats.");
    satKnob.setTooltip ("Saturation applied to the delay line for tape character.");
    mixKnob.setTooltip ("Blend between the dry signal and delayed signal.");
    enableBtn.setTooltip ("Enable or bypass the tape delay effect.");
    reelViz.setTooltip ("Animated tape-reel display reflecting the current delay movement.");

    timeAtt     = std::make_unique<SliderAttachment> (apvts, ParamIDs::delayTime,     timeKnob);
    feedbackAtt = std::make_unique<SliderAttachment> (apvts, ParamIDs::delayFeedback, feedbackKnob);
    modAtt      = std::make_unique<SliderAttachment> (apvts, ParamIDs::delayMod,      modKnob);
    satAtt      = std::make_unique<SliderAttachment> (apvts, ParamIDs::delaySat,      satKnob);
    mixAtt      = std::make_unique<SliderAttachment> (apvts, ParamIDs::delayMix,      mixKnob);
    enableAtt   = std::make_unique<ButtonAttachment> (apvts, ParamIDs::delayEnabled,  enableBtn);
}

void TapeDelayPanel::paint (juce::Graphics& g)
{
    WomackSkin::paintPanelShell (g, getLocalBounds().toFloat(), WomackSkin::accentBlue());
}

void TapeDelayPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto topRow = bounds.removeFromTop (28);
    enableBtn.setBounds (topRow.removeFromLeft (80));

    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    reelLabel.setBounds (vizArea.removeFromTop (16));
    reelViz.setBounds (vizArea.reduced (4));

    auto knobRow = bounds;
    int knobW = knobRow.getWidth() / 5;

    auto layoutKnob = [] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& knob)
    {
        label.setBounds (area.removeFromTop (16));
        knob.setBounds (area);
    };

    layoutKnob (knobRow.removeFromLeft (knobW), timeLabel, timeKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), feedbackLabel, feedbackKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), modLabel, modKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), satLabel, satKnob);
    layoutKnob (knobRow, mixLabel, mixKnob);
}

void TapeDelayPanel::TapeReelViz::paint (juce::Graphics& g)
{
    auto frame = getLocalBounds().toFloat().reduced (1.0f);
    WomackSkin::paintDisplayWell (g, frame, WomackSkin::accentBlue());

    auto bounds = frame.reduced (12.0f);
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
        g.setColour (WomackSkin::accentBlue().withAlpha (0.6f));
        for (int i = 0; i < 3; ++i)
        {
            float a = angle + i * juce::MathConstants<float>::twoPi / 3.0f;
            float x2 = cx + std::cos (a) * reelRadius * 0.8f;
            float y2 = cy + std::sin (a) * reelRadius * 0.8f;
            g.drawLine (cx, cy, x2, y2, 1.5f);
        }

        // Hub
        g.setColour (WomackSkin::accentBlue());
        g.fillEllipse (cx - 4, cy - 4, 8, 8);
    };

    drawReel (leftCx, 1.0f);
    drawReel (rightCx, -1.0f);

    // Tape path between reels
    g.setColour (WomackSkin::accentBlue().withAlpha (0.3f));
    g.drawLine (leftCx + reelRadius, cy, rightCx - reelRadius, cy, 2.0f);
}
