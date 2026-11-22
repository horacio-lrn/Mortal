#include "PluginEditor.h"

MortalEditor::MortalEditor (MortalProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (760, 580);

    auto& rnd = juce::Random::getSystemRandom();
    for (auto& s : seeds) s = rnd.nextFloat();

    // Fila superior
    addKnob ("position", "POSITION", 110, 120, 72, "");
    addKnob ("density",  "DENSITY",  650, 120, 72, "");

    // Fila media
    addKnob ("size",     "DURATION", 110, 238, 72, " ms");
    addKnob ("pitch",    "PITCH",    650, 238, 72, " st");

    // Fila inferior izquierda / derecha
    addKnob ("spray",    "SPRAY",    110, 356, 72, "");
    addKnob ("feedback", "FEEDBACK", 650, 356, 72, "");

    // Centro, debajo de la esfera
    addKnob ("mix",      "MIX",      380, 356, 84, "");

    // Panel inferior: duracion aleatoria y LFO
    addKnob ("durrnd",   "DUR RANDOM", 200, 480, 72, "");
    addKnob ("lforate",  "LFO RATE",   320, 480, 72, " Hz");
    addKnob ("lfodepth", "LFO DEPTH",  440, 480, 72, "");

    targetBox.addItemList ({ "Position", "Density", "Pitch", "Spray", "Mix" }, 1);
    targetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a1212));
    targetBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffffb3b3));
    targetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff8c1111));
    targetBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffff2b2b));
    targetBox.setBounds (500, 468, 120, 24);
    addAndMakeVisible (targetBox);
    targetAttach = std::make_unique<ComboAttachment> (processor.apvts, "lfotarget", targetBox);

    targetLabel.setText ("LFO TARGET", juce::dontSendNotification);
    targetLabel.setJustificationType (juce::Justification::centred);
    targetLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    targetLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    targetLabel.setBounds (500, 446, 120, 16);
    addAndMakeVisible (targetLabel);

    randomBtn.setButtonText ("RANDOMIZE");
    randomBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffb01c1c));
    randomBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff2b2b));
    randomBtn.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
    randomBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    randomBtn.setBounds (640, 468, 100, 24);
    addAndMakeVisible (randomBtn);
    randomBtn.onClick = [this]
    {
        auto& r = juce::Random::getSystemRandom();
        for (auto* s : sliders)
        {
            const auto range = s->getRange();
            s->setValue (range.getStart() + r.nextDouble() * (range.getEnd() - range.getStart()),
                         juce::sendNotificationSync);
        }
        targetBox.setSelectedItemIndex (r.nextInt (5), juce::sendNotificationSync);
    };

    startTimerHz (30);
}

MortalEditor::~MortalEditor()
{
    stopTimer();
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
}

void MortalEditor::addKnob (const juce::String& paramID, const juce::String& text,
                            int cx, int cy, int size, const juce::String& suffix)
{
    auto* s = sliders.add (new juce::Slider());
    s->setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, size, 16);
    s->setTextValueSuffix (suffix);
    s->setColour (juce::Slider::textBoxTextColourId,       juce::Colour (0xffffb3b3));
    s->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s->setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    s->setLookAndFeel (&laf);
    s->setBounds (cx - size / 2, cy - size / 2, size, size + 18);
    addAndMakeVisible (s);

    auto* l = labels.add (new juce::Label ({}, text));
    l->setJustificationType (juce::Justification::centred);
    l->setFont (juce::Font (12.0f, juce::Font::bold));
    l->setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    l->setBounds (cx - 60, cy + size / 2 + 20, 120, 16);
    addAndMakeVisible (l);

    attachments.push_back (std::make_unique<SliderAttachment> (processor.apvts, paramID, *s));
}

void MortalEditor::drawSphere (juce::Graphics& g, juce::Point<float> c, float r, float act) const
{
    const juce::Colour hot (0xffff2b2b);

    for (int i = 0; i < 5; ++i)
    {
        const float rr = r + 6.0f + (float) i * 6.0f;
        g.setColour (hot.withAlpha ((0.14f - 0.024f * (float) i) * (0.4f + act)));
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f);
    }

    juce::Path clip;
    clip.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    g.saveState();
    g.reduceClipRegion (clip);

    juce::ColourGradient base (juce::Colour (0xffff2b2b).withAlpha (0.6f + 0.4f * act), c.x, c.y,
                               juce::Colour (0xff2a0c0c), c.x, c.y - r, true);
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

        g.setColour (juce::Colour (0xffff7070).withAlpha (0.35f + 0.5f * act));
        g.strokePath (crack, juce::PathStrokeType (1.2f + 1.5f * act));
    }

    g.restoreState();

    g.setColour (juce::Colour (0xff120404));
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 2.0f);
}

void MortalEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bg (juce::Colour (0xff2e1818), 0.0f, 0.0f,
                             juce::Colour (0xff140909), 0.0f, bounds.getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colour (0xffff2b2b));
    g.setFont (juce::Font (32.0f, juce::Font::plain));
    g.drawText ("MORTAL", juce::Rectangle<int> (0, 12, getWidth(), 44), juce::Justification::centred);

    g.setColour (juce::Colour (0xffb06060));
    g.setFont (juce::Font (12.0f, juce::Font::plain));
    g.drawText ("GRANULAR FX", juce::Rectangle<int> (0, 58, getWidth(), 16), juce::Justification::centred);

    const float act = juce::jlimit (0.0f, 1.0f, processor.getActivity() * 1.5f);
    const auto centre = sphereArea.toFloat().getCentre();
    drawSphere (g, centre, 92.0f, act);
}
