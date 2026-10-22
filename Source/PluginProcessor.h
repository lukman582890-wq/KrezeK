#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DspBlocks.h"

namespace ids
{
    inline constexpr const char* input     = "input";
    inline constexpr const char* output    = "output";
    inline constexpr const char* gate      = "gate";
    inline constexpr const char* blend     = "blend";
    inline constexpr const char* panic     = "panic";
    inline constexpr const char* chaos     = "chaos";
    inline constexpr const char* speed     = "speed";
    inline constexpr const char* rise      = "rise";
    inline constexpr const char* riseSync  = "riseSync";
    inline constexpr const char* oct1      = "oct1";
    inline constexpr const char* oct2      = "oct2";
    inline constexpr const char* latch1    = "latch1";
    inline constexpr const char* latch2    = "latch2";
    inline constexpr const char* stereo    = "stereo";
}

// Mode tempo sync untuk RISE: panjang dalam beat (1 beat = not 1/4).
inline constexpr int numRiseDivs = 7;
inline constexpr float riseBeats[numRiseDivs] = { 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };
inline constexpr const char* riseDivNames[numRiseDivs] = { "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bar" };

class OctaveHavocAudioProcessor : public juce::AudioProcessor
{
public:
    OctaveHavocAudioProcessor();
    ~OctaveHavocAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float> *pInput, *pOutput, *pGate, *pBlend, *pPanic, *pChaos, *pSpeed,
                       *pRise, *pRiseSync, *pOct1, *pOct2, *pStereo;

    double sr = 44100.0;

    DelayPitchShifter shifters[2][2]; // [voice][channel]
    AllPass allpass[2][2];            // [channel][stage]

    juce::SmoothedValue<float> inGain, outGain, blendSm;

    float voiceOct[2] = { 0.0f, 0.0f };
    float voiceGain[2] = { 0.0f, 0.0f };

    float gateEnv = 0.0f, gateGain = 1.0f;
    float lfoPhase = 0.0f;

    juce::Random rng;
    float chaosTarget = 0.0f, chaosState = 0.0f;
    int chaosCountdown = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctaveHavocAudioProcessor)
};
