#pragma once
#include "PluginProcessor.h"

class ModelDLook final : public juce::LookAndFeel_V4
{
public:
    ModelDLook();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                          juce::Slider::SliderStyle, juce::Slider&) override;
};

class EmberEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit EmberEditor(EmberProcessor&);
    ~EmberEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Knob : juce::Component
    {
        Knob(EmberProcessor&, const juce::String&, const juce::String&, const juce::String&);
        void paint(juce::Graphics&) override;
        void resized() override;
        juce::Slider slider;
        juce::String caption;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    void addKnob(const char*, const char*, int, int, const char* = "");
    void addSwitch(const char*, const char*, int, int, bool = false);
    void timerCallback() override;
    void createWood();
    EmberProcessor& processor;
    ModelDLook look;
    juce::Component panel;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::ToggleButton>> switches;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> switchAttachments;
    juce::MidiKeyboardComponent keyboard;
    juce::Slider pitchWheel, modWheel;
    juce::ComboBox presets;
    juce::TextButton panic { "ALL NOTES OFF" };
    juce::TooltipWindow tooltips { this, 650 };
    juce::Image wood;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmberEditor)
};
