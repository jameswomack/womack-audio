#include "ResonotePanel.h"
#include "../DSP/NoteFrequency.h"

ResonotePanel::ResonotePanel (ResonoteProcessor& processor)
    : proc (processor),
      responseCurve (processor)
{
    auto& apvts = proc.getAPVTS();

    modeParam      = apvts.getRawParameterValue (ParamIDs::mode);
    midiTrackParam = apvts.getRawParameterValue (ParamIDs::midiTrack);

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

    addAndMakeVisible (responseCurve);
    responseCurve.setTooltip ("Filter response over a log-frequency axis with note gridlines; the orange marker is the locked note.");

    modeSelector.addItemList ({ "Bell", "Low-Pass", "High-Pass" }, 1);
    addAndMakeVisible (modeSelector);
    addAndMakeVisible (snapBtn);
    addAndMakeVisible (midiBtn);

    rootSelector.addItemList ({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    scaleSelector.addItemList ({ "Chromatic", "Major", "Minor" }, 1);
    setupLabel (rootLabel);
    setupLabel (scaleLabel);
    addAndMakeVisible (rootSelector);
    addAndMakeVisible (scaleSelector);

    freqKnob.setTooltip ("Filter frequency. Snaps to the nearest note when SNAP is on; set by MIDI notes when MIDI is on.");
    resKnob.setTooltip  ("Resonance / Q. Higher values sharpen the band and add warm harmonic character.");
    gainKnob.setTooltip ("Boost or cut at the center frequency (Bell mode only).");
    outKnob.setTooltip  ("Output level trim.");
    modeSelector.setTooltip ("Filter shape: Bell, Low-Pass, or High-Pass.");
    snapBtn.setTooltip ("Snap the frequency to the nearest note in the selected scale.");
    midiBtn.setTooltip ("Let incoming MIDI notes set the frequency (last note wins).");
    rootSelector.setTooltip ("Root note of the snap scale.");
    scaleSelector.setTooltip ("Scale that SNAP quantizes the frequency to.");

    modeAtt  = std::make_unique<ComboBoxAttachment> (apvts, ParamIDs::mode, modeSelector);
    freqAtt  = std::make_unique<SliderAttachment>   (apvts, ParamIDs::frequency, freqKnob);
    resAtt   = std::make_unique<SliderAttachment>   (apvts, ParamIDs::resonance, resKnob);
    gainAtt  = std::make_unique<SliderAttachment>   (apvts, ParamIDs::gain, gainKnob);
    outAtt   = std::make_unique<SliderAttachment>   (apvts, ParamIDs::output, outKnob);
    snapAtt  = std::make_unique<ButtonAttachment>   (apvts, ParamIDs::snap, snapBtn);
    midiAtt  = std::make_unique<ButtonAttachment>   (apvts, ParamIDs::midiTrack, midiBtn);
    rootAtt  = std::make_unique<ComboBoxAttachment> (apvts, ParamIDs::root, rootSelector);
    scaleAtt = std::make_unique<ComboBoxAttachment> (apvts, ParamIDs::scale, scaleSelector);

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
    const int    cents = juce::roundToInt (NoteFrequency::centsFromNearest (hz));

    juce::String txt = NoteFrequency::noteName (midi);
    txt << "   " << (cents >= 0 ? "+" : "") << cents << " c";
    noteReadout.setText (txt, juce::dontSendNotification);

    const bool isBell = (int) modeParam->load() == 0;
    if (gainKnob.isEnabled() != isBell)
    {
        gainKnob.setEnabled (isBell);
        gainLabel.setEnabled (isBell);
    }

    const bool midiOn = midiTrackParam->load() > 0.5f;
    if (freqKnob.isEnabled() == midiOn)   // state changed
    {
        freqKnob.setEnabled  (! midiOn);
        freqLabel.setEnabled (! midiOn);
    }

    const bool scaleActive = proc.getScaleType() != 0; // 0 = Chromatic
    if (rootSelector.isEnabled() != scaleActive)
    {
        rootSelector.setEnabled (scaleActive);
        rootLabel.setEnabled (scaleActive);
    }
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
    // Right side: SNAP + MIDI toggles.
    midiBtn.setBounds (topRow.removeFromRight (84).reduced (4));
    snapBtn.setBounds (topRow.removeFromRight (84).reduced (4));
    // Left side: Mode | Root | Scale dropdowns.
    auto dropW = juce::jmax (90, topRow.getWidth() / 3 - 6);
    auto modeArea = topRow.removeFromLeft (dropW);
    modeLabel.setBounds (modeArea.removeFromTop (16));
    modeSelector.setBounds (modeArea.reduced (2, 4));
    auto rootArea = topRow.removeFromLeft (juce::jmin (dropW, 80));
    rootLabel.setBounds (rootArea.removeFromTop (16));
    rootSelector.setBounds (rootArea.reduced (2, 4));
    auto scaleArea = topRow.removeFromLeft (juce::jmin (dropW, 120));
    scaleLabel.setBounds (scaleArea.removeFromTop (16));
    scaleSelector.setBounds (scaleArea.reduced (2, 4));

    // Knob row pinned to the bottom (fixed height); the viz takes the rest.
    auto knobRow = bounds.removeFromBottom (128);
    const int knobW = knobRow.getWidth() / 4;

    auto layoutKnob = [] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& knob)
    {
        label.setBounds (area.removeFromTop (16));
        knob.setBounds (area.reduced (4, 0));
    };

    layoutKnob (knobRow.removeFromLeft (knobW), freqLabel, freqKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), resLabel,  resKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), gainLabel, gainKnob);
    layoutKnob (knobRow,                        outLabel,  outKnob);

    // Note readout just above the knobs.
    noteReadout.setBounds (bounds.removeFromBottom (44));

    // Response curve fills all remaining space and grows with the window.
    responseCurve.setBounds (bounds.reduced (2));
}
