#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

WomackFXProcessor::WomackFXProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout()),
      fuzz (apvts),
      univibe (apvts),
      tapeDelay (apvts)
{
}

juce::AudioProcessorValueTreeState::ParameterLayout WomackFXProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Fuzz parameters
    params.push_back (std::make_unique<juce::AudioParameterBool>   (juce::ParameterID { ParamIDs::fuzzEnabled, 1 }, "Fuzz On",   false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::fuzzDrive,   1 }, "Drive",     juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f, 0.5f), 5.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::fuzzTone,    1 }, "Tone",      juce::NormalisableRange<float> (200.0f, 20000.0f, 1.0f, 0.3f), 4000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::fuzzMix,     1 }, "Fuzz Mix",  0.0f, 1.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::fuzzType,    1 }, "Fuzz Type", juce::StringArray { "Tanh", "Hard Clip", "Asymmetric" }, 0));

    // Univibe parameters
    params.push_back (std::make_unique<juce::AudioParameterBool>   (juce::ParameterID { ParamIDs::vibeEnabled,  1 }, "Vibe On",      false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::vibeRate,     1 }, "Rate",         juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 2.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::vibeDepth,    1 }, "Depth",        0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::vibeFeedback, 1 }, "Vibe FB",      juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::vibeMix,      1 }, "Vibe Mix",     0.0f, 1.0f, 0.5f));

    // Tape Delay parameters
    params.push_back (std::make_unique<juce::AudioParameterBool>   (juce::ParameterID { ParamIDs::delayEnabled,  1 }, "Delay On",     false));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::delayTime,     1 }, "Time",         juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.4f), 350.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::delayFeedback, 1 }, "Delay FB",     juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.4f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::delayMod,      1 }, "Modulation",   0.0f, 1.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::delaySat,      1 }, "Saturation",   0.0f, 1.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { ParamIDs::delayMix,      1 }, "Delay Mix",    0.0f, 1.0f, 0.5f));

    return { params.begin(), params.end() };
}

void WomackFXProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    fuzz.prepare (spec);
    univibe.prepare (spec);
    tapeDelay.prepare (spec);
}

void WomackFXProcessor::releaseResources()
{
    fuzz.reset();
    univibe.reset();
    tapeDelay.reset();
}

bool WomackFXProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void WomackFXProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block (buffer);

    // Signal chain: Fuzz -> Univibe -> Tape Delay
    fuzz.process (block);
    univibe.process (block);
    tapeDelay.process (block);
}

void WomackFXProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WomackFXProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* WomackFXProcessor::createEditor()
{
    return new WomackFXEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WomackFXProcessor();
}
