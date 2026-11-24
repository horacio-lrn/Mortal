#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <vector>

class GranularEngine
{
public:
    struct Params { float position, size, density, pitch, spray, feedback, mix, durRnd; };

    void prepare (double sr)
    {
        sampleRate = sr;
        bufferSize = (int) (sr * 6.0);
        bufL.assign ((size_t) bufferSize, 0.0f);
        bufR.assign ((size_t) bufferSize, 0.0f);
        writeIndex = 0;
        countdown = 0;
        for (auto& g : grains) g.active = false;
    }

    void setParams (const Params& p) { params = p; }
    float getActivity() const noexcept { return activity; }

    void process (float* L, float* R, int numSamples)
    {
        if (bufferSize == 0) return;

        const double baseLen = juce::jlimit (5.0, 4000.0, (double) params.size) * 0.001 * sampleRate;
        const double interval = sampleRate / juce::jmax (1.0, (double) params.density);
        const double ratio = std::pow (2.0, params.pitch / 12.0);
        const float minDelay = (float) (0.02 * sampleRate);
        const float maxDelay = (float) (3.5 * sampleRate);
        const float fb = params.feedback;
        const float mix = params.mix;

        for (int i = 0; i < numSamples; ++i)
        {
            if (--countdown <= 0)
            {
                spawnGrain (baseLen, ratio, minDelay, maxDelay);
                countdown += juce::jmax (1, (int) interval);
            }

            float wetL = 0.0f, wetR = 0.0f;
            int active = 0;

            for (auto& g : grains)
            {
                if (! g.active) continue;
                ++active;

                const float t = (float) (g.age / g.length);
                const float s = std::sin (juce::MathConstants<float>::pi * t);
                const float env = s * s;

                const int i0 = wrap ((int) g.readPos);
                const int i1 = wrap ((int) g.readPos + 1);
                const float frac = (float) (g.readPos - std::floor (g.readPos));

                const float sL = bufL[(size_t) i0] + (bufL[(size_t) i1] - bufL[(size_t) i0]) * frac;
                const float sR = bufR[(size_t) i0] + (bufR[(size_t) i1] - bufR[(size_t) i0]) * frac;

                wetL += sL * env * g.gainL;
                wetR += sR * env * g.gainR;

                g.readPos += g.inc;
                if (g.readPos >= bufferSize) g.readPos -= bufferSize;
                g.age += 1.0;
                if (g.age >= g.length) g.active = false;
            }

            activity += ((float) active / (float) maxGrains - activity) * 0.02f;

            const float inL = L[i];
            const float inR = R[i];

            bufL[(size_t) writeIndex] = inL + std::tanh (wetL) * fb;
            bufR[(size_t) writeIndex] = inR + std::tanh (wetR) * fb;
            if (++writeIndex >= bufferSize) writeIndex = 0;

            L[i] = inL * (1.0f - mix) + wetL * mix;
            R[i] = inR * (1.0f - mix) + wetR * mix;
        }
    }

private:
    static constexpr int maxGrains = 64;

    struct Grain
    {
        bool active = false;
        double readPos = 0.0, inc = 1.0, length = 1.0, age = 0.0;
        float gainL = 0.0f, gainR = 0.0f;
    };

    int wrap (int i) const noexcept
    {
        i %= bufferSize;
        if (i < 0) i += bufferSize;
        return i;
    }

    void spawnGrain (double baseLen, double ratio, float minDelay, float maxDelay)
    {
        for (auto& g : grains)
        {
            if (g.active) continue;

            const float spray = params.spray;
            const float base = minDelay + params.position * (maxDelay - minDelay);
            const float jitter = (rnd.nextFloat() * 2.0f - 1.0f) * spray * maxDelay * 0.5f;
            const float delay = juce::jlimit (minDelay, maxDelay, base + jitter);

            double start = (double) writeIndex - (double) delay;
            while (start < 0.0) start += bufferSize;
            g.readPos = start;

            // Duracion propia de cada grano: la base mas una variacion aleatoria
            const double durMul = 1.0 + (double) (rnd.nextFloat() * 2.0f - 1.0f) * (double) params.durRnd;
            const double len = juce::jlimit (0.005 * sampleRate, 4.0 * sampleRate, baseLen * durMul);

            const float pan = (rnd.nextFloat() * 2.0f - 1.0f) * spray;
            const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            const double grainSec = len / sampleRate;
            const float amp = 1.0f / std::sqrt (juce::jmax (1.0f, params.density * (float) grainSec));

            g.gainL = std::cos (angle) * amp;
            g.gainR = std::sin (angle) * amp;
            g.inc = ratio;
            g.length = len;
            g.age = 0.0;
            g.active = true;
            return;
        }
    }

    double sampleRate = 44100.0;
    int bufferSize = 0;
    std::vector<float> bufL, bufR;
    int writeIndex = 0;
    int countdown = 0;
    Params params { 0.5f, 80.0f, 20.0f, 0.0f, 0.1f, 0.0f, 0.5f, 0.0f };
    std::array<Grain, maxGrains> grains {};
    float activity = 0.0f;
    juce::Random rnd;
};
