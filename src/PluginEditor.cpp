#include "PluginEditor.h"

MortalEditor::MortalEditor (MortalProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1000, 930);

    // Columna izquierda
    addKnob ("position", "POSITION",   170, 120, 72);
    addKnob ("size",     "DURATION",   170, 238, 72);
    addKnob ("spray",    "SPRAY",      170, 356, 72);
    addKnob ("durrnd",   "DUR RANDOM", 170, 474, 60);

    // Columna derecha
    addKnob ("density",  "DENSITY",  830, 120, 72);
    addKnob ("pitch",    "PITCH",    830, 238, 72);
    addKnob ("feedback", "FEEDBACK", 830, 356, 72);

    // Centro
    addKnob ("mix", "MIX", 500, 356, 84);

    // Entrada y salida, en los bordes, con medidores a su lado
    addKnob ("ingain",  "IN",  72,  240, 60, true);
    addKnob ("outgain", "OUT", 928, 240, 60, true);
    inMeter.setBounds (12, 120, 16, 280);
    outMeter.setBounds (972, 120, 16, 280);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);

    // Botones centrales
    randomBtn.setButtonText ("RANDOMIZE");
    randomBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffb01c1c));
    randomBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff2b2b));
    randomBtn.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
    randomBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    randomBtn.setBounds (420, 446, 160, 24);
    addAndMakeVisible (randomBtn);
    randomBtn.onClick = [this] { randomizeAll(); };

    resetBtn.setButtonText ("RESET DEFAULT");
    resetBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff3a1a1a));
    resetBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6a2a2a));
    resetBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffffb3b3));
    resetBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    resetBtn.setBounds (420, 478, 160, 24);
    addAndMakeVisible (resetBtn);
    resetBtn.onClick = [this] { resetAll(); };

    // Faders de efectos: delay, reverb y saturacion
    addFader ("delay",  "DELAY",      700, 560, 130);
    addFader ("reverb", "REVERB",     800, 560, 130);
    addFader ("sat",    "SATURATION", 900, 560, 130);

    lowEqBtn.setButtonText ("LOW EQ");
    lowEqBtn.setClickingTogglesState (true);
    lowEqBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff3a1a1a));
    lowEqBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff2b2b));
    lowEqBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffffb3b3));
    lowEqBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    lowEqBtn.setBounds (770, 732, 60, 22);
    addAndMakeVisible (lowEqBtn);
    lowEqAttach = std::make_unique<ButtonAttachment> (processor.apvts, "loweq", lowEqBtn);

    // Seccion LFO: presets, dibujo y controles
    shapeLabel.setText ("LFO SHAPE (dibuixa aqui)", juce::dontSendNotification);
    shapeLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    shapeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    shapeLabel.setBounds (30, 540, 260, 14);
    addAndMakeVisible (shapeLabel);

    presetBox.addItemList ({ "Sine", "Triangle", "Square", "Saw Up", "Saw Down", "Steps" }, 1);
    presetBox.setTextWhenNothingSelected ("Preset...");
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a1212));
    presetBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffffb3b3));
    presetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff8c1111));
    presetBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffff2b2b));
    presetBox.setBounds (30, 560, 150, 24);
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0) processor.applyShapePreset (idx);
    };

    clearBtn.setButtonText ("CLEAR DRAW");
    clearBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff3a1a1a));
    clearBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6a2a2a));
    clearBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffffb3b3));
    clearBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    clearBtn.setBounds (190, 560, 120, 24);
    addAndMakeVisible (clearBtn);
    clearBtn.onClick = [this] { processor.clearShape(); presetBox.setSelectedId (0, juce::dontSendNotification); };

    shapeEditor.setBounds (30, 592, 440, 120);
    addAndMakeVisible (shapeEditor);

    addKnob ("lforate",  "LFO RATE",  530, 620, 60);
    addKnob ("lfodepth", "LFO DEPTH", 620, 620, 60);

    targetBox.addItemList ({ "Position", "Density", "Pitch", "Spray", "Mix" }, 1);
    targetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a1212));
    targetBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffffb3b3));
    targetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff8c1111));
    targetBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffff2b2b));
    targetBox.setBounds (480, 718, 150, 24);
    addAndMakeVisible (targetBox);
    targetAttach = std::make_unique<ComboAttachment> (processor.apvts, "lfotarget", targetBox);

    targetLabel.setText ("LFO TARGET", juce::dontSendNotification);
    targetLabel.setJustificationType (juce::Justification::centred);
    targetLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    targetLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    targetLabel.setBounds (480, 700, 150, 16);
    addAndMakeVisible (targetLabel);

    // Boton para mostrar u ocultar el panel DRUGS
    drugsBtn.setButtonText ("DRUGS");
    drugsBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff3a1a1a));
    drugsBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff2b2b));
    drugsBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffff2b2b));
    drugsBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    drugsBtn.setBounds (420, 780, 160, 24);
    addAndMakeVisible (drugsBtn);
    drugsBtn.onClick = [this]
    {
        drugsVisible = ! drugsVisible;
        for (auto* c : drugComps)
            c->setVisible (drugsVisible);
        repaint();
    };

    // Seccion DRUGS: tres knobs de 1 a 100 %, fuera del randomizador y ocultos al inicio
    addKnob ("lsd",      "LSD",      250, 840, 64, false, true);
    drugComps.add (sliders.getLast());
    drugComps.add (labels.getLast());
    addKnob ("ketamine", "KETAMINE", 500, 840, 64, false, true);
    drugComps.add (sliders.getLast());
    drugComps.add (labels.getLast());
    addKnob ("cocaine",  "COCAINE",  750, 840, 64, false, true);
    drugComps.add (sliders.getLast());
    drugComps.add (labels.getLast());

    for (auto* c : drugComps)
        c->setVisible (false);

    // Sincronia con el tempo del proyecto
    syncBtn.setButtonText ("SYNC");
    syncBtn.setClickingTogglesState (true);
    syncBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff3a1a1a));
    syncBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff2b2b));
    syncBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffffb3b3));
    syncBtn.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    syncBtn.setBounds (420, 510, 70, 24);
    addAndMakeVisible (syncBtn);
    syncAttach = std::make_unique<ButtonAttachment> (processor.apvts, "sync", syncBtn);

    divBox.addItemList (MortalProcessor::getDivisionNames(), 1);
    divBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a1212));
    divBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffffb3b3));
    divBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff8c1111));
    divBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffff2b2b));
    divBox.setBounds (500, 510, 80, 24);
    addAndMakeVisible (divBox);
    divAttach = std::make_unique<ComboAttachment> (processor.apvts, "division", divBox);

    bpmLabel.setJustificationType (juce::Justification::centred);
    bpmLabel.setFont (juce::Font (11.0f, juce::Font::plain));
    bpmLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    bpmLabel.setBounds (420, 538, 160, 14);
    addAndMakeVisible (bpmLabel);

    startTimerHz (30);
}

MortalEditor::~MortalEditor()
{
    stopTimer();
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
}

void MortalEditor::addKnob (const juce::String& paramID, const juce::String& text,
                            int cx, int cy, int size, bool isGain, bool locked)
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
        // Knobs: siempre de 1 % a 100 %
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

    if (isGain || locked)
        lockedSliders.add (s);

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
}

void MortalEditor::addFader (const juce::String& paramID, const juce::String& text,
                             int cx, int top, int height)
{
    auto* s = sliders.add (new juce::Slider());
    s->setSliderStyle (juce::Slider::LinearVertical);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    s->setColour (juce::Slider::textBoxTextColourId,       juce::Colour (0xffffb3b3));
    s->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s->setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);

    // Faders de 0 % a 100 %
    s->textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
    s->valueFromTextFunction = [] (const juce::String& t)
    {
        return t.retainCharacters ("0123456789.").getDoubleValue() / 100.0;
    };

    // Los efectos no se tocan con RANDOMIZE
    lockedSliders.add (s);

    s->setLookAndFeel (&laf);
    s->setBounds (cx - 30, top, 60, height + 18);
    addAndMakeVisible (s);

    auto* l = labels.add (new juce::Label ({}, text));
    l->setJustificationType (juce::Justification::centred);
    l->setFont (juce::Font (12.0f, juce::Font::bold));
    l->setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    l->setBounds (cx - 55, top + height + 20, 110, 16);
    addAndMakeVisible (l);

    attachments.push_back (std::make_unique<SliderAttachment> (processor.apvts, paramID, *s));
}

void MortalEditor::randomizeAll()
{
    auto& r = juce::Random::getSystemRandom();

    // Knobs: todos menos entrada, salida, efectos y DRUGS
    for (auto* s : sliders)
    {
        if (lockedSliders.contains (s)) continue;
        const auto range = s->getRange();
        s->setValue (range.getStart() + r.nextDouble() * (range.getEnd() - range.getStart()),
                     juce::sendNotificationSync);
    }

    targetBox.setSelectedItemIndex (r.nextInt (5), juce::sendNotificationSync);
    presetBox.setSelectedId (0, juce::dontSendNotification);

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
    presetBox.setSelectedId (1, juce::dontSendNotification); // Sine
}

void MortalEditor::drawDevil (juce::Graphics& g, juce::Point<float> c, float r, float act) const
{
    const juce::Colour red (0xffff2b2b);

    // Halo que reacciona a la actividad
    for (int i = 0; i < 5; ++i)
    {
        const float rr = r * 1.2f + (float) i * r * 0.12f;
        g.setColour (red.withAlpha ((0.16f - 0.025f * (float) i) * (0.4f + act)));
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f);
    }

    // Transformaciones: normal y espejo horizontal (para el lado derecho)
    const juce::AffineTransform T = juce::AffineTransform::translation (c.x, c.y);
    const juce::AffineTransform M (-1.0f, 0.0f, c.x, 0.0f, 1.0f, c.y);

    // Cuernos
    juce::Path horn;
    horn.startNewSubPath (-0.5f * r, -0.72f * r);
    horn.quadraticTo (-0.95f * r, -0.9f * r, -1.05f * r, -1.45f * r);
    horn.quadraticTo (-0.8f * r, -1.0f * r, -0.22f * r, -0.62f * r);
    horn.closeSubPath();

    g.setColour (juce::Colour (0xff3a0a0a));
    g.fillPath (horn, T);
    g.fillPath (horn, M);
    g.setColour (red.withAlpha (0.8f));
    g.strokePath (horn, juce::PathStrokeType (1.5f), T);
    g.strokePath (horn, juce::PathStrokeType (1.5f), M);

    // Cabeza
    juce::Path head;
    head.startNewSubPath (0.0f, -0.85f * r);
    head.cubicTo (0.55f * r, -0.95f * r, 0.95f * r, -0.55f * r, 0.9f * r, -0.05f * r);
    head.cubicTo (0.85f * r, 0.5f * r, 0.45f * r, 0.85f * r, 0.0f, 0.95f * r);
    head.cubicTo (-0.45f * r, 0.85f * r, -0.85f * r, 0.5f * r, -0.9f * r, -0.05f * r);
    head.cubicTo (-0.95f * r, -0.55f * r, -0.55f * r, -0.95f * r, 0.0f, -0.85f * r);
    head.closeSubPath();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a1414), c.x, c.y - r,
                                             juce::Colour (0xff2a0606), c.x, c.y + r, false));
    g.fillPath (head, T);
    g.setColour (juce::Colour (0xff120404));
    g.strokePath (head, juce::PathStrokeType (2.0f), T);

    // Cejas angulosas
    juce::Path brow;
    brow.startNewSubPath (-0.72f * r, -0.42f * r);
    brow.lineTo (-0.12f * r, -0.28f * r);
    g.setColour (juce::Colour (0xff1a0303));
    g.strokePath (brow, juce::PathStrokeType (0.09f * r), T);
    g.strokePath (brow, juce::PathStrokeType (0.09f * r), M);

    // Ojos: almendras brillantes que parpadean con la actividad
    juce::Path eye;
    eye.startNewSubPath (-0.6f * r, -0.3f * r);
    eye.quadraticTo (-0.4f * r, -0.45f * r, -0.15f * r, -0.15f * r);
    eye.quadraticTo (-0.4f * r, -0.2f * r, -0.6f * r, -0.3f * r);
    eye.closeSubPath();

    const float flicker = 0.85f + 0.15f * std::sin (phase * 5.0f);
    g.setColour (juce::Colour (0xffffa020).withAlpha (juce::jlimit (0.0f, 1.0f, (0.6f + 0.4f * act) * flicker)));
    g.fillPath (eye, T);
    g.fillPath (eye, M);
    g.setColour (juce::Colour (0xffffe080).withAlpha (0.4f + 0.6f * act));
    g.strokePath (eye, juce::PathStrokeType (1.0f), T);
    g.strokePath (eye, juce::PathStrokeType (1.0f), M);

    // Boca con colmillos
    juce::Path mouth;
    mouth.startNewSubPath (-0.5f * r, 0.35f * r);
    mouth.quadraticTo (0.0f, 0.7f * r, 0.5f * r, 0.35f * r);
    g.setColour (juce::Colour (0xff120202));
    g.strokePath (mouth, juce::PathStrokeType (0.05f * r), T);

    juce::Path fang;
    fang.startNewSubPath (-0.36f * r, 0.5f * r);
    fang.lineTo (-0.27f * r, 0.76f * r);
    fang.lineTo (-0.17f * r, 0.56f * r);
    fang.closeSubPath();
    g.setColour (juce::Colour (0xfffff0e0));
    g.fillPath (fang, T);
    g.fillPath (fang, M);
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

    // Textos de la esquina superior derecha
    g.setColour (juce::Colour (0xffb06060));
    g.setFont (juce::Font (11.0f, juce::Font::plain));
    g.drawText ("alabeu al hippie mal vestit",
                juce::Rectangle<int> (480, 10, getWidth() - 490, 14), juce::Justification::right);
    g.drawText ("distribuiu aquest plugin amb seny, que de cervell no en venen",
                juce::Rectangle<int> (480, 26, getWidth() - 490, 14), juce::Justification::right);

    // Texto de la esquina inferior derecha
    g.drawText ("Ningu es fa responsable d'aquest plugin",
                juce::Rectangle<int> (480, 912, getWidth() - 490, 14), juce::Justification::right);

    const float act = juce::jlimit (0.0f, 1.0f, processor.getActivity() * 1.5f);
    const auto centre = devilArea.toFloat().getCentre();
    drawDevil (g, centre, 80.0f, act);

    // Panel DRUGS (solo cuando esta desplegado)
    if (drugsVisible)
    {
        auto panel = juce::Rectangle<float> (20.0f, 774.0f, 960.0f, 150.0f);
        g.setColour (juce::Colour (0x66100505));
        g.fillRoundedRectangle (panel, 10.0f);
        g.setColour (juce::Colour (0xff6a2a2a));
        g.drawRoundedRectangle (panel, 10.0f, 1.0f);
    }
}
