#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "WavetableEngine.h"
#include "ModAndArpEngine.h"

struct BlitzSound : public juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class BlitzVoice : public juce::SynthesiserVoice {
public:
    BlitzVoice(juce::AudioProcessorValueTreeState& apvts, WavetableBank& wtBank);
    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<BlitzSound*>(s) != nullptr; }
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
private:
    juce::AudioProcessorValueTreeState& state;
    WavetableBank& bank;
    double currentSampleRate = 44100.0;
    float currentFreq = 440.0f, targetFreq = 440.0f, zapPitchSemis = 0.0f;
    std::array<float, 8> phaseA {};
    float phaseB = 0.0f, phaseC = 0.0f, noteVelocity = 0.0f;
    juce::ADSR env1, env2, env3;
    LFOEngine lfo1, lfo2, lfo3, lfo4;
    juce::dsp::StateVariableTPTFilter<float> filter1L, filter1R, filter2L, filter2R;
    double samplePosA = 0.0, samplePosB = 0.0, grainPhase = 0.0;
    std::array<float, 512> combBufL {}, combBufR {};
    int combIdx = 0;
    float applyWarp(int mode, float phase, float amount, float modSignal);
};

class BlitzAudioProcessor : public juce::AudioProcessor {
public:
    BlitzAudioProcessor();
    ~BlitzAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Blitz-NPC"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    juce::File getRootUserDirectory();
    void reloadWavetables();
    void loadSampleIntoOsc(int oscIdx, const juce::File& wavFile);
    void ensureDefaultAssetsExist();
    void savePresetToFile(const juce::String& bankName, const juce::String& presetName);
    void loadPresetFromFile(const juce::File& presetFile);
    juce::String currentBankName { "01 - Blitz Leads (50)" }, currentPresetName, currentSkinName { "Serum Cyber Cyan" };
    std::atomic<float> meterLevelL { 0.0f }, meterLevelR { 0.0f };
    std::array<float, 256> scopeRing {};
    std::atomic<int> scopeWriteIdx { 0 };
    WavetableBank wavetableBank;
    juce::AudioProcessorValueTreeState apvts;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::Synthesiser synth;
    SimpleArpeggiator arp;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL { 96000 }, delayR { 96000 };
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    juce::dsp::IIR::Filter<float> eqLowL, eqLowR, eqMidL, eqMidR, eqHighL, eqHighR;
    float crushHoldL = 0.0f, crushHoldR = 0.0f;
    int crushCounter = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlitzAudioProcessor)
};
