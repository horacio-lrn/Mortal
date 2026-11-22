#pragma once
#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"
#include "LfoShapeEditor.h"
#include "LevelMeter.h"

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

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0.0f, 0.0f, style, slider);
            return;
        }

        const juce::Colour red (0xffff2b2b);
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y;
        const float bottom = (float) (y + h);

        // Pista
        juce::Rectangle<float> track (cx - 3.0f, top, 6.0f, bottom - top);
        g.setColour (juce::Colour (0xff1a0d0d));
        g.fillRoundedRectangle (track, 3.0f);

        // Relleno desde abajo hasta el pulgar
        juce::Rectangle<float> fill (cx - 3.0f, sliderPos, 6.0f, bottom - sliderPos);
        g.setColour (red.withAlpha (0.45f));
        g.fillRoundedRectangle (fill, 3.0f);

        // Pulgar
        juce::Rectangle<float> thumb (cx - 12.0f, sliderPos - 6.0f, 24.0f, 12.0f);
        g.setColour (juce::Colour (0xff3a1818));
        g.fillRoundedRectangle (thumb, 3.0f);
        g.setColour (red);
        g.drawRoundedRectangle (thumb, 3.0f, 1.5f);
        g.drawHorizontalLine ((int) sliderPos, cx - 8.0f, cx + 8.0f);
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
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override
    {
        phase += 0.03f;
        repaint (sphereArea);
        shapeEditor.repaint();
        inMeter.tick();
        outMeter.tick();
    }

    void addKnob (const juce::String& paramID, const juce::String& text,
                  int cx, int cy, int size, bool isGain = false);
    void addFader (const juce::String& paramID, const juce::String& text,
                   int cx, int top, int height);
    void drawSphere (juce::Graphics& g, juce::Point<float> centre, float radius, float act) const;
    void randomizeAll();
    void resetAll();

    MortalProcessor& processor;
    MortalLookAndFeel laf;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    juce::Slider* inSlider = nullptr;
    juce::Slider* outSlider = nullptr;

    juce::ComboBox targetBox;
    juce::Label targetLabel;
    juce::Label shapeLabel;
    juce::ComboBox presetBox;
    juce::TextButton clearBtn;
    juce::TextButton randomBtn;
    juce::TextButton resetBtn;
    juce::TextButton lowEqBtn;
    std::unique_ptr<ComboAttachment> targetAttach;
    std::unique_ptr<ButtonAttachment> lowEqAttach;

    LevelMeter inMeter { processor.getInputLevel() };
    LevelMeter outMeter { processor.getOutputLevel() };
    LfoShapeEditor shapeEditor { processor };

    juce::Rectangle<int> sphereArea { 350, 100, 200, 200 };
    std::array<float, 40> seeds {};
    float phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MortalEditor)
};
