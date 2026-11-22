#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include "GranularEngine.h"

class MortalProcessor : public juce::AudioProcessor
{
public:
    static constexpr int shapeSize = 64;

    MortalProcessor();
    ~MortalProcessor() override = default;

    void prepareToPlay (double sr, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
    }

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mortal"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getActivity() const noexcept { return activity.load(); }

    // Forma del LFO (0 = abajo, 1 = arriba)
    float getShapeValue (int i) const noexcept { return shape[(size_t) i].load(); }
    void setShapeValue (int i, float v) noexcept { shape[(size_t) i].store (juce::jlimit (0.0f, 1.0f, v)); }
    void applyShapePreset (int presetIndex) noexcept; // -1 = linea plana
    void resetShape() noexcept;                       // sinusoide por defecto
    void clearShape() noexcept { applyShapePreset (-1); }
    float getLfoPhase() const noexcept { return displayPhase.load(); }

    // Niveles para los medidores
    const std::atomic<float>& getInputLevel() const noexcept { return inLevel; }
    const std::atomic<float>& getOutputLevel() const noexcept { return outLevel; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<std::atomic<float>*, 8> raw {};
    std::atomic<float>* lfoRate = nullptr;
    std::atomic<float>* lfoDepth = nullptr;
    std::atomic<float>* lfoTarget = nullptr;
    std::atomic<float>* inGain = nullptr;
    std::atomic<float>* outGain = nullptr;
    std::atomic<float>* reverbAmt = nullptr;
    std::atomic<float>* lowEqOn = nullptr;
    std::atomic<float>* delayAmt = nullptr;
    std::atomic<float>* satAmt = nullptr;

    GranularEngine engine;
    std::atomic<float> activity { 0.0f };
    std::atomic<float> displayPhase { 0.0f };
    std::array<std::atomic<float>, shapeSize> shape;
    std::atomic<float> inLevel { 0.0f };
    std::atomic<float> outLevel { 0.0f };

    juce::Reverb reverb;
    juce::AudioBuffer<float> revBuf;
    float hpCoef = 0.0f;
    float hpX1[2] { 0.0f, 0.0f };
    float hpY1[2] { 0.0f, 0.0f };

    // Delay corto y calido: linea de retardo con filtro paso bajo en la realimentacion
    std::vector<float> dLine[2];
    int dIdx = 0;
    int dSamples = 1;
    float dLpCoef = 0.0f;
    float dLpState[2] { 0.0f, 0.0f };

    double sampleRate = 44100.0;
    double lfoPhase = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MortalProcessor)
};
