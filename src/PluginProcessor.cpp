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
    inGain    = apvts.getRawParameterValue ("ingain");
    outGain   = apvts.getRawParameterValue ("outgain");

    resetShape();
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

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ingain", 1 }, "Input Gain", juce::NormalisableRange<float> (0.0f, 2.0f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outgain", 1 }, "Output Gain", juce::NormalisableRange<float> (0.0f, 2.0f), 1.0f));

    return { params.begin(), params.end() };
}

void MortalProcessor::resetShape() noexcept
{
    for (int i = 0; i < shapeSize; ++i)
        shape[(size_t) i].store (0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * (float) i / (float) shapeSize));
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

    // Entrada
    buffer.applyGain (0, 0, n, inGain->load());
    buffer.applyGain (1, 0, n, inGain->load());

    // LFO: lee la forma dibujada en la fase actual (interpolacion lineal)
    const float pos = (float) lfoPhase * (float) shapeSize;
    const int i0 = juce::jlimit (0, shapeSize - 1, (int) pos);
    const int i1 = (i0 + 1) % shapeSize;
    const float frac = pos - (float) i0;
    const float shapeV = getShapeValue (i0) + (getShapeValue (i1) - getShapeValue (i0)) * frac;
    const float mod = lfoDepth->load() * (2.0f * shapeV - 1.0f);

    lfoPhase = std::fmod (lfoPhase + (double) lfoRate->load() * n / sampleRate, 1.0);
    displayPhase.store ((float) lfoPhase);

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

    // Salida
    buffer.applyGain (0, 0, n, outGain->load());
    buffer.applyGain (1, 0, n, outGain->load());

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

    juce::StringArray vals;
    for (int i = 0; i < shapeSize; ++i)
        vals.add (juce::String (getShapeValue (i), 4));
    xml->setAttribute ("lfoShape", vals.joinIntoString (","));

    copyXmlToBinary (*xml, destData);
}

void MortalProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
    {
        const auto shapeStr = xml->getStringAttribute ("lfoShape");
        if (shapeStr.isNotEmpty())
        {
            auto parts = juce::StringArray::fromTokens (shapeStr, ",", "");
            if (parts.size() == shapeSize)
                for (int i = 0; i < shapeSize; ++i)
                    setShapeValue (i, parts[i].getFloatValue());
        }
        xml->removeAttribute ("lfoShape");
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MortalProcessor();
}
