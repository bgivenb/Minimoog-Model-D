#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
float polyBlep(double t, double dt)
{
    if (t < dt) { auto x = t / dt; return static_cast<float>(x + x - x*x - 1); }
    if (t > 1 - dt) { auto x = (t - 1) / dt; return static_cast<float>(x*x + x + x + 1); }
    return 0;
}
}

float EmberProcessor::Oscillator::next(double hz, double rate, int wave)
{
    auto dt = juce::jlimit(0.000001, 0.45, hz / rate);
    float result = 0;
    if (wave == 0) result = static_cast<float>(2 * phase - 1) - polyBlep(phase, dt);
    else if (wave == 1 || wave == 2 || wave == 5)
    {
        const double width = wave == 1 ? 0.5 : (wave == 2 ? 0.25 : 0.125);
        result = (phase < width ? 1.0f : -1.0f) + polyBlep(phase, dt)
                 - polyBlep(std::fmod(phase + 1 - width, 1.0), dt);
        result -= static_cast<float>(2 * width - 1); // pulse DC compensation
    }
    else
    {
        // Band-limited triangle, odd harmonics below Nyquist.
        for (int h = 1; h <= 31 && h * hz < rate * 0.45; h += 2)
            result += static_cast<float>(((h % 4 == 1) ? 1.0 : -1.0)
                * std::sin(juce::MathConstants<double>::twoPi * phase * h) / (h*h));
        result *= 0.81056947f;
        if (wave == 4) result = result * 0.5f + (static_cast<float>(2 * phase - 1) - polyBlep(phase, dt)) * 0.5f;
    }
    phase += dt;
    phase -= std::floor(phase);
    return result;
}

juce::AudioProcessorValueTreeState::ParameterLayout EmberProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&](const juce::String& id, const juce::String& name, float lo, float hi, float def, float skew = 1.0f)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(id, 1), name,
            juce::NormalisableRange<float>(lo, hi, 0.0f, skew), def));
    };
    for (int i = 1; i <= 3; ++i)
    {
        auto prefix = "osc" + juce::String(i);
        auto name = "Oscillator " + juce::String(i) + " ";
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(prefix + "Wave", 1), name + "Wave",
            juce::StringArray { "Saw", "Square", "Pulse 25%", "Triangle", "Tri / Saw", "Pulse 12%" }, 0));
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(prefix + "Octave", 1), name + "Range",
            juce::StringArray { "32'", "16'", "8'", "4'", "2'", "LO" }, i == 3 ? 1 : 2));
        add(prefix + "Tune", name + "Frequency (semitones)", -7, 7, i == 2 ? 0.07f : (i == 3 ? -0.05f : 0.0f));
        add(prefix + "Level", name + "Level", 0, 1, i == 1 ? 0.8f : (i == 2 ? 0.55f : 0.35f));
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(prefix + "On", 1), name + "On", true));
    }
    add("noise", "Noise", 0, 1, 0);
    add("cutoff", "Filter Cutoff", 20, 20000, 1100, 0.25f);
    add("resonance", "Filter Emphasis", 0, 0.98f, 0.25f);
    add("drive", "Filter Drive", 1, 6, 1.7f);
    add("envAmount", "Filter Contour (octaves)", -6, 6, 3);
    add("keyTrack", "Keyboard Tracking", 0, 1, 0.5f);
    for (auto prefix : { juce::String("amp"), juce::String("filter") })
    {
        auto label = prefix == "amp" ? juce::String("Amp ") : juce::String("Filter ");
        add(prefix + "Attack", label + "Attack", 0.002f, 5, 0.006f, 0.3f);
        add(prefix + "Decay", label + "Decay", 0.01f, 5, prefix == "amp" ? 0.3f : 0.5f, 0.3f);
        add(prefix + "Sustain", label + "Sustain", 0, 1, prefix == "amp" ? 0.8f : 0.15f);
    }
    add("glide", "Glide", 0, 1.5f, 0, 0.4f);
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("legato", 1), "Legato", true));
    add("lfoRate", "LFO Rate", 0.1f, 20, 5, 0.5f);
    add("lfoPitch", "LFO Pitch (semitones)", 0, 2, 0);
    add("lfoFilter", "LFO Filter (octaves)", 0, 4, 0);
    add("velocity", "Velocity Sensitivity", 0, 1, 0.35f);
    add("volume", "Output (dB)", -48, 0, -9);
    add("masterTune", "Master Tune (semitones)", -12, 12, 0);
    add("modMix", "Modulation Mix", 0, 1, 0);
    for (auto id : { "oscMod", "filterMod", "osc3Track", "pink", "output", "a440", "decay" })
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(id, 1), id,
            juce::String(id) == "osc3Track" || juce::String(id) == "output" || juce::String(id) == "decay"));
    return layout;
}

EmberProcessor::EmberProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "EmberState", makeParameters())
{
    for (auto* param : getParameters())
        if (auto* pWithId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            parameters.emplace(pWithId->paramID, state.getRawParameterValue(pWithId->paramID));
}

float EmberProcessor::value(const char* id) const { return parameters.find(id)->second->load(); }

bool EmberProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainInputChannelSet().isDisabled()
        && (layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            || layout.getMainOutputChannelSet() == juce::AudioChannelSet::mono());
}

void EmberProcessor::prepareToPlay(double rate, int maximumBlockSize)
{
    sampleRate = rate;
    filter.prepare({ rate, static_cast<juce::uint32>(juce::jmax(1, maximumBlockSize)), 1 });
    filter.setMode(juce::dsp::LadderFilterMode::LPF24);
    ampEnvelope.setSampleRate(rate);
    filterEnvelope.setSampleRate(rate);
    outputGain.reset(rate, 0.02); outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(value("volume")));
    cutoff.reset(rate, 0.02); cutoff.setCurrentAndTargetValue(value("cutoff"));
    bend.reset(rate, 0.005); bend.setCurrentAndTargetValue(0);
    wheel.reset(rate, 0.01); wheel.setCurrentAndTargetValue(0);
    for (auto& m : mix) { m.reset(rate, 0.01); m.setCurrentAndTargetValue(0); }
    for (auto& osc : oscillators) osc.phase = 0;
    lfoPhase = 0;
    allOff();
}

void EmberProcessor::allOff()
{
    noteCount = 0;
    sustain.fill(false);
    ampEnvelope.reset(); filterEnvelope.reset(); filter.reset();
    dcInput = dcOutput = 0;
    pinkSum = modulationSource = 0;
    pinkRows.fill(0); pinkCounter = 0;
}

void EmberProcessor::selectNote(bool retrigger)
{
    if (noteCount == 0) { ampEnvelope.noteOff(); filterEnvelope.noteOff(); return; }
    // The original keyboard gives priority to the lowest held key.
    const auto& note = *std::min_element(notes.begin(), notes.begin() + noteCount,
        [](const Note& a, const Note& b) { return a.key < b.key; });
    targetFrequency = juce::MidiMessage::getMidiNoteInHertz(note.key);
    noteVelocity = note.velocity;
    if (retrigger) { ampEnvelope.noteOn(); filterEnvelope.noteOn(); }
}

void EmberProcessor::handleMidi(const juce::MidiMessage& m)
{
    const int channel = m.getChannel();
    if (m.isNoteOn())
    {
        bool wasActive = noteCount > 0;
        for (int i = noteCount - 1; i >= 0; --i)
            if (notes[static_cast<size_t>(i)].key == m.getNoteNumber() && notes[static_cast<size_t>(i)].channel == channel)
            { for (int j = i; j < noteCount - 1; ++j) notes[static_cast<size_t>(j)] = notes[static_cast<size_t>(j+1)]; --noteCount; }
        if (noteCount == 128) { std::move(notes.begin()+1, notes.end(), notes.begin()); --noteCount; }
        notes[static_cast<size_t>(noteCount++)] = { m.getNoteNumber(), channel, m.getFloatVelocity(), true };
        selectNote(!wasActive || !p.legato);
        if (!wasActive || p.glide < 0.001f) frequency = targetFrequency;
    }
    else if (m.isNoteOff())
    {
        for (int i = noteCount - 1; i >= 0; --i)
            if (notes[static_cast<size_t>(i)].key == m.getNoteNumber() && notes[static_cast<size_t>(i)].channel == channel)
            {
                notes[static_cast<size_t>(i)].held = false;
                if (!sustain[static_cast<size_t>(channel - 1)])
                { for (int j = i; j < noteCount - 1; ++j) notes[static_cast<size_t>(j)] = notes[static_cast<size_t>(j+1)]; --noteCount; }
            }
        selectNote(false);
    }
    else if (m.isPitchWheel()) bend.setTargetValue((m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
    else if (m.isAllSoundOff()) allOff();
    else if (m.isAllNotesOff()) { noteCount = 0; sustain.fill(false); selectNote(false); }
    else if (m.isController())
    {
        if (m.getControllerNumber() == 1) wheel.setTargetValue(m.getControllerValue() / 127.0f);
        if (m.getControllerNumber() == 64)
        {
            sustain[static_cast<size_t>(channel - 1)] = m.getControllerValue() >= 64;
            if (!sustain[static_cast<size_t>(channel - 1)])
            {
                for (int i = noteCount - 1; i >= 0; --i)
                    if (!notes[static_cast<size_t>(i)].held && notes[static_cast<size_t>(i)].channel == channel)
                    { for (int j = i; j < noteCount-1; ++j) notes[static_cast<size_t>(j)] = notes[static_cast<size_t>(j+1)]; --noteCount; }
                selectNote(false);
            }
        }
    }
}

float EmberProcessor::renderSample()
{
    const float gain = outputGain.getNextValue();
    const float baseCutoff = cutoff.getNextValue();
    const float pitchBend = bend.getNextValue() + p.pitchWheel * 2;
    const float modWheel = juce::jmax(wheel.getNextValue(), p.mod);
    const float lfo = static_cast<float>(std::sin(lfoPhase * juce::MathConstants<double>::twoPi));
    lfoPhase += p.lfoRate / sampleRate; lfoPhase -= std::floor(lfoPhase);
    frequency += (targetFrequency - frequency) * (p.glide < 0.001f ? 1.0 : -std::expm1(-1.0 / (p.glide * sampleRate)));
    float mixed = 0;
    const float whiteNoise = random.nextFloat() * 2 - 1;
    // Voss-McCartney octave-rate sample-and-hold rows plus full-rate white noise.
    auto counter = ++pinkCounter;
    unsigned row = 0;
    while ((counter & 1u) == 0u && row < 16u) { ++row; counter >>= 1u; }
    if (row < 16u)
    {
        pinkSum -= pinkRows[row]; pinkRows[row] = random.nextFloat() * 2 - 1; pinkSum += pinkRows[row];
    }
    const float noise = p.pink ? (pinkSum + whiteNoise) * 0.24f : whiteNoise;
    const float modulation = modulationSource * (1 - p.modMix) + noise * p.modMix;
    for (size_t i = 0; i < 3; ++i)
    {
        const auto base = (i == 2 && !p.osc3Track) ? 261.625565 : frequency;
        const auto range = p.octave[i] == 5 ? -7 : p.octave[i] - 2;
        const auto hz = base * std::exp2(range + (p.tune[i] + p.masterTune + pitchBend
                      + lfo * p.lfoPitch + (p.oscMod ? modulation * modWheel * 12 : 0)) / 12.0);
        const float waveSample = oscillators[i].next(hz, sampleRate, p.wave[i]);
        if (i == 2) modulationSource = waveSample;
        mixed += waveSample * mix[i].getNextValue();
    }
    mixed += noise * mix[3].getNextValue();
    const float env = filterEnvelope.getNextSample();
    auto hz = baseCutoff * std::exp2(env * p.env + p.lfoFilter * lfo + (p.filterMod ? modulation * modWheel * 4 : 0))
            * std::pow(static_cast<float>(frequency / 261.625565), p.track);
    filter.setCutoffFrequencyHz(juce::jlimit(20.0f, static_cast<float>(juce::jmin(20000.0, sampleRate * 0.45)), hz));
    float sample = filter.tick(mixed * 0.28f);
    float blocked = sample - dcInput + 0.995f * dcOutput;
    dcInput = sample; dcOutput = blocked;
    sample = std::tanh(blocked * 1.5f) * ampEnvelope.getNextSample()
           * (1.0f - p.velocity + p.velocity * noteVelocity) * gain;
    referencePhase += 440.0 / sampleRate; referencePhase -= std::floor(referencePhase);
    if (p.a440) sample += static_cast<float>(std::sin(referencePhase * juce::MathConstants<double>::twoPi)) * 0.1f * gain;
    return p.output ? sample : 0.0f;
}

void EmberProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    keyboard.processNextMidiBuffer(midi, 0, audio.getNumSamples(), true);
    if (panicRequested.exchange(false)) allOff();
    static constexpr const char* oscIds[3][5] = {
        { "osc1Wave", "osc1Octave", "osc1Tune", "osc1On", "osc1Level" },
        { "osc2Wave", "osc2Octave", "osc2Tune", "osc2On", "osc2Level" },
        { "osc3Wave", "osc3Octave", "osc3Tune", "osc3On", "osc3Level" }
    };
    for (int i = 0; i < 3; ++i)
    {
        p.wave[static_cast<size_t>(i)] = static_cast<int>(value(oscIds[i][0]));
        p.octave[static_cast<size_t>(i)] = static_cast<int>(value(oscIds[i][1]));
        p.tune[static_cast<size_t>(i)] = value(oscIds[i][2]);
        mix[i].setTargetValue(value(oscIds[i][3]) > 0.5f ? value(oscIds[i][4]) : 0.0f);
    }
    mix[3].setTargetValue(value("noise"));
    p.resonance = value("resonance"); p.drive = value("drive"); p.env = value("envAmount");
    p.track = value("keyTrack"); p.glide = value("glide"); p.legato = value("legato") > 0.5f;
    p.lfoRate = value("lfoRate"); p.lfoPitch = value("lfoPitch"); p.lfoFilter = value("lfoFilter"); p.velocity = value("velocity");
    p.masterTune = value("masterTune"); p.modMix = value("modMix"); p.oscMod = value("oscMod") > 0.5f;
    p.filterMod = value("filterMod") > 0.5f; p.osc3Track = value("osc3Track") > 0.5f;
    p.pink = value("pink") > 0.5f; p.output = value("output") > 0.5f; p.a440 = value("a440") > 0.5f;
    p.pitchWheel = uiPitch.load(); p.mod = uiMod.load();
    filter.setResonance(p.resonance); filter.setDrive(p.drive);
    cutoff.setTargetValue(value("cutoff")); outputGain.setTargetValue(juce::Decibels::decibelsToGain(value("volume")));
    const bool decay = value("decay") > 0.5f;
    auto updateEnvelope = [](juce::ADSR& envelope, juce::ADSR::Parameters settings)
    {
        const auto old = envelope.getParameters();
        // Reapplying ADSR settings each block would recalculate an active release rate.
        if (old.attack != settings.attack || old.decay != settings.decay
            || old.sustain != settings.sustain || old.release != settings.release)
            envelope.setParameters(settings);
    };
    updateEnvelope(ampEnvelope, { value("ampAttack"), value("ampDecay"), value("ampSustain"), decay ? value("ampDecay") : 0.005f });
    updateEnvelope(filterEnvelope, { value("filterAttack"), value("filterDecay"), value("filterSustain"), decay ? value("filterDecay") : 0.005f });
    int position = 0;
    float blockPeak = 0;
    auto render = [&](int end)
    {
        for (; position < end; ++position)
        {
            const auto sample = renderSample();
            blockPeak = juce::jmax(blockPeak, std::abs(sample));
            for (int channel = 0; channel < audio.getNumChannels(); ++channel) audio.setSample(channel, position, sample);
        }
    };
    for (const auto metadata : midi)
    { render(juce::jlimit(0, audio.getNumSamples(), metadata.samplePosition)); handleMidi(metadata.getMessage()); }
    render(audio.getNumSamples());
    peak.store(juce::jmax(blockPeak, peak.load() * 0.92f));
    midi.clear();
}

const juce::String EmberProcessor::getProgramName(int index)
{
    static const juce::StringArray names { "01 / Warm Current", "02 / Basement Bass", "03 / Copper Lead", "04 / Soft Triangle", "05 / Lunar Sweep" };
    return names[juce::jlimit(0, 4, index)];
}

void EmberProcessor::setCurrentProgram(int index)
{
    index = juce::jlimit(0, 4, index);
    for (auto* parameter : getParameters()) parameter->setValueNotifyingHost(parameter->getDefaultValue());
    auto set = [&](const char* id, float v) { auto* param = state.getParameter(id); param->setValueNotifyingHost(param->convertTo0to1(v)); };
    if (index == 1) { set("cutoff", 220); set("resonance", 0.4f); set("envAmount", 4); set("ampSustain", 0.6f); set("filterDecay", 0.23f); set("filterSustain", 0); set("osc1Octave", 1); set("drive", 2.5f); }
    if (index == 2) { set("osc1Wave", 1); set("osc2Wave", 2); set("osc3Level", 0); set("cutoff", 2300); set("resonance", 0.48f); set("glide", 0.09f); set("lfoPitch", 0.055f); }
    if (index == 3) { set("osc1Wave", 3); set("osc2Wave", 3); set("osc3Level", 0.15f); set("cutoff", 1700); set("envAmount", 1); set("ampAttack", 0.09f); set("ampDecay", 1.2f); set("drive", 1); }
    if (index == 4) { set("cutoff", 450); set("resonance", 0.78f); set("filterAttack", 1.2f); set("filterDecay", 2); set("envAmount", 4.5f); set("ampAttack", 0.3f); set("ampDecay", 2); set("noise", 0.12f); set("lfoFilter", 0.6f); set("lfoRate", 0.3f); }
    currentProgram.store(index);
}

void EmberProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto tree = state.copyState(); tree.setProperty("program", currentProgram.load(), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary(*xml, dest);
}
void EmberProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            currentProgram.store(juce::jlimit(0, 4, static_cast<int>(tree.getProperty("program", 0))));
            state.replaceState(tree);
        }
}
juce::AudioProcessorEditor* EmberProcessor::createEditor() { return new EmberEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EmberProcessor(); }
