#include "PluginEditor.h"

MortalEditor::MortalEditor (MortalProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (760, 730);

    auto& rnd = juce::Random::getSystemRandom();
    for (auto& s : seeds) s = rnd.nextFloat();

    // Fila superior
    addKnob ("position", "POSITION", 110, 120, 72);
    addKnob ("density",  "DENSITY",  650, 120, 72);

    // Fila media
    addKnob ("size",     "DURATION", 110, 238, 72);
    addKnob ("pitch",    "PITCH",    650, 238, 72);

    // Fila inferior de columnas laterales
    addKnob ("spray",    "SPRAY",    110, 356, 72);
    addKnob ("feedback", "FEEDBACK", 650, 356, 72);
    addKnob ("durrnd",   "DUR RANDOM", 110, 474, 60);

    // Centro
    addKnob ("mix", "MIX", 380, 356, 84);

    // Entrada y salida, en los bordes laterales
    addKnob ("ingain",  "IN",  44, 238, 60, true);
    addKnob ("outgain", "OUT", 716, 238, 60, true);

    // Botones centrales
    randomBtn.setButtonText ("RANDOMIZE");
    randomBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffb01c1c));
    randomBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff2b2b));
    randomBtn.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
    randomBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    randomBtn.setBounds (300, 446, 160, 24);
    addAndMakeVisible (randomBtn);
    randomBtn.onClick = [this] { randomizeAll(); };

    resetBtn.setButtonText ("RESET DEFAULT");
    resetBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff3a1a1a));
    resetBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6a2a2a));
    resetBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffffb3b3));
    resetBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    resetBtn.setBounds (300, 478, 160, 24);
    addAndMakeVisible (resetBtn);
    resetBtn.onClick = [this] { resetAll(); };

    // Seccion LFO: forma dibujada + controles
    shapeLabel.setText ("LFO SHAPE (dibuja aqui)", juce::dontSendNotification);
    shapeLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    shapeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    shapeLabel.setBounds (30, 548, 260, 14);
    addAndMakeVisible (shapeLabel);

    shapeEditor.setBounds (30, 566, 430, 120);
    addAndMakeVisible (shapeEditor);

    addKnob ("lforate",  "LFO RATE",  520, 600, 60);
    addKnob ("lfodepth", "LFO DEPTH", 620, 600, 60);

    targetBox.addItemList ({ "Position", "Density", "Pitch", "Spray", "Mix" }, 1);
    targetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a1212));
    targetBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffffb3b3));
    targetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff8c1111));
    targetBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffff2b2b));
    targetBox.setBounds (480, 690, 160, 24);
    addAndMakeVisible (targetBox);
    targetAttach = std::make_unique<ComboAttachment> (processor.apvts, "lfotarget", targetBox);

    targetLabel.setText ("LFO TARGET", juce::dontSendNotification);
    targetLabel.setJustificationType (juce::Justification::centred);
    targetLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    targetLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    targetLabel.setBounds (480, 672, 160, 16);
    addAndMakeVisible (targetLabel);

    startTimerHz (30);
}

MortalEditor::~MortalEditor()
{
    stopTimer();
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
}

void MortalEditor::addKnob (const juce::String& paramID, const juce::String& text,
                            int cx, int cy, int size, bool isGain)
{
    auto* s = sliders.add (new juce::Slider());
    s->setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, size, 16);
    s->setColour (juce::Slider::textBoxTextColourId,       juce::Colour (0xffffb3b3));
    s->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s->setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);

    if (isGain)
    {
        // Entrada/salida: 0 % a 200 %, con 100 % como unidad
        s->textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
        s->valueFromTextFunction = [] (const juce::String& t)
        {
            return t.retainCharacters ("0123456789.").getDoubleValue() / 100.0;
        };
    }
    else
    {
        // Resto de knobs: siempre de 1 % a 100 %
        s->textFromValueFunction = [s] (double v)
        {
            const double proportion = s->valueToProportionOfLength (v);
            return juce::String (juce::roundToInt (proportion * 99.0) + 1) + " %";
        };
        s->valueFromTextFunction = [s] (const juce::String& t)
        {
            const double pct = juce::jlimit (1.0, 100.0, t.retainCharacters ("0123456789.").getDoubleValue());
            return s->proportionOfLengthToValue ((pct - 1.0) / 99.0);
        };
    }

    s->setLookAndFeel (&laf);
    s->setBounds (cx - size / 2, cy - size / 2, size, size + 18);
    addAndMakeVisible (s);

    auto* l = labels.add (new juce::Label ({}, text));
    l->setJustificationType (juce::Justification::centred);
    l->setFont (juce::Font (12.0f, juce::Font::bold));
    l->setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    l->setBounds (cx - 55, cy + size / 2 + 20, 110, 16);
    addAndMakeVisible (l);

    attachments.push_back (std::make_unique<SliderAttachment> (processor.apvts, paramID, *s));

    if (paramID == "ingain")  inSlider = s;
    if (paramID == "outgain") outSlider = s;
}

void MortalEditor::randomizeAll()
{
    auto& r = juce::Random::getSystemRandom();

    // Knobs: todos menos la entrada y la salida, que no se tocan
    for (auto* s : sliders)
    {
        if (s == inSlider || s == outSlider) continue;
        const auto range = s->getRange();
        s->setValue (range.getStart() + r.nextDouble() * (range.getEnd() - range.getStart()),
                     juce::sendNotificationSync);
    }

    targetBox.setSelectedItemIndex (r.nextInt (5), juce::sendNotificationSync);

    // Forma del LFO: 9 puntos aleatorios unidos por interpolacion
    float pts[9];
    for (auto& v : pts) v = r.nextFloat();
    for (int i = 0; i < MortalProcessor::shapeSize; ++i)
    {
        const float x = (float) i / (float) (MortalProcessor::shapeSize - 1) * 8.0f;
        const int k0 = juce::jmin (7, (int) x);
        const float t = x - (float) k0;
        processor.setShapeValue (i, pts[k0] + (pts[k0 + 1] - pts[k0]) * t);
    }
}

void MortalEditor::resetAll()
{
    // Vuelve cada parametro de la API a su valor por defecto
    for (auto* p : processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    processor.resetShape();
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
