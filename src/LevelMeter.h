#pragma once
#include <JuceHeader.h>
#include <atomic>

// Medidor vertical en dB (de -60 a 0) con caida suave.
class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (const std::atomic<float>& source) : src (source) {}

    // Llamar desde el timer del editor
    void tick()
    {
        shown = juce::jmax (src.load(), shown * 0.92f);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff1a0c0c));
        g.fillRoundedRectangle (b, 3.0f);

        const float db = juce::Decibels::gainToDecibels (shown, -60.0f);
        const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);

        auto bar = b.reduced (2.0f);
        const float h = bar.getHeight() * norm;
        juce::Rectangle<float> fillR (bar.getX(), bar.getBottom() - h, bar.getWidth(), h);

        juce::ColourGradient grad (juce::Colour (0xffff2b2b), 0.0f, bar.getBottom(),
                                   juce::Colour (0xffff9a9a), 0.0f, bar.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fillR, 2.0f);

        g.setColour (juce::Colour (0xff6a2a2a));
        g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 1.0f);
    }

private:
    const std::atomic<float>& src;
    float shown = 0.0f;
};
