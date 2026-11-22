#include "PluginEditor.h"

MortalEditor::MortalEditor (MortalProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (760, 440);

    auto& rnd = juce::Random::getSystemRandom();
    for (auto& s : seeds) s = rnd.nextFloat();

    addKnob ("position", "POSITION", 110, 130, 84);
    addKnob ("size",     "SIZE",     110, 240, 84);
    addKnob ("spray",    "SPRAY",    110, 350, 84);

    addKnob ("density",  "DENSITY",  650, 130, 84);
    addKnob ("pitch",    "PITCH",    650, 240, 84);
    addKnob ("feedback", "FEEDBACK", 650, 350, 84);

    addKnob ("mix",      "MIX",      380, 350, 96);

    startTimerHz (30);
}

MortalEditor::~MortalEditor()
{
    stopTimer();
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
}

void MortalEditor::addKnob (const juce::String& paramID, const juce::String& text, int cx, int cy, int size)
{
    auto* s = sliders.add (new juce::Slider());
    s->setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s->setLookAndFeel (&laf);
    s->setBounds (cx - size / 2, cy - size / 2, size, size);
    addAndMakeVisible (s);

    auto* l = labels.add (new juce::Label ({}, text));
    l->setJustificationType (juce::Justification::centred);
    l->setFont (juce::Font (12.0f, juce::Font::bold));
    l->setColour (juce::Label::textColourId, juce::Colour (0xff9aa0a8));
    l->setBounds (cx - 60, cy + size / 2 + 2, 120, 18);
    addAndMakeVisible (l);

    attachments.push_back (std::make_unique<SliderAttachment> (processor.apvts, paramID, *s));
}

void MortalEditor::drawSphere (juce::Graphics& g, juce::Point<float> c, float r, float act) const
{
    const juce::Colour hot (0xffff7a1a);

    for (int i = 0; i < 5; ++i)
    {
        const float rr = r + 6.0f + (float) i * 6.0f;
        g.setColour (hot.withAlpha ((0.10f - 0.018f * (float) i) * (0.4f + act)));
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f);
    }

    juce::Path clip;
    clip.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    g.saveState();
    g.reduceClipRegion (clip);

    juce::ColourGradient base (juce::Colour (0xffff8a2a).withAlpha (0.6f + 0.4f * act), c.x, c.y,
                               juce::Colour (0xff3b3b3e), c.x, c.y - r, true);
    g.setGradientFill (base);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    for (int k = 0; k < (int) seeds.size(); ++k)
    {
        const float a = seeds[(size_t) k] * juce::MathConstants<float>::twoPi + std::sin (phase * 0.7f + (float) k) * 0.15f;
        const float len = r * (0.3f + 0.7f * seeds[(size_t) ((k + 7) % (int) seeds.size())]);

        juce::Path crack;
        crack.startNewSubPath (c.x + std::cos (a) * r * 0.1f, c.y + std::sin (a) * r * 0.1f);
        crack.quadraticTo (c.x + std::cos (a + 0.3f) * len * 0.5f, c.y + std::sin (a + 0.3f) * len * 0.5f,
                           c.x + std::cos (a + 0.1f) * len, c.y + std::sin (a + 0.1f) * len);

        g.setColour (juce::Colour (0xffffc857).withAlpha (0.35f + 0.5f * act));
        g.strokePath (crack, juce::PathStrokeType (1.2f + 1.5f * act));
    }

    g.restoreState();

    g.setColour (juce::Colour (0xff111214));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 2.0f);
}

void MortalEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bg (juce::Colour (0xff34373d), 0.0f, 0.0f,
                             juce::Colour (0xff1c1d21), 0.0f, bounds.getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colour (0xffc4c9d1));
    g.setFont (juce::Font (32.0f, juce::Font::plain));
    g.drawText ("MORTAL", juce::Rectangle<int> (0, 12, getWidth(), 44), juce::Justification::centred);

    g.setColour (juce::Colour (0xff8b9099));
    g.setFont (juce::Font (12.0f, juce::Font::plain));
    g.drawText ("GRANULAR FX", juce::Rectangle<int> (0, 58, getWidth(), 16), juce::Justification::centred);

    const float act = juce::jlimit (0.0f, 1.0f, processor.getActivity() * 1.5f);
    const auto centre = sphereArea.toFloat().getCentre();
    drawSphere (g, centre, 92.0f, act);
}
