#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class EmberProcessor final : public juce::AudioProcessor
{
public:
    EmberProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Model D 1970"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }
    int getNumPrograms() override { return 5; }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;
    std::atomic<float> peak { 0.0f };
    std::atomic<bool> panicRequested { false };
    std::atomic<float> uiPitch { 0 }, uiMod { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    struct Oscillator
    {
        double phase = 0;
        float next(double frequency, double rate, int wave);
    };
    struct Filter : juce::dsp::LadderFilter<float>
    {
        float tick(float x) { updateSmoothers(); return processSample(x, 0); }
    } filter;
    struct Note { int key = 0, channel = 1; float velocity = 1; bool held = false; };
    std::array<Note, 128> notes {};
    std::array<bool, 16> sustain {};
    int noteCount = 0;
    void handleMidi(const juce::MidiMessage&);
    void selectNote(bool retrigger);
    void allOff();
    float renderSample();
    float value(const char* id) const;
    struct Parameters
    {
        std::array<float, 3> level {}, tune {};
        std::array<int, 3> wave {}, octave {};
        float noise = 0, cutoff = 1200, resonance = 0, drive = 1, env = 3;
        float track = 0.5f, glide = 0, lfoRate = 4, lfoPitch = 0, lfoFilter = 0, velocity = 0.3f;
        bool legato = true;
        std::array<bool, 3> enabled { true, true, true };
        float masterTune = 0, modMix = 0, pitchWheel = 0, mod = 0;
        bool oscMod = false, filterMod = false, osc3Track = true, pink = false, output = true, a440 = false;
    } p;
    // Cached atomic pointers: no map lookup or allocation in the sample loop.
    struct ParameterLess
    {
        using is_transparent = void;
        bool operator()(const juce::String& a, const juce::String& b) const { return a < b; }
        bool operator()(const juce::String& a, const char* b) const { return std::strcmp(a.toRawUTF8(), b) < 0; }
        bool operator()(const char* a, const juce::String& b) const { return std::strcmp(a, b.toRawUTF8()) < 0; }
    };
    std::map<juce::String, std::atomic<float>*, ParameterLess> parameters;
    std::array<Oscillator, 3> oscillators;
    juce::ADSR ampEnvelope, filterEnvelope;
    juce::SmoothedValue<float> outputGain, bend, wheel, cutoff, mix[4];
    juce::Random random { 0x454d4245 };
    double sampleRate = 44100, frequency = 110, targetFrequency = 110, lfoPhase = 0;
    float noteVelocity = 1, dcInput = 0, dcOutput = 0;
    float pinkSum = 0, modulationSource = 0;
    std::array<float, 16> pinkRows {};
    juce::uint32 pinkCounter = 0;
    double referencePhase = 0;
    std::atomic<int> currentProgram { 0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmberProcessor)
};
