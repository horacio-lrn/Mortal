#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Area de dibujo de la forma de onda del LFO. Arrastra el raton para dibujar.
class LfoShapeEditor : public juce::Component
{
public:
    explicit LfoShapeEditor (MortalProcessor& p) : processor (p) {}

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const juce::Colour red (0xffff2b2b);
        const int N = MortalProcessor::shapeSize;

        g.setColour (juce::Colour (0xff1a0c0c));
        g.fillRoundedRectangle (b, 6.0f);

        g.setColour (juce::Colour (0xff3a1a1a));
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine ((int) (b.getY() + b.getHeight() * (float) i / 4.0f), b.getX(), b.getRight());
        for (int i = 1; i < 8; ++i)
            g.drawVerticalLine ((int) (b.getX() + b.getWidth() * (float) i / 8.0f), b.getY(), b.getBottom());

        juce::Path curve;
        for (int i = 0; i < N; ++i)
        {
            const float x = b.getX() + b.getWidth() * (float) i / (float) (N - 1);
            const float y = b.getBottom() - b.getHeight() * processor.getShapeValue (i);
            if (i == 0) curve.startNewSubPath (x, y);
            else        curve.lineTo (x, y);
        }

        juce::Path fill = curve;
        fill.lineTo (b.getRight(), b.getBottom());
        fill.lineTo (b.getX(), b.getBottom());
        fill.closeSubPath();

        g.setColour (red.withAlpha (0.18f));
        g.fillPath (fill);
        g.setColour (red);
        g.strokePath (curve, juce::PathStrokeType (2.0f));

        // Cabezal de reproduccion del LFO
        const float px = b.getX() + b.getWidth() * processor.getLfoPhase();
        g.setColour (juce::Colour (0x66ffffff));
        g.drawVerticalLine ((int) px, b.getY(), b.getBottom());

        g.setColour (juce::Colour (0xff8a5050));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        lastIdx = -1;
        drawAt (e.position);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        drawAt (e.position);
    }

private:
    void drawAt (juce::Point<float> pos)
    {
        auto b = getLocalBounds().toFloat();
        if (b.getWidth() <= 0.0f || b.getHeight() <= 0.0f) return;

        const int N = MortalProcessor::shapeSize;
        const float xN = juce::jlimit (0.0f, 1.0f, (pos.x - b.getX()) / b.getWidth());
        const float yN = juce::jlimit (0.0f, 1.0f, 1.0f - (pos.y - b.getY()) / b.getHeight());
        const int idx = juce::jlimit (0, N - 1, (int) (xN * (float) (N - 1) + 0.5f));

        if (lastIdx < 0)
        {
            lastIdx = idx;
            lastY = yN;
        }

        const int from = lastIdx;
        if (from == idx)
        {
            processor.setShapeValue (idx, yN);
        }
        else
        {
            // Interpola entre el punto anterior y el actual para que la linea no tenga huecos
            const int step = idx > from ? 1 : -1;
            for (int k = from; ; k += step)
            {
                const float t = (float) (k - from) / (float) (idx - from);
                processor.setShapeValue (k, lastY + (yN - lastY) * t);
                if (k == idx) break;
            }
        }

        lastIdx = idx;
        lastY = yN;
        repaint();
    }

    MortalProcessor& processor;
    int lastIdx = -1;
    float lastY = 0.0f;
};
