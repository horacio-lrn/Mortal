#pragma once
#include <JuceHeader.h>
#include <array>
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

    // Forma del LFO dibujada por el usuario (0 = abajo, 1 = arriba)
    float getShapeValue (int i) const noexcept { return shape[(size_t) i].load(); }
    void setShapeValue (int i, float v) noexcept { shape[(size_t) i].store (juce::jlimit (0.0f, 1.0f, v)); }
    void resetShape() noexcept;
    float getLfoPhase() const noexcept { return displayPhase.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<std::atomic<float>*, 8> raw {};
    std::atomic<float>* lfoRate = nullptr;
    std::atomic<float>* lfoDepth = nullptr;
    std::atomic<float>* lfoTarget = nullptr;
    std::atomic<float>* inGain = nullptr;
    std::atomic<float>* outGain = nullptr;

    GranularEngine engine;
    std::atomic<float> activity { 0.0f };
    std::atomic<float> displayPhase { 0.0f };
    std::array<std::atomic<float>, shapeSize> shape;
    double sampleRate = 44100.0;
    double lfoPhase = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MortalProcessor)
};
