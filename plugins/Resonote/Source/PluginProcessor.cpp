#include "PluginProcessor.h"
#include "Parameters.h"
#include "DSP/NoteFrequency.h"

ResonoteProcessor::ResonoteProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    modeParam      = apvts.getRawParameterValue (ParamIDs::mode);
    freqParam      = apvts.getRawParameterValue (ParamIDs::frequency);
    snapParam      = apvts.getRawParameterValue (ParamIDs::snap);
    resParam       = apvts.getRawParameterValue (ParamIDs::resonance);
    gainParam      = apvts.getRawParameterValue (ParamIDs::gain);
    midiTrackParam = apvts.getRawParameterValue (ParamIDs::midiTrack);
    outputParam    = apvts.getRawParameterValue (ParamIDs::output);
}

juce::AudioProcessorValueTreeState::ParameterLayout ResonoteProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::mode, 1 }, "Mode",
        juce::StringArray { "Bell", "Low-Pass", "High-Pass" }, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::frequency, 1 }, "Frequency",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.01f, 0.25f), 220.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::snap, 1 }, "Snap", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::resonance, 1 }, "Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::gain, 1 }, "Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::midiTrack, 1 }, "MIDI Track", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::output, 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));

    return layout;
}

void ResonoteProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    svf.prepare (spec);
    lastMidiNote = -1;
}

bool ResonoteProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == mainOut;
}

void ResonoteProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const bool midiTrack = midiTrackParam->load() > 0.5f;

    if (midiTrack)
    {
        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
                lastMidiNote = msg.getNoteNumber();
        }
    }

    float freq;
    if (midiTrack && lastMidiNote >= 0)
    {
        freq = (float) NoteFrequency::midiToHz (lastMidiNote);
    }
    else
    {
        freq = freqParam->load();
        if (snapParam->load() > 0.5f)
            freq = (float) NoteFrequency::snapToNote (freq);
    }

    currentFreqHz.store (freq);

    svf.setMode (static_cast<ResonantSVF::Mode> ((int) modeParam->load()));
    svf.setFrequency (freq);
    svf.setResonance (resParam->load());
    svf.setGainDb (gainParam->load());

    juce::dsp::AudioBlock<float> block (buffer);
    svf.process (block);

    block.multiplyBy (juce::Decibels::decibelsToGain (outputParam->load()));
}

void ResonoteProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ResonoteProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* ResonoteProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ResonoteProcessor();
}
