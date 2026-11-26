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
    double getBpm() const noexcept { return bpmNow.load(); }
    static juce::StringArray getDivisionNames();

    // Niveles para los medidores
    const std::atomic<float>& getInputLevel() const noexcept { return inLevel; }
    const std::atomic<float>& getOutputLevel() const noexcept { return outLevel; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static double divisionBeats (int index) noexcept;

    void processLsd (juce::AudioBuffer<float>& buffer, int n);
    void processFreeze (juce::AudioBuffer<float>& buffer, int n);
    void processStutter (juce::AudioBuffer<float>& buffer, int n);
    void processFlanger (juce::AudioBuffer<float>& buffer, int n);

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
    std::atomic<float>* lsdAmt = nullptr;
    std::atomic<float>* ketAmt = nullptr;
    std::atomic<float>* cocAmt = nullptr;
    std::atomic<float>* syncParam = nullptr;
    std::array<std::atomic<float>*, 7> divParams {};
    std::atomic<float>* flangerAmt = nullptr;

    GranularEngine engine;
    std::atomic<float> activity { 0.0f };
    std::atomic<float> displayPhase { 0.0f };
    std::array<std::atomic<float>, shapeSize> shape;
    std::atomic<float> inLevel { 0.0f };
    std::atomic<float> outLevel { 0.0f };
    std::atomic<double> bpmNow { 120.0 };
    bool syncNow = false;
    double secLfo = 0.5, secDelay = 0.5, secGrain = 0.25, secLsd = 0.5, secFreeze = 0.25, secStut = 0.25, secFlange = 0.25;

    // FLANGER: linea de retardo corta
    std::vector<float> flBuf[2];
    int flIdx = 0;
    double flPh = 0.0;

    juce::Reverb reverb;
    juce::AudioBuffer<float> revBuf;
    float hpCoef = 0.0f;
    float hpX1[2] { 0.0f, 0.0f };
    float hpY1[2] { 0.0f, 0.0f };

    // Delay corto y calido
    std::vector<float> dLine[2];
    int dIdx = 0;
    int dSamples = 1;
    float dLpCoef = 0.0f;
    float dLpState[2] { 0.0f, 0.0f };

    // LSD: pitch warp con linea de retardo modulada
    std::vector<float> lsdBuf[2];
    int lsdIdx = 0;
    double lsdPh1 = 0.0, lsdPh2 = 0.0;

    // KETAMINE: congelado con dos cabezales de ventana Hann desfasados
    std::vector<float> kBuf[2];
    int kIdx = 0;
    bool kFrozen = false;
    int kStart = 0;
    int kR = 0;
    int kLen = 1;

    // COCAINE: stutter de fragmentos repetidos
    std::vector<float> stBuf[2];
    int stIdx = 0;
    int stCount = 0;
    int stSrc = 0;
    int stPhase = 0;
    int stFrag = 1;

    double sampleRate = 44100.0;
    double lfoPhase = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MortalProcessor)
};
