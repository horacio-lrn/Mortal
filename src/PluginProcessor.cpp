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
    reverbAmt = apvts.getRawParameterValue ("reverb");
    lowEqOn   = apvts.getRawParameterValue ("loweq");
    delayAmt  = apvts.getRawParameterValue ("delay");
    satAmt    = apvts.getRawParameterValue ("sat");

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

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverb", 1 }, "Large Plate Reverb", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "loweq", 1 }, "Reverb Low EQ", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delay", 1 }, "Warm Delay", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sat", 1 }, "Saturation", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    return { params.begin(), params.end() };
}

void MortalProcessor::applyShapePreset (int presetIndex) noexcept
{
    for (int i = 0; i < shapeSize; ++i)
    {
        const float x = (float) i / (float) shapeSize;
        float v = 0.5f;

        switch (presetIndex)
        {
            case 0: v = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * x); break; // Sine
            case 1: v = x < 0.5f ? 2.0f * x : 2.0f * (1.0f - x); break;                         // Triangle
            case 2: v = x < 0.5f ? 1.0f : 0.0f; break;                                          // Square
            case 3: v = x; break;                                                               // Saw up
            case 4: v = 1.0f - x; break;                                                        // Saw down
            case 5: v = std::floor (x * 4.0f) / 3.0f; break;                                    // Steps
            default: break;                                                                     // Linea plana
        }

        shape[(size_t) i].store (juce::jlimit (0.0f, 1.0f, v));
    }
}

void MortalProcessor::resetShape() noexcept
{
    applyShapePreset (0);
}

void MortalProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    engine.prepare (sr);

    // Reverb de placa grande, mas contenida
    reverb.setSampleRate (sr);
    juce::Reverb::Parameters rp;
    rp.roomSize   = 0.8f;
    rp.damping    = 0.3f;
    rp.width      = 0.8f;
    rp.wetLevel   = 1.0f;
    rp.dryLevel   = 0.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.reset();

    revBuf.setSize (2, juce::jmax (1, samplesPerBlock), false, true, false);

    // Filtro paso alto de un polo a 300 Hz para la reverb (LOW EQ)
    const double fc = 300.0;
    const double rc = 1.0 / (2.0 * juce::MathConstants<double>::pi * fc);
    const double dt = 1.0 / sr;
    hpCoef = (float) (rc / (rc + dt));
    hpX1[0] = hpX1[1] = hpY1[0] = hpY1[1] = 0.0f;

    // Delay: 120 ms, linea de hasta 500 ms
    const int lineSize = (int) (sr * 0.5) + 2;
    for (auto& line : dLine)
        line.assign ((size_t) lineSize, 0.0f);
    dIdx = 0;
    dSamples = (int) (0.12 * sr);
    dLpCoef = (float) (1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * 2000.0 / sr));
    dLpState[0] = dLpState[1] = 0.0f;
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
    inLevel.store (juce::jmax (buffer.getMagnitude (0, 0, n), buffer.getMagnitude (1, 0, n)));

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

    // Reverb de placa grande (mas sutil), con LOW EQ opcional sobre la cola
    const float revLevel = reverbAmt->load() * 0.35f;
    if (revLevel > 0.0001f)
    {
        if (revBuf.getNumSamples() < n)
            revBuf.setSize (2, n, false, false, true);

        revBuf.copyFrom (0, 0, buffer, 0, 0, n);
        revBuf.copyFrom (1, 0, buffer, 1, 0, n);
        reverb.processStereo (revBuf.getWritePointer (0), revBuf.getWritePointer (1), n);

        if (lowEqOn->load() > 0.5f)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                float* w = revBuf.getWritePointer (ch);
                float x1 = hpX1[ch], y1 = hpY1[ch];
                for (int i = 0; i < n; ++i)
                {
                    const float x = w[i];
                    const float y = hpCoef * (y1 + x - x1);
                    x1 = x;
                    y1 = y;
                    w[i] = y;
                }
                hpX1[ch] = x1;
                hpY1[ch] = y1;
            }
        }

        buffer.addFrom (0, 0, revBuf, 0, 0, n, revLevel);
        buffer.addFrom (1, 0, revBuf, 1, 0, n, revLevel);
    }

    // Delay corto y calido: el repetido pasa por un paso bajo y se realimenta suavemente
    {
        const float dLvl = delayAmt->load();
        const int lineSize = (int) dLine[0].size();
        float* w0 = buffer.getWritePointer (0);
        float* w1 = buffer.getWritePointer (1);

        for (int i = 0; i < n; ++i)
        {
            const int rd = (dIdx - dSamples + lineSize) % lineSize;

            const float d0 = dLine[0][(size_t) rd];
            const float d1 = dLine[1][(size_t) rd];
            dLpState[0] += dLpCoef * (d0 - dLpState[0]);
            dLpState[1] += dLpCoef * (d1 - dLpState[1]);

            const float x0 = w0[i];
            const float x1 = w1[i];
            w0[i] = x0 + d0 * dLvl * 0.6f;
            w1[i] = x1 + d1 * dLvl * 0.6f;

            dLine[0][(size_t) dIdx] = x0 + dLpState[0] * 0.35f;
            dLine[1][(size_t) dIdx] = x1 + dLpState[1] * 0.35f;

            dIdx = (dIdx + 1) % lineSize;
        }
    }

    // Saturador: mezcla entre la senal limpia y una version con tanh normalizada
    const float a = satAmt->load();
    if (a > 0.0001f)
    {
        const float k = 1.0f + 4.0f * a;
        const float norm = std::tanh (k);
        for (int ch = 0; ch < 2; ++ch)
        {
            float* w = buffer.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
                w[i] = w[i] * (1.0f - a) + a * std::tanh (k * w[i]) / norm;
        }
    }

    // Salida
    buffer.applyGain (0, 0, n, outGain->load());
    buffer.applyGain (1, 0, n, outGain->load());
    outLevel.store (juce::jmax (buffer.getMagnitude (0, 0, n), buffer.getMagnitude (1, 0, n)));

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
