#pragma once
#include <JuceHeader.h>
#include <vector>
#include "Attack64Layout.h"

// Значения синтезатора в реальных единицах (обновляются каждый блок)
struct SynthParams {
    float wave1 = 1, wave2 = 0, octave = 0, level = 0.8f;
    float filterType = 0, cutoff = 8000, resonance = 0.2f, envAmt = 0.3f;
    float aAtk = 0.01f, aDec = 0.3f, aSus = 0.7f, aRel = 0.4f;
    float bAtk = 0.2f, bDec = 0.8f, bSus = 0.5f, bRel = 1.2f;
    float volume = 0.5f;
};

class Attack64Sound : public juce::SynthesiserSound {
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class Attack64Voice : public juce::SynthesiserVoice {
public:
    explicit Attack64Voice(const SynthParams& p) : P(p) {}
    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<Attack64Sound*>(s) != nullptr; }
    void startNote(int note, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote(float, bool allowTail) override;
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>& out, int start, int num) override;

private:
    static float osc(float type, double ph);
    const SynthParams& P;
    juce::ADSR ampEnv, filtEnv;
    double ph1 = 0.0, ph2 = 0.0, freq = 440.0;
    float vel = 0.0f, lp = 0.0f, bp = 0.0f;
};

class Attack64Processor : public juce::AudioProcessor {
public:
    Attack64Processor();
    ~Attack64Processor() override = default;

    void prepareToPlay(double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Attack64"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Пресеты
    void applyDelayPreset(int index);
    void applyReverbPreset(int index);
    void applySynthPreset(int index);
    static int getNumSynthPresets() { return 300; }
    static juce::String getSynthPresetName(int index);

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void readParams();
    void setParam(const juce::String& id, float value);
    void processDelay(float* L, float* R, int n);

    SynthParams sp;
    juce::Synthesiser synth;

    double currentSampleRate = 44100.0;
    float lfoPhase = 0.0f, lfoDepth = 0.0f, lfoRate = 2.0f;
    float fxTime = 375, fxFb = 0.35f, fxMix = 0.2f, fxTone = 0.5f;

    std::vector<float> dlL, dlR;
    int dlW = 0;
    float lpL = 0.0f, lpR = 0.0f;

    juce::Reverb reverb;
    juce::Reverb::Parameters revP;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Attack64Processor)
};
