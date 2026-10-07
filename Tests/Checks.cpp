#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <stdexcept>
#include "../Source/PluginProcessor.h"

void require(bool condition, const char* message)
{ if (!condition) throw std::runtime_error(message); }

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    try
    {
        require(argc >= 3, "Usage: EmberChecks <bundle.vst3> <output directory>");
        juce::File bundle(argv[1]), output(argv[2]); output.createDirectory();
        juce::AudioPluginFormatManager formats; juce::addDefaultFormatsToManager(formats);
        juce::OwnedArray<juce::PluginDescription> descriptions;
        for (auto* format : formats.getFormats()) format->findAllTypesForFile(descriptions, bundle.getFullPathName());
        require(descriptions.size() == 1, "VST3 scan must find exactly one instrument");
        juce::String error;
        auto plugin = formats.createPluginInstance(*descriptions[0], 48000, 256, error);
        require(plugin != nullptr, error.toRawUTF8());
        require(descriptions[0]->isInstrument && plugin->acceptsMidi(), "Must be a MIDI instrument");
        std::cout << "Loaded actual VST3: " << plugin->getName() << "\n";
        auto parameter = [&](const juce::String& name) -> juce::AudioProcessorParameter*
        {
            for (auto* p : plugin->getParameters()) if (p->getName(128) == name) return p;
            throw std::runtime_error(("Parameter missing: " + name).toStdString());
        };
        auto set = [&](const juce::String& name, float value) { parameter(name)->setValueNotifyingHost(value); };
        plugin->prepareToPlay(48000, 256);
        auto process = [&](int count, const juce::MidiBuffer& events)
        {
            juce::AudioBuffer<float> audio(2, count); audio.clear();
            juce::MidiBuffer midi(events); plugin->processBlock(audio, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int s = 0; s < count; ++s)
                { require(std::isfinite(audio.getSample(ch,s)), "Non-finite output"); require(std::abs(audio.getSample(ch,s)) <= 1.01f, "Unsafe output peak"); }
            return audio;
        };
        auto quiet = process(256, {}); require(quiet.getMagnitude(0,256) < 1e-7f, "Idle instrument must be silent");
        juce::MidiBuffer on; on.addEvent(juce::MidiMessage::noteOn(1, 48, 0.9f), 97);
        auto start = process(256,on);
        require(start.getMagnitude(0,97) < 1e-7f, "Note starts before MIDI timestamp");
        require(start.getMagnitude(100,156) > 1e-6f, "MIDI note produced no audio");
        juce::MidiBuffer sustain; sustain.addEvent(juce::MidiMessage::controllerEvent(1,64,127),0);
        sustain.addEvent(juce::MidiMessage::noteOff(1,48),1); process(256,sustain);
        for (int i = 0; i < 220; ++i) quiet = process(256,{});
        require(quiet.getRMSLevel(0,0,256) > 0.001f, "Sustain pedal failed");
        juce::MidiBuffer up; up.addEvent(juce::MidiMessage::controllerEvent(1,64,0),0); process(256,up);
        for (int i = 0; i < 1500; ++i) quiet = process(256,{});
        require(quiet.getMagnitude(0,256) < 1e-6f, "Stuck note after sustain release");
        std::cout << "PASS: silence, sample-accurate note-on, sustain pedal, release\n";

        set("Filter Cutoff", 0.37f); set("Oscillator 2 Frequency (semitones)", 0.56f);
        juce::MemoryBlock saved; plugin->getStateInformation(saved);
        set("Filter Cutoff", 0.8f); plugin->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        require(std::abs(parameter("Filter Cutoff")->getValue()-0.37f) < 0.0001f, "State recall failed");
        const char invalid[] = "not a valid state"; plugin->setStateInformation(invalid, sizeof(invalid));
        require(std::abs(parameter("Filter Cutoff")->getValue()-0.37f) < 0.0001f, "Invalid state changed patch");
        std::cout << "PASS: parameter automation and state round trip\n";

        // Verify the musical behavior, not just finite samples: low-note priority,
        // return to a held key, exact tuning and two-semitone MIDI pitch bend.
        plugin->setCurrentProgram(0);
        set("Oscillator 1 Wave", 0.6f); set("Oscillator 2 On", 0); set("Oscillator 3 On", 0);
        set("Filter Cutoff", 1); set("Filter Emphasis", 0); set("Filter Contour (octaves)", 0.5f);
        set("Amp Sustain", 1);
        auto estimateFrequency = [&](const juce::MidiBuffer& events)
        {
            process(256,events);
            for(int i=0;i<80;++i) process(256,{});
            int crossings=0; float previous=0; const int samples=48000;
            for(int offset=0;offset<samples;offset+=256)
            {
                auto a=process(juce::jmin(256,samples-offset),{});
                for(int i=0;i<a.getNumSamples();++i)
                { float current=a.getSample(0,i); if(previous<0 && current>=0) ++crossings; previous=current; }
            }
            return crossings;
        };
        juce::MidiBuffer low; low.addEvent(juce::MidiMessage::noteOn(1,60,1.0f),0);
        require(std::abs(estimateFrequency(low)-262) <= 2, "C4 oscillator tuning failed");
        juce::MidiBuffer high; high.addEvent(juce::MidiMessage::noteOn(1,72,1.0f),0);
        require(std::abs(estimateFrequency(high)-262) <= 2, "Lowest-note priority failed");
        juce::MidiBuffer releaseLow; releaseLow.addEvent(juce::MidiMessage::noteOff(1,60),0);
        require(std::abs(estimateFrequency(releaseLow)-523) <= 2, "Held-note fallback failed");
        juce::MidiBuffer bendUp; bendUp.addEvent(juce::MidiMessage::pitchWheel(1,16383),0);
        require(std::abs(estimateFrequency(bendUp)-587) <= 2, "Pitch wheel range failed");
        std::cout << "PASS: oscillator tuning, lowest-note priority, held-note fallback, pitch bend\n";

        // All presets, multiple host rates/block sizes, pitch bend, overlapping notes and panic.
        for (double rate : { 44100.0, 48000.0, 96000.0 })
            for (int size : { 1, 64, 257, 1024 })
            {
                plugin->releaseResources(); plugin->prepareToPlay(rate,size);
                for (int program = 0; program < plugin->getNumPrograms(); ++program)
                {
                    plugin->setCurrentProgram(program);
                    juce::MidiBuffer notes;
                    notes.addEvent(juce::MidiMessage::noteOn(1,36+program*7,0.95f),0);
                    notes.addEvent(juce::MidiMessage::pitchWheel(1,12000),0);
                    auto audio = process(size,notes);
                    for (int b = 0; b < 8; ++b) audio = process(size,{});
                    juce::MidiBuffer kill; kill.addEvent(juce::MidiMessage::allSoundOff(1),0);
                    audio = process(size,kill);
                    require(audio.getMagnitude(0,size) < 1e-6f, "All sound off failed");
                }
            }
        std::cout << "PASS: 5 presets at 3 sample rates / 4 buffer sizes, panic\n";

        plugin->releaseResources(); plugin->prepareToPlay(48000,256);
        juce::Random random(1970);
        for (int round = 0; round < 80; ++round)
        {
            for (auto* p : plugin->getParameters())
                if (p->getName(128) != "Bypass") p->setValueNotifyingHost(random.nextFloat());
            juce::MidiBuffer notes; notes.addEvent(juce::MidiMessage::noteOn(1,random.nextInt(100)+12,0.9f),0);
            process(256,notes); for (int b=0; b<8; ++b) process(256,{});
        }
        std::cout << "PASS: randomized parameter / MIDI stability\n";
        plugin->setCurrentProgram(0); plugin->releaseResources(); plugin->prepareToPlay(48000,256);

        // Render an audible sequence using the shipping VST3 binary.
        const int seconds = 15, total = seconds*48000;
        juce::AudioBuffer<float> demo(2,total); demo.clear();
        for (int offset=0; offset<total; offset+=256)
        {
            juce::MidiBuffer events;
            const int count = juce::jmin(256,total-offset);
            for (int i=0; i<count; ++i)
            {
                int t = offset+i;
                if (t % 24000 == 0) events.addEvent(juce::MidiMessage::noteOn(1,48 + std::array<int,8>{0,0,7,10,12,7,3,5}[static_cast<size_t>((t/24000)%8)],0.9f),i);
                if (t % 24000 == 18000) events.addEvent(juce::MidiMessage::allNotesOff(1),i);
            }
            auto block=process(count,events);
            for(int ch=0;ch<2;++ch) demo.copyFrom(ch,offset,block,ch,0,count);
        }
        juce::WavAudioFormat wav;
        auto stream=output.getChildFile("Model-D-demo.wav").createOutputStream();
        require(stream != nullptr, "Cannot create demo file"); stream->setPosition(0); stream->truncate();
        auto writer=std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(stream.get(),48000,2,24,{},0));
        require(writer != nullptr, "Cannot create WAV writer"); stream.release();
        require(writer->writeFromAudioSampleBuffer(demo,0,total), "Cannot write demo audio"); writer.reset();

        auto hostedEditor=std::unique_ptr<juce::AudioProcessorEditor>(plugin->createEditorIfNeeded());
        require(hostedEditor != nullptr, "Plugin editor failed to open");
        hostedEditor.reset();
        // A VST3 host wrapper contains a foreign native HWND, which a JUCE
        // component snapshot cannot traverse. Render the identical editor source directly.
        auto preview=std::make_unique<EmberProcessor>();
        auto editor=std::unique_ptr<juce::AudioProcessorEditor>(preview->createEditorIfNeeded());
        editor->setVisible(true);
        juce::PNGImageFormat png;
        auto saveImage=[&](const char* file)
        {
            auto image=editor->createComponentSnapshot(editor->getLocalBounds());
            auto out=output.getChildFile(file).createOutputStream();
            require(out != nullptr, "Cannot create screenshot"); out->setPosition(0); out->truncate();
            require(png.writeImageToStream(image,*out), "Cannot write screenshot");
        };
        saveImage("Model-D-panel.png");
        editor->setSize(1008,560); saveImage("Model-D-panel-small.png");
        editor.reset(); plugin->releaseResources();
        std::cout << "PASS: editor creation, resize, screenshots; rendered 15-second WAV\nALL CHECKS PASSED\n";
        return 0;
    }
    catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}
