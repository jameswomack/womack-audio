#include "ResonotePanel.h"
#include "../DSP/NoteFrequency.h"

ResonotePanel::ResonotePanel (ResonoteProcessor& processor)
    : proc (processor),
      responseCurve (processor)
{
    setLookAndFeel (&lookAndFeel);

    auto& apvts = proc.getAPVTS();

    modeParam      = apvts.getRawParameterValue (ParamIDs::mode);
    midiTrackParam = apvts.getRawParameterValue (ParamIDs::midiTrack);

    auto setupKnob = [this] (juce::Slider& s, const juce::String& suffix = "")
    {
        WomackSkin::configureKnob (s, this, suffix);
        addAndMakeVisible (s);
    };

    setupKnob (freqKnob, " Hz");
    setupKnob (resKnob);
    setupKnob (gainKnob, " dB");
    setupKnob (outKnob,  " dB");

    auto setupLabel = [this] (juce::Label& label)
    {
        WomackSkin::configureLabel (label);
        addAndMakeVisible (label);
    };

    setupLabel (freqLabel);
    setupLabel (resLabel);
    setupLabel (gainLabel);
    setupLabel (outLabel);
    setupLabel (modeLabel);

    noteReadout.setJustificationType (juce::Justification::centred);
    noteReadout.setColour (juce::Label::textColourId, WomackSkin::accentIce());
    noteReadout.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    noteReadout.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (noteReadout);

    addAndMakeVisible (responseCurve);
    responseCurve.setTooltip ("Filter response over a log-frequency axis with note gridlines; the orange marker is the locked note.");

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

    modeSelector.setColour (juce::ComboBox::textColourId, WomackSkin::textStrong());
    modeSelector.setColour (juce::ComboBox::outlineColourId, WomackSkin::accentIce().withAlpha (0.55f));

    startTimerHz (30);
}

ResonotePanel::~ResonotePanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
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
        gainKnob.setAlpha (isBell ? 1.0f : 0.35f);
        gainLabel.setAlpha (isBell ? 1.0f : 0.45f);
    }

    const bool midiOn = midiTrackParam->load() > 0.5f;
    if (freqKnob.isEnabled() == midiOn)   // state changed
    {
        freqKnob.setEnabled  (! midiOn);
        freqLabel.setEnabled (! midiOn);
        freqKnob.setAlpha (! midiOn ? 1.0f : 0.35f);
        freqLabel.setAlpha (! midiOn ? 1.0f : 0.45f);
    }
}

void ResonotePanel::paint (juce::Graphics& g)
{
    WomackSkin::paintEditorBackground (g, getLocalBounds(), WomackSkin::accentIce());

    auto bounds = getLocalBounds().reduced (18);
    auto header = bounds.removeFromTop (56);

    g.setColour (WomackSkin::textStrong());
    g.setFont (juce::FontOptions (23.0f, juce::Font::bold));
    g.drawText ("WOMACK RESONOTE", header.removeFromTop (28), juce::Justification::centred);

    g.setColour (WomackSkin::textMuted());
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("note-aware resonant filter", header, juce::Justification::centred);
}

void ResonotePanel::resized()
{
    auto bounds = getLocalBounds().reduced (20);
    bounds.removeFromTop (60);

    auto topRow = bounds.removeFromTop (60);
    auto modeArea = topRow.removeFromLeft (220);
    modeLabel.setBounds (modeArea.removeFromTop (18));
    modeSelector.setBounds (modeArea.reduced (0, 4));
    midiBtn.setBounds (topRow.removeFromRight (104).reduced (4, 10));
    snapBtn.setBounds (topRow.removeFromRight (104).reduced (4, 10));

    auto knobRow = bounds.removeFromBottom (148);
    const int knobW = knobRow.getWidth() / 4;

    auto layoutKnob = [] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& knob)
    {
        label.setBounds (area.removeFromTop (18));
        knob.setBounds (area.reduced (2, 0));
    };

    layoutKnob (knobRow.removeFromLeft (knobW), freqLabel, freqKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), resLabel,  resKnob);
    layoutKnob (knobRow.removeFromLeft (knobW), gainLabel, gainKnob);
    layoutKnob (knobRow,                        outLabel,  outKnob);

    noteReadout.setBounds (bounds.removeFromBottom (52).reduced (10, 2));
    responseCurve.setBounds (bounds.reduced (2));
}
