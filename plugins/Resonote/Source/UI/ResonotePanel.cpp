#include "ResonotePanel.h"
#include "../DSP/NoteFrequency.h"

ResonotePanel::ResonotePanel (ResonoteProcessor& processor)
    : proc (processor)
{
    auto& apvts = proc.getAPVTS();

    auto setupKnob = [this] (juce::Slider& s, const juce::String& suffix = "")
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        if (suffix.isNotEmpty())
            s.setTextValueSuffix (suffix);
        addAndMakeVisible (s);
    };

    setupKnob (freqKnob, " Hz");
    setupKnob (resKnob);
    setupKnob (gainKnob, " dB");
    setupKnob (outKnob,  " dB");

    auto setupLabel = [this] (juce::Label& label)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
        label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    };

    setupLabel (freqLabel);
    setupLabel (resLabel);
    setupLabel (gainLabel);
    setupLabel (outLabel);
    setupLabel (modeLabel);

    noteReadout.setJustificationType (juce::Justification::centred);
    noteReadout.setColour (juce::Label::textColourId, juce::Colour (0xff66ddff));
    noteReadout.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    noteReadout.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (noteReadout);

    modeSelector.addItemList ({ "Bell", "Low-Pass", "High-Pass" }, 1);
    addAndMakeVisible (modeSelector);
    addAndMakeVisible (snapBtn);
    addAndMakeVisible (midiBtn);

    freqKnob.setTooltip ("Filter frequency. Snaps to the nearest note when SNAP is on; set by MIDI notes when MIDI is on.");
    resKnob.setTooltip  ("Resonance / Q. Higher values sharpen the band and add warm harmonic character.");
    gainKnob.setTooltip ("Boost or cut at the center frequency (Bell mode only).");
    outKnob.setTooltip  ("Output level trim.");
    modeSelector.setTooltip ("Filter shape: Bell, Low-Pass, or High-Pass.");
    snapBtn.setTooltip ("Snap the frequency to the nearest chromatic note.");
    midiBtn.setTooltip ("Let incoming MIDI notes set the frequency (last note wins).");

    modeAtt = std::make_unique<ComboBoxAttachment> (apvts, ParamIDs::mode, modeSelector);
    freqAtt = std::make_unique<SliderAttachment>   (apvts, ParamIDs::frequency, freqKnob);
    resAtt  = std::make_unique<SliderAttachment>   (apvts, ParamIDs::resonance, resKnob);
    gainAtt = std::make_unique<SliderAttachment>   (apvts, ParamIDs::gain, gainKnob);
    outAtt  = std::make_unique<SliderAttachment>   (apvts, ParamIDs::output, outKnob);
    snapAtt = std::make_unique<ButtonAttachment>   (apvts, ParamIDs::snap, snapBtn);
    midiAtt = std::make_unique<ButtonAttachment>   (apvts, ParamIDs::midiTrack, midiBtn);

    startTimerHz (30);
}

ResonotePanel::~ResonotePanel()
{
    stopTimer();
}

void ResonotePanel::timerCallback()
{
    const float hz = proc.getCurrentFrequencyHz();
    const int   midi = NoteFrequency::nearestMidi (hz);
    const double cents = NoteFrequency::centsFromNearest (hz);

    juce::String txt = NoteFrequency::noteName (midi);
    txt << "   " << (cents >= 0.0 ? "+" : "") << juce::String (cents, 0) << " c";
    noteReadout.setText (txt, juce::dontSendNotification);

    const bool isBell = (int) *proc.getAPVTS().getRawParameterValue (ParamIDs::mode) == 0;
    gainKnob.setEnabled (isBell);
    gainLabel.setEnabled (isBell);

    const bool midiOn = *proc.getAPVTS().getRawParameterValue (ParamIDs::midiTrack) > 0.5f;
    freqKnob.setEnabled (! midiOn);
}

void ResonotePanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111111));

    g.setColour (juce::Colour (0xff66ddff));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("WOMACK RESONOTE", getLocalBounds().removeFromTop (36),
                juce::Justification::centred);
}

void ResonotePanel::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    bounds.removeFromTop (32);    // title

    auto topRow = bounds.removeFromTop (56);
    auto modeArea = topRow.removeFromLeft (200);
    modeLabel.setBounds (modeArea.removeFromTop (16));
    modeSelector.setBounds (modeArea.reduced (0, 4));
    midiBtn.setBounds (topRow.removeFromRight (90).reduced (4));
    snapBtn.setBounds (topRow.removeFromRight (90).reduced (4));

    auto vizArea = bounds.removeFromTop (bounds.getHeight() / 2);
    noteReadout.setBounds (vizArea.removeFromBottom (40));

    auto knobRow = bounds;
    const int knobW = knobRow.getWidth() / 4;

    auto layoutKnob = [] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& knob)
    {
        label.setBounds (area.removeFromTop (16));
        knob.setBounds (area);
    };

    layoutKnob (knobRow.removeFromLeft (knobW), freqLabel, freqKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), resLabel,  resKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), gainLabel, gainKnob);
    layoutKnob (knobRow,                        outLabel,  outKnob);
}
