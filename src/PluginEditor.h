#pragma once
#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"

class MortalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        const juce::Colour red (0xffff2b2b);
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
        const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto c = bounds.getCentre();
        const float r = d * 0.5f;
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc (c.x, c.y, r + 4.0f, r + 4.0f, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff1a0d0d));
        g.strokePath (track, juce::PathStrokeType (3.0f));

        juce::Path value;
        value.addCentredArc (c.x, c.y, r + 4.0f, r + 4.0f, 0.0f, startAngle, angle, true);
        g.setColour (red.withAlpha (0.30f));
        g.strokePath (value, juce::PathStrokeType (7.0f));
        g.setColour (red);
        g.strokePath (value, juce::PathStrokeType (3.0f));

        juce::ColourGradient body (juce::Colour (0xff4a2424), c.x, c.y - r,
                                   juce::Colour (0xff1a0c0c), c.x, c.y + r, false);
        g.setGradientFill (body);
        g.fillEllipse (c.x - r, c.y - r, d, d);
        g.setColour (juce::Colour (0xff0a0303));
        g.drawEllipse (c.x - r, c.y - r, d, d, 1.5f);

        const float dotR = r * 0.78f;
        const float dx = c.x + std::sin (angle) * dotR;
        const float dy = c.y - std::cos (angle) * dotR;
        g.setColour (red.withAlpha (0.35f));
        g.fillEllipse (dx - 6.0f, dy - 6.0f, 12.0f, 12.0f);
        g.setColour (red);
        g.fillEllipse (dx - 3.0f, dy - 3.0f, 6.0f, 6.0f);
    }
};

class MortalEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MortalEditor (MortalProcessor&);
    ~MortalEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override { phase += 0.03f; repaint (sphereArea); }
    void addKnob (const juce::String& paramID, const juce::String& text,
                  int cx, int cy, int size, const juce::String& suffix);
    void drawSphere (juce::Graphics& g, juce::Point<float> centre, float radius, float act) const;

    MortalProcessor& processor;
    MortalLookAndFeel laf;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    juce::ComboBox targetBox;
    juce::Label targetLabel;
    juce::TextButton randomBtn;
    std::unique_ptr<ComboAttachment> targetAttach;

    juce::Rectangle<int> sphereArea { 280, 100, 200, 200 };
    std::array<float, 40> seeds {};
    float phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MortalEditor)
};
