#include "PluginEditor.h"

namespace
{
    // Parametros de division: LFO, DELAY, GRAIN, LSD, KETAMINE, COCAINE
    const char* const divIds[7] = { "lfodiv", "delaydiv", "grainsdiv", "lsddiv", "freezediv", "stutdiv", "flangediv" };

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
    {
        g.setColour (juce::Colour (0x66100505));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colour (0xff5a2222));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

        if (title.isNotEmpty())
        {
            g.setColour (juce::Colour (0xffff2b2b));
            g.setFont (juce::Font (12.0f, juce::Font::bold));
            g.drawText (title, juce::Rectangle<float> (r.getX() + 12.0f, r.getY() + 6.0f, 200.0f, 16.0f),
                        juce::Justification::left);
        }
    }

    void styleButton (juce::TextButton& b, bool primary)
    {
        b.setColour (juce::TextButton::buttonColourId,   juce::Colour (primary ? 0xffb01c1c : 0xff3a1a1a));
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (primary ? 0xffff2b2b : 0xff6a2a2a));
        b.setColour (juce::TextButton::textColourOffId,  juce::Colour (primary ? 0xffffffff : 0xffffb3b3));
        b.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    }

    void styleCombo (juce::ComboBox& c)
    {
        c.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a1212));
        c.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffffb3b3));
        c.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff8c1111));
        c.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffff2b2b));
    }
}

MortalEditor::MortalEditor (MortalProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1000, 830);
    syncRaw = processor.apvts.getRawParameterValue ("sync");

    // Entrada y salida, en los bordes
    addKnob ("ingain",  "IN",  66,  110, 60, -1, true);
    addKnob ("outgain", "OUT", 930, 110, 60, -1, true);
    inMeter.setBounds (12, 110, 16, 410);
    outMeter.setBounds (972, 110, 16, 410);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);

    // Panel GRAIN (izquierda)
    addKnob ("position", "POSITION",   180, 130, 68);
    addKnob ("size",     "DURATION",   300, 130, 68, 2);
    addKnob ("spray",    "SPRAY",      180, 260, 68);
    addKnob ("durrnd",   "DUR RANDOM", 300, 260, 68);
    addKnob ("density",  "DENSITY",    180, 390, 68);
    addKnob ("pitch",    "PITCH",      300, 390, 68);

    // Panel central: devil, MIX y botones
    addKnob ("mix", "MIX", 500, 300, 84);

    randomBtn.setButtonText ("RANDOMIZE");
    styleButton (randomBtn, true);
    randomBtn.setBounds (400, 428, 200, 26);
    addAndMakeVisible (randomBtn);
    randomBtn.onClick = [this] { randomizeAll(); };

    resetBtn.setButtonText ("RESET DEFAULT");
    styleButton (resetBtn, false);
    resetBtn.setBounds (400, 460, 200, 26);
    addAndMakeVisible (resetBtn);
    resetBtn.onClick = [this] { resetAll(); };

    syncBtn.setButtonText ("SYNC");
    syncBtn.setClickingTogglesState (true);
    styleButton (syncBtn, false);
    syncBtn.setBounds (400, 494, 96, 24);
    addAndMakeVisible (syncBtn);
    syncAttach = std::make_unique<ButtonAttachment> (processor.apvts, "sync", syncBtn);
    syncBtn.onClick = [this] { updateVisibility(); };

    bpmLabel.setJustificationType (juce::Justification::left);
    bpmLabel.setFont (juce::Font (11.0f, juce::Font::plain));
    bpmLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    bpmLabel.setBounds (504, 497, 100, 18);
    addAndMakeVisible (bpmLabel);

    // Panel FX (derecha)
    addKnob ("feedback", "FEEDBACK", 700, 130, 68);
    addKnob ("flanger",  "FLANGER",  840, 130, 68, 6, false, false, true);
    addFader ("delay",  "DELAY",      680, 296, 100);
    addDivBox (1, 680, 436, false);
    addFader ("reverb", "REVERB",     760, 296, 100);
    addFader ("sat",    "SATURATION", 840, 296, 100);

    lowEqBtn.setButtonText ("LOW EQ");
    lowEqBtn.setClickingTogglesState (true);
    styleButton (lowEqBtn, false);
    lowEqBtn.setBounds (720, 468, 80, 22);
    addAndMakeVisible (lowEqBtn);
    lowEqAttach = std::make_unique<ButtonAttachment> (processor.apvts, "loweq", lowEqBtn);

    // Panel LFO (abajo izquierda)
    presetBox.addItemList ({ "Sine", "Triangle", "Square", "Saw Up", "Saw Down", "Steps" }, 1);
    presetBox.setTextWhenNothingSelected ("Preset...");
    styleCombo (presetBox);
    presetBox.setBounds (126, 584, 130, 24);
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0) processor.applyShapePreset (idx);
    };

    clearBtn.setButtonText ("CLEAR DRAW");
    styleButton (clearBtn, false);
    clearBtn.setBounds (266, 584, 110, 24);
    addAndMakeVisible (clearBtn);
    clearBtn.onClick = [this] { processor.clearShape(); presetBox.setSelectedId (0, juce::dontSendNotification); };

    targetLabel.setText ("LFO TARGET", juce::dontSendNotification);
    targetLabel.setJustificationType (juce::Justification::centred);
    targetLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    targetLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    targetLabel.setBounds (400, 566, 120, 14);
    addAndMakeVisible (targetLabel);

    targetBox.addItemList ({ "Position", "Density", "Pitch", "Spray", "Mix" }, 1);
    styleCombo (targetBox);
    targetBox.setBounds (400, 584, 120, 24);
    addAndMakeVisible (targetBox);
    targetAttach = std::make_unique<ComboAttachment> (processor.apvts, "lfotarget", targetBox);

    shapeLabel.setText ("LFO SHAPE (dibuixa aqui)", juce::dontSendNotification);
    shapeLabel.setFont (juce::Font (11.0f, juce::Font::plain));
    shapeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    shapeLabel.setBounds (126, 604, 330, 14);
    addAndMakeVisible (shapeLabel);

    shapeEditor.setBounds (126, 622, 330, 130);
    addAndMakeVisible (shapeEditor);

    addKnob ("lforate",  "LFO RATE",  520, 620, 56, 0);
    addKnob ("lfodepth", "LFO DEPTH", 600, 620, 56);

    // Panel DRUGS (abajo derecha, plegable)
    drugsBtn.setButtonText ("DRUGS  [+]");
    styleButton (drugsBtn, true);
    drugsBtn.setBounds (664, 562, 302, 26);
    addAndMakeVisible (drugsBtn);
    drugsBtn.onClick = [this]
    {
        drugsOpen = ! drugsOpen;
        drugsBtn.setButtonText (drugsOpen ? "DRUGS  [-]" : "DRUGS  [+]");
        updateVisibility();
        repaint();
    };

    addKnob ("lsd",      "LSD",      730, 620, 60, 3, false, true);
    addKnob ("ketamine", "KETAMINE", 815, 620, 60, 4, false, true);
    addKnob ("cocaine",  "COCAINE",  900, 620, 60, 5, false, true);

    updateVisibility();
    startTimerHz (30);
}

MortalEditor::~MortalEditor()
{
    stopTimer();
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
}

void MortalEditor::timerCallback()
{
    phase += 0.03f;
    repaint (juce::Rectangle<int> (380, 100, 240, 230));
    shapeEditor.repaint();
    inMeter.tick();
    outMeter.tick();
    bpmLabel.setText (juce::String (processor.getBpm(), 1) + " BPM", juce::dontSendNotification);
    updateVisibility();
}

// Con SYNC activo: aparecen las divisiones y se atenuan los knobs de tiempo.
// Con SYNC apagado: solo se ven los knobs, como antes.
void MortalEditor::updateVisibility()
{
    const bool sync = syncRaw->load() > 0.5f;

    for (auto* c : drugComps)
        c->setVisible (drugsOpen);

    for (auto* b : divBoxes)
    {
        if (alwaysDivs.contains (b))    b->setVisible (true);
        else if (drugDivs.contains (b)) b->setVisible (drugsOpen);
        else                            b->setVisible (sync);
    }

    for (auto* s : timedSliders)
        s->setEnabled (! sync);
}

void MortalEditor::addKnob (const juce::String& paramID, const juce::String& text,
                            int cx, int top, int size, int divIndex, bool isGain, bool isDrug, bool alwaysDiv)
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

    if (isGain || isDrug || alwaysDiv) lockedSliders.add (s);
    if (divIndex >= 0 && ! isDrug && ! alwaysDiv) timedSliders.add (s);

    s->setLookAndFeel (&laf);
    s->setBounds (cx - size / 2, top, size, size + 18);
    addAndMakeVisible (s);

    auto* l = labels.add (new juce::Label ({}, text));
    l->setJustificationType (juce::Justification::centred);
    l->setFont (juce::Font (11.0f, juce::Font::bold));
    l->setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    l->setBounds (cx - 38, top + size + 20, 76, 16);
    addAndMakeVisible (l);

    attachments.push_back (std::make_unique<SliderAttachment> (processor.apvts, paramID, *s));

    if (divIndex >= 0)
        if (alwaysDiv) alwaysDivs.add (addDivBox (divIndex, cx, top + size + 38, isDrug));
        else           addDivBox (divIndex, cx, top + size + 38, isDrug);

    if (isDrug)
    {
        drugComps.add (s);
        drugComps.add (l);
    }
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
    l->setFont (juce::Font (11.0f, juce::Font::bold));
    l->setColour (juce::Label::textColourId, juce::Colour (0xffd0a0a0));
    l->setBounds (cx - 38, top + height + 20, 76, 16);
    addAndMakeVisible (l);

    attachments.push_back (std::make_unique<SliderAttachment> (processor.apvts, paramID, *s));
}

juce::ComboBox* MortalEditor::addDivBox (int divIndex, int cx, int y, bool isDrug)
{
    auto* box = divBoxes.add (new juce::ComboBox());
    box->addItemList (MortalProcessor::getDivisionNames(), 1);
    styleCombo (*box);
    box->setBounds (cx - 45, y, 90, 22);
    addAndMakeVisible (box);
    divAttachments.push_back (std::make_unique<ComboAttachment> (processor.apvts, divIds[divIndex], *box));

    if (isDrug)
        drugDivs.add (box);

    return box;
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
    // Fondo
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e1818), 0.0f, 0.0f,
                                             juce::Colour (0xff140909), 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Cabecera
    g.setColour (juce::Colour (0xffff2b2b));
    g.setFont (juce::Font (30.0f, juce::Font::plain));
    g.drawText ("MORTAL", juce::Rectangle<int> (0, 12, getWidth(), 36), juce::Justification::centred);

    g.setColour (juce::Colour (0xffb06060));
    g.setFont (juce::Font (11.0f, juce::Font::plain));
    g.drawText ("GRANULAR FX", juce::Rectangle<int> (0, 50, getWidth(), 14), juce::Justification::centred);

    g.drawText ("alabeu al hippie mal vestit",
                juce::Rectangle<int> (480, 10, getWidth() - 490, 14), juce::Justification::right);
    g.drawText ("distribuiu aquest plugin amb seny, que de cervell no en venen",
                juce::Rectangle<int> (480, 26, getWidth() - 490, 14), juce::Justification::right);

    g.setColour (juce::Colour (0xff5a2222));
    g.drawHorizontalLine (74, 20.0f, 980.0f);

    // Paneles
    drawPanel (g, { 110.0f, 90.0f, 260.0f, 450.0f }, "GRAIN");
    drawPanel (g, { 380.0f, 90.0f, 240.0f, 450.0f }, "");
    drawPanel (g, { 630.0f, 90.0f, 260.0f, 450.0f }, "FX");
    drawPanel (g, { 110.0f, 556.0f, 530.0f, 234.0f }, "LFO");
    drawPanel (g, { 650.0f, 556.0f, 330.0f, drugsOpen ? 234.0f : 40.0f }, "");

    // Devil
    const float act = juce::jlimit (0.0f, 1.0f, processor.getActivity() * 1.5f);
    drawDevil (g, { 500.0f, 200.0f }, 62.0f, act);

    // Pie
    g.setColour (juce::Colour (0xffb06060));
    g.setFont (juce::Font (11.0f, juce::Font::plain));
    g.drawText ("Ningu es fa responsable d'aquest plugin",
                juce::Rectangle<int> (480, 806, getWidth() - 490, 14), juce::Justification::right);
}
