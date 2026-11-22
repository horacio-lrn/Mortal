#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

MortalProcessor::MortalProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createLayout())
{
    raw[0] = apvts.getRawParameterValue ("position");
    raw[1] = apvts.getRawParameterValue ("size");
    raw[2] = apvts.getRawParameterValue ("density");
    raw[3] = apvts.getRawParameterValue ("pitch");
    raw[4] = apvts.getRawParameterValue ("spray");
    raw[5] = apvts.getRawParameterValue ("feedback");
    raw[6] = apvts.getRawParameterValue ("mix");
    raw[7] = apvts.getRawParameterValue ("durrnd");

    lfoRate   = apvts.getRawParameterValue ("lforate");
    lfoDepth  = apvts.getRawParameterValue ("lfodepth");
    lfoTarget = apvts.getRawParameterValue ("lfotarget");
}

juce::AudioProcessorValueTreeState::ParameterLayout MortalProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "position", 1 }, "Position", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size", 1 }, "Duration (ms)", juce::NormalisableRange<float> (5.0f, 500.0f, 0.0f, 0.4f), 80.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "density", 1 }, "Density", juce::NormalisableRange<float> (1.0f, 100.0f, 0.0f, 0.5f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pitch", 1 }, "Pitch (st)", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spray", 1 }, "Spray", juce::NormalisableRange<float> (0.0f, 1.0f), 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback", juce::NormalisableRange<float> (0.0f, 0.95f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "durrnd", 1 }, "Duration Random", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lforate", 1 }, "LFO Rate (Hz)", juce::NormalisableRange<float> (0.05f, 10.0f, 0.0f, 0.4f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfodepth", 1 }, "LFO Depth", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "lfotarget", 1 }, "LFO Target",
        juce::StringArray { "Position", "Density", "Pitch", "Spray", "Mix" }, 0));

    return { params.begin(), params.end() };
}

void MortalProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    engine.prepare (sr);
}

void MortalProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int n = buffer.getNumSamples();

    // LFO sinusoidal: se calcula una vez por bloque y modula un unico destino
    const double lfoValue = std::sin (juce::MathConstants<double>::twoPi * lfoPhase);
    lfoPhase = std::fmod (lfoPhase + (double) lfoRate->load() * n / sampleRate, 1.0);
    const float mod = lfoDepth->load() * (float) lfoValue;

    GranularEngine::Params p { raw[0]->load(), raw[1]->load(), raw[2]->load(), raw[3]->load(),
                               raw[4]->load(), raw[5]->load(), raw[6]->load(), raw[7]->load() };

    switch ((int) lfoTarget->load())
    {
        case 0: p.position = juce::jlimit (0.0f, 1.0f, p.position + mod * 0.5f); break;
        case 1: p.density  = juce::jlimit (1.0f, 100.0f, p.density + mod * 40.0f); break;
        case 2: p.pitch    = juce::jlimit (-24.0f, 24.0f, p.pitch + mod * 12.0f); break;
        case 3: p.spray    = juce::jlimit (0.0f, 1.0f, p.spray + mod * 0.5f); break;
        case 4: p.mix      = juce::jlimit (0.0f, 1.0f, p.mix + mod * 0.5f); break;
        default: break;
    }

    engine.setParams (p);
    engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
    activity.store (engine.getActivity());
}

juce::AudioProcessorEditor* MortalProcessor::createEditor()
{
    return new MortalEditor (*this);
}

void MortalProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MortalProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MortalProcessor();
}
