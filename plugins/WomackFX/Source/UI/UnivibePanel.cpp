#include "UnivibePanel.h"
#include "../Parameters.h"
#include "../../../Shared/UI/WomackSkin.h"

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

    auto setupLabel = [] (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
        label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        label.setInterceptsMouseClicks (false, false);
    };

    setupLabel (rateLabel);
    setupLabel (depthLabel);
    setupLabel (feedbackLabel);
    setupLabel (mixLabel);
    setupLabel (rotorLabel);

    addAndMakeVisible (rateKnob);
    addAndMakeVisible (depthKnob);
    addAndMakeVisible (feedbackKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (rateLabel);
    addAndMakeVisible (depthLabel);
    addAndMakeVisible (feedbackLabel);
    addAndMakeVisible (mixLabel);
    addAndMakeVisible (enableBtn);
    addAndMakeVisible (rotorViz);
    addAndMakeVisible (rotorLabel);

    rateKnob.setTooltip ("LFO speed controlling the Univibe swirl rate.");
    depthKnob.setTooltip ("Depth of the phase modulation.");
    feedbackKnob.setTooltip ("Feedback amount for a more pronounced throb.");
    mixKnob.setTooltip ("Blend between the dry signal and Univibe effect.");
    enableBtn.setTooltip ("Enable or bypass the Univibe effect.");
    rotorViz.setTooltip ("Animated rotor display reflecting the current Univibe modulation phase.");

    rateAtt     = std::make_unique<SliderAttachment> (apvts, ParamIDs::vibeRate,     rateKnob);
    depthAtt    = std::make_unique<SliderAttachment> (apvts, ParamIDs::vibeDepth,    depthKnob);
    feedbackAtt = std::make_unique<SliderAttachment> (apvts, ParamIDs::vibeFeedback, feedbackKnob);
    mixAtt      = std::make_unique<SliderAttachment> (apvts, ParamIDs::vibeMix,      mixKnob);
    enableAtt   = std::make_unique<ButtonAttachment> (apvts, ParamIDs::vibeEnabled,  enableBtn);
}

void UnivibePanel::paint (juce::Graphics& g)
{
    WomackSkin::paintPanelShell (g, getLocalBounds().toFloat(), WomackSkin::accentTeal());
}

void UnivibePanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);

    auto topRow = bounds.removeFromTop (28);
    enableBtn.setBounds (topRow.removeFromLeft (80));

    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    rotorLabel.setBounds (vizArea.removeFromTop (16));
    rotorViz.setBounds (vizArea.reduced (4));

    auto knobRow = bounds;
    int knobW = knobRow.getWidth() / 4;

    auto layoutKnob = [] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& knob)
    {
        label.setBounds (area.removeFromTop (16));
        knob.setBounds (area);
    };

    layoutKnob (knobRow.removeFromLeft (knobW), rateLabel, rateKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), depthLabel, depthKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), feedbackLabel, feedbackKnob);
    layoutKnob (knobRow, mixLabel, mixKnob);
}

void UnivibePanel::RotorViz::paint (juce::Graphics& g)
{
    auto frame = getLocalBounds().toFloat().reduced (1.0f);
    WomackSkin::paintDisplayWell (g, frame, WomackSkin::accentTeal());

    auto bounds = frame.reduced (12.0f);
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

        juce::ColourGradient grad (WomackSkin::accentTeal().withAlpha (0.8f), cx, cy,
                                    WomackSkin::accentTeal().withAlpha (0.1f), x1, y1, true);
        g.setGradientFill (grad);
        g.drawLine (x1, y1, x2, y2, 3.0f);
    }

    // Center dot
    g.setColour (WomackSkin::accentTeal());
    g.fillEllipse (cx - 3, cy - 3, 6, 6);
}
