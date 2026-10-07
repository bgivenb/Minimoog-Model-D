#include "PluginEditor.h"

namespace
{
const juce::Colour ivory { 0xffe6e2ce }, panelBlack { 0xff171819 };
juce::Font font(float height, bool bold = false)
{ return juce::Font(juce::FontOptions("Arial", height, bold ? juce::Font::bold : juce::Font::plain)); }
void text(juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, float size = 12, bool bold = false)
{ g.setColour(ivory); g.setFont(font(size, bold)); g.drawFittedText(s, r, juce::Justification::centred, 1, 0.75f); }
}

ModelDLook::ModelDLook()
{
    setColour(juce::Slider::textBoxTextColourId, ivory);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff202120));
    setColour(juce::ComboBox::textColourId, ivory);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff81745b));
    setColour(juce::PopupMenu::backgroundColourId, panelBlack);
    setColour(juce::PopupMenu::textColourId, ivory);
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff262726));
    setColour(juce::TextButton::textColourOffId, ivory);
    setColour(juce::Slider::thumbColourId, ivory);
    setColour(juce::Slider::trackColourId, juce::Colour(0xff635e52));
}

void ModelDLook::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                float position, float start, float end, juce::Slider&)
{
    const float cx = x + width * 0.5f, cy = y + height * 0.5f;
    const float radius = juce::jmin(width, height) * 0.34f;
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end-start) * i / 10;
        auto point = [&](float r) { return juce::Point<float>(cx + std::sin(a)*r, cy - std::cos(a)*r); };
        g.setColour(ivory.withAlpha(0.85f));
        g.drawLine({ point(radius + 5), point(radius + 9) }, 1.1f);
    }
    auto body = juce::Rectangle<float>(cx-radius, cy-radius, radius*2, radius*2);
    g.setColour(juce::Colours::black.withAlpha(0.8f)); g.fillEllipse(body.translated(2, 4).expanded(2));
    g.setGradientFill({ juce::Colour(0xff515355), cx-radius, cy-radius, juce::Colour(0xff08090a), cx+radius, cy+radius, false });
    g.fillEllipse(body);
    for (int i = 0; i < 30; ++i)
    {
        float a = i * juce::MathConstants<float>::twoPi / 30;
        g.setColour((i % 2 == 0 ? juce::Colours::black : juce::Colours::white).withAlpha(0.28f));
        g.drawLine(cx+std::sin(a)*radius*0.76f, cy-std::cos(a)*radius*0.76f,
                   cx+std::sin(a)*radius*0.96f, cy-std::cos(a)*radius*0.96f, 2.0f);
    }
    const float cap = radius * 0.67f;
    auto metal = juce::Rectangle<float>(cx-cap, cy-cap, cap*2, cap*2);
    g.setGradientFill({ juce::Colour(0xfff0f1e9), cx-cap, cy-cap, juce::Colour(0xff777f83), cx+cap, cy+cap, false });
    g.fillEllipse(metal);
    g.setColour(juce::Colours::white.withAlpha(0.6f)); g.drawEllipse(metal.reduced(0.6f), 0.8f);
    // Radial fine machining on the aluminium cap.
    for (int i = 0; i < 90; ++i)
    {
        const float a = i * juce::MathConstants<float>::twoPi / 90;
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawLine(cx, cy, cx + std::sin(a)*cap, cy + std::cos(a)*cap, 0.45f);
    }
    const float angle = start + position * (end-start);
    g.setColour(juce::Colour(0xff171717));
    g.drawLine(cx+std::sin(angle)*cap*0.12f, cy-std::cos(angle)*cap*0.12f,
               cx+std::sin(angle)*cap*0.93f, cy-std::cos(angle)*cap*0.93f, 1.7f);
    g.setColour(ivory);
    g.drawLine(cx+std::sin(angle)*radius*0.74f, cy-std::cos(angle)*radius*0.74f,
               cx+std::sin(angle)*radius*0.98f, cy-std::cos(angle)*radius*0.98f, 2.5f);
}

void ModelDLook::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const bool on = b.getToggleState();
    auto r = juce::Rectangle<float>(8, 24, static_cast<float>(b.getWidth()-16), 24);
    g.setColour(juce::Colours::black); g.fillRoundedRectangle(r.expanded(3), 2);
    const auto colour = b.getProperties().getWithDefault("orange", false) ? juce::Colour(0xffed541d) : juce::Colour(0xff6996a0);
    auto top = on ? colour.brighter(0.25f) : colour.darker(0.15f);
    if (over) top = top.brighter(0.12f);
    g.setGradientFill({ top, r.getX(), r.getY(), colour.darker(0.55f), r.getRight(), r.getBottom(), false });
    g.fillRoundedRectangle(r, 2);
    g.setColour(colour.brighter(0.5f)); g.drawLine(on ? r.getRight()-4 : r.getX()+4, r.getY()+2,
        on ? r.getRight()-4 : r.getX()+4, r.getBottom()-2, 2);
    text(g, b.getButtonText(), { 0, 0, b.getWidth(), 22 }, 9, true);
    text(g, on ? "ON" : "OFF", { 0, 50, b.getWidth(), 14 }, 8);
}

void ModelDLook::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                float position, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    auto well = juce::Rectangle<float>(static_cast<float>(x+width/2-17), static_cast<float>(y), 34, static_cast<float>(height));
    g.setColour(juce::Colours::black); g.fillRoundedRectangle(well.expanded(3,1), 8);
    auto wheel = well.reduced(6,1);
    g.setGradientFill({ juce::Colour(0xff626056), wheel.getX(), 0, juce::Colour(0xff242523), wheel.getRight(), 0, false });
    g.fillRoundedRectangle(wheel, 9);
    for (int line = 4; line < height-4; line += 5)
    {
        const float yy=static_cast<float>(y+line);
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawLine(wheel.getX()+2, yy, wheel.getRight()-2, yy, 1);
        g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawLine(wheel.getX()+2, yy+1, wheel.getRight()-2, yy+1, 1);
    }
    g.setColour(ivory.withAlpha(0.8f)); g.drawLine(wheel.getX()+2, position, wheel.getRight()-2, position, 2);
}

EmberEditor::Knob::Knob(EmberProcessor& p, const juce::String& id, const juce::String& title, const juce::String& tip)
    : caption(title)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    slider.setTooltip(tip.isEmpty() ? title : tip);
    slider.setName(title);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, id, slider);
    slider.setDoubleClickReturnValue(true, p.state.getParameter(id)->convertFrom0to1(p.state.getParameter(id)->getDefaultValue()));
    if (id.contains("Attack") || id.contains("Decay"))
        slider.textFromValueFunction = [](double v) { return v < 1 ? juce::String(v*1000, 0) + " ms" : juce::String(v, 2) + " s"; };
    else if (id == "cutoff") slider.textFromValueFunction = [](double v) { return v < 1000 ? juce::String(v,0)+" Hz" : juce::String(v/1000,2)+" kHz"; };
    else if (id == "volume") slider.textFromValueFunction = [](double v) { return juce::String(v,1)+" dB"; };
    else if (id.contains("Tune")) slider.textFromValueFunction = [](double v) { return juce::String(v,2)+" st"; };
    else if (!id.contains("Wave") && !id.contains("Octave")) slider.textFromValueFunction = [](double v) { return juce::String(v,2); };
    // JUCE's parameter parser expects raw values; custom units are display-only.
    slider.setTextBoxIsEditable(false);
    slider.onValueChange = [this] { repaint(); };
}
void EmberEditor::Knob::paint(juce::Graphics& g)
{
    text(g, caption, { 0, 0, getWidth(), 18 }, 10, true);
    text(g, slider.getTextFromValue(slider.getValue()), { 0, getHeight()-17, getWidth(), 17 }, 11);
}
void EmberEditor::Knob::resized() { slider.setBounds(0, 17, getWidth(), getHeight()-34); }

void EmberEditor::addKnob(const char* id, const char* title, int x, int y, const char* tip)
{
    auto knob = std::make_unique<Knob>(processor, id, title, tip);
    panel.addAndMakeVisible(*knob); knob->setBounds(x, y, 88, 105);
    knobs.push_back(std::move(knob));
}
void EmberEditor::addSwitch(const char* id, const char* title, int x, int y, bool orange)
{
    auto button = std::make_unique<juce::ToggleButton>(title);
    button->getProperties().set("orange", orange);
    button->setTooltip(title);
    panel.addAndMakeVisible(*button); button->setBounds(x, y, 72, 68);
    switchAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state, id, *button));
    switches.push_back(std::move(button));
}

EmberEditor::EmberEditor(EmberProcessor& p)
    : AudioProcessorEditor(p), processor(p), keyboard(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&look);
    addAndMakeVisible(panel);
    addKnob("masterTune", "TUNE", 89, 123, "Master tuning in semitones; double-click to reset.");
    addKnob("glide", "GLIDE", 39, 262, "Portamento time between overlapping notes.");
    addKnob("modMix", "MODULATION MIX", 132, 262, "Blend oscillator 3 and noise as modulation sources. Raise the MOD wheel.");
    addSwitch("oscMod", "OSC. MODULATION", 143, 384, true);
    for (int i = 0; i < 3; ++i)
    {
        const auto prefix = "osc" + juce::String(i+1);
        int y = 98 + i*120;
        addKnob((prefix+"Octave").toRawUTF8(), "RANGE", 247, y);
        addKnob((prefix+"Tune").toRawUTF8(), "FREQUENCY", 346, y);
        addKnob((prefix+"Wave").toRawUTF8(), "WAVEFORM", 445, y);
        addKnob((prefix+"Level").toRawUTF8(), ("OSC. " + juce::String(i+1) + " VOLUME").toRawUTF8(), 551, y);
        addSwitch((prefix+"On").toRawUTF8(), "", 646, y+20);
    }
    addKnob("drive", "INPUT DRIVE", 726, 150, "Ladder saturation, replacing the hardware external input level.");
    addKnob("noise", "NOISE VOLUME", 726, 285);
    addSwitch("pink", "PINK NOISE", 736, 395);
    addSwitch("filterMod", "FILTER MOD.", 829, 116, true);
    addKnob("keyTrack", "KEYBOARD", 823, 220, "Filter keyboard tracking from zero to full.");
    addSwitch("osc3Track", "OSC. 3 CONTROL", 44, 384, true);
    addKnob("cutoff", "CUTOFF FREQUENCY", 916, 98);
    addKnob("resonance", "EMPHASIS", 1017, 98);
    addKnob("envAmount", "AMOUNT OF CONTOUR", 1118, 98);
    addKnob("filterAttack", "ATTACK TIME", 916, 220);
    addKnob("filterDecay", "DECAY TIME", 1017, 220);
    addKnob("filterSustain", "SUSTAIN LEVEL", 1118, 220);
    addKnob("ampAttack", "ATTACK TIME", 916, 347);
    addKnob("ampDecay", "DECAY TIME", 1017, 347);
    addKnob("ampSustain", "SUSTAIN LEVEL", 1118, 347);
    addKnob("volume", "MAIN VOLUME", 1234, 98);
    addSwitch("output", "MAIN OUTPUT", 1331, 124);
    addSwitch("a440", "A-440", 1242, 258);
    addSwitch("legato", "LEGATO", 1331, 258);
    addSwitch("decay", "DECAY", 119, 579);
    panel.addAndMakeVisible(keyboard);
    keyboard.setAvailableRange(53, 96); keyboard.setLowestVisibleKey(53);
    keyboard.setScrollButtonsVisible(false); keyboard.setKeyWidth(1183.0f / 26.0f);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffeeeade));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff111214));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff55544f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x40849892));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0x8086988b));
    keyboard.setMidiChannel(1);
    for (auto* wheel : { &pitchWheel, &modWheel })
    {
        panel.addAndMakeVisible(*wheel); wheel->setSliderStyle(juce::Slider::LinearVertical);
        wheel->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    }
    pitchWheel.setRange(-1, 1); pitchWheel.setValue(0); pitchWheel.setName("Pitch wheel");
    pitchWheel.onValueChange = [this] { processor.uiPitch.store(static_cast<float>(pitchWheel.getValue())); };
    pitchWheel.onDragEnd = [this] { pitchWheel.setValue(0); };
    pitchWheel.setTooltip("Pitch bend +/- 2 semitones. Springs back on release.");
    modWheel.setRange(0, 1); modWheel.setName("Modulation wheel");
    modWheel.onValueChange = [this] { processor.uiMod.store(static_cast<float>(modWheel.getValue())); };
    modWheel.setTooltip("Raise to apply oscillator 3 / noise modulation to enabled destinations.");
    panel.addAndMakeVisible(presets);
    for (int i = 0; i < p.getNumPrograms(); ++i) presets.addItem(p.getProgramName(i), i+1);
    presets.setSelectedId(p.getCurrentProgram()+1, juce::dontSendNotification);
    presets.onChange = [this] { processor.setCurrentProgram(presets.getSelectedId()-1); };
    panel.addAndMakeVisible(panic);
    panic.onClick = [this] { processor.keyboard.reset(); processor.panicRequested.store(true); };
    createWood();
    setResizable(true, true); setResizeLimits(1008, 560, 1800, 1000);
    getConstrainer()->setFixedAspectRatio(1440.0 / 800.0);
    setSize(1440, 800);
    startTimerHz(20);
}
EmberEditor::~EmberEditor() { processor.uiPitch.store(0); processor.uiMod.store(0); setLookAndFeel(nullptr); }

void EmberEditor::createWood()
{
    wood = juce::Image(juce::Image::RGB, 1440, 800, false);
    juce::Graphics g(wood);
    g.setGradientFill({ juce::Colour(0xff8b5732), 0, 0, juce::Colour(0xff4a2a17), 250, 800, false });
    g.fillAll();
    juce::Random r(1970);
    for (int i = 0; i < 2800; ++i)
    {
        float y = r.nextFloat()*800, x = r.nextFloat()*1440;
        juce::Path grain; grain.startNewSubPath(x, y);
        grain.cubicTo(x+90, y-3, x+150, y+4, x+220+r.nextFloat()*300, y+r.nextFloat()*6);
        g.setColour((i % 3 == 0 ? juce::Colour(0xffd79d62) : juce::Colour(0xff25160e)).withAlpha(0.05f+r.nextFloat()*0.17f));
        g.strokePath(grain, juce::PathStrokeType(0.4f+r.nextFloat()*1.1f));
    }
}

void EmberEditor::paint(juce::Graphics& canvas)
{
    juce::Graphics::ScopedSaveState saved(canvas);
    canvas.addTransform(juce::AffineTransform::scale(getWidth()/1440.0f, getHeight()/800.0f));
    auto& g = canvas;
    g.drawImageAt(wood, 0, 0);
    g.setColour(juce::Colour(0xff27180f)); g.drawRoundedRectangle({ 2, 2, 1436, 796 }, 8, 4);
    g.setColour(juce::Colour(0xffcb925b).withAlpha(0.4f)); g.drawLine(22, 10, 1418, 10, 2);
    g.setGradientFill({ juce::Colour(0xff252728), 0, 55, juce::Colour(0xff101112), 0, 505, false });
    g.fillRect(29, 55, 1382, 454);
    g.setColour(juce::Colour(0xffa3a29a)); g.drawRect(29, 55, 1382, 454, 2);
    g.setColour(ivory.withAlpha(0.7f));
    for (int x : { 228, 541, 819, 1218 }) g.drawVerticalLine(x, 56, 487);
    g.drawHorizontalLine(335, 908, 1208);
    g.drawHorizontalLine(81, 245, 529);
    g.drawHorizontalLine(207, 245, 529);
    g.drawHorizontalLine(327, 245, 529);
    text(g, "OSCILLATOR - 1", { 324, 62, 135, 20 }, 12);
    text(g, "OSCILLATOR - 2", { 324, 206, 135, 16 }, 10);
    text(g, "OSCILLATOR - 3", { 324, 326, 135, 16 }, 10);
    text(g, "FILTER", { 971, 69, 164, 20 }, 12, true);
    text(g, "LOUDNESS CONTOUR", { 947, 329, 238, 18 }, 11, true);
    text(g, "CONTROLLERS", { 37, 472, 182, 24 }, 17);
    text(g, "OSCILLATOR BANK", { 323, 472, 209, 24 }, 17);
    text(g, "MIXER", { 559, 472, 240, 24 }, 17);
    text(g, "MODIFIERS", { 914, 472, 293, 24 }, 17);
    text(g, "OUTPUT", { 1237, 472, 155, 24 }, 17);
    text(g, "OSC. 3        NOISE", { 132, 373, 90, 14 }, 8);
    // Piano hinge along the base of the raised control panel.
    g.setGradientFill({ juce::Colour(0xffaaa79b), 0, 508, juce::Colour(0xff44433e), 0, 517, false });
    g.fillRect(31, 508, 1378, 9);
    for (int x = 31; x < 1409; x += 17) { g.setColour(juce::Colour(0xff262522)); g.drawVerticalLine(x, 509, 516); }
    // Lower name plate and inset left-hand performance controls.
    g.setColour(juce::Colour(0xff121314)); g.fillRoundedRectangle(1125, 524, 270, 47, 2);
    text(g, "minimoog", { 1141, 526, 144, 28 }, 26, true);
    text(g, "MODEL D / 1970", { 1278, 529, 106, 22 }, 10, true);
    text(g, "Created by Given Peace", { 1141, 554, 243, 14 }, 11);
    g.setColour(juce::Colour(0xff202222)); g.fillRect(31, 578, 185, 198);
    text(g, "PITCH", { 42, 745, 62, 20 }, 10); text(g, "MOD", { 119, 745, 62, 20 }, 10);
    text(g, "MONOPHONIC", { 40, 583, 68, 20 }, 9);
    text(g, "THREE OSCILLATOR SYNTHESIZER", { 35, 535, 310, 24 }, 11);
    text(g, "MODEL D", { 41, 9, 126, 26 }, 20, true);
    text(g, "1970", { 168, 13, 50, 20 }, 12);
    text(g, "Created by Given Peace", { 41, 34, 177, 14 }, 11);
    // Fasteners at the cabinet corners.
    for (auto point : { juce::Point<float>(17, 30), {1423,30}, {17,780}, {1423,780}, {40,66}, {1400,66} })
    {
        g.setColour(juce::Colour(0xff171310)); g.fillEllipse(point.x-4, point.y-4, 8, 8);
        g.setColour(juce::Colour(0xff6a6258)); g.drawLine(point.x-2, point.y-1, point.x+2, point.y+1, 1);
    }
    g.setColour(juce::Colour(0xff080a08)); g.fillRoundedRectangle(1253, 373, 129, 34, 3);
    for (int i = 0; i < 13; ++i)
    {
        const bool lit = processor.peak.load() > std::pow(10.0f, (-48.0f + i*3.5f) / 20.0f);
        g.setColour((i > 10 ? juce::Colour(0xffe69455) : juce::Colour(0xffa3bb7b)).withAlpha(lit ? 1.0f : 0.13f));
        g.fillRect(1260 + i*9, 382, 6, 16);
    }
    text(g, "OUTPUT LEVEL", { 1250, 410, 140, 20 }, 10);
}

void EmberEditor::resized()
{
    panel.setBounds(0, 0, 1440, 800);
    panel.setTransform(juce::AffineTransform::scale(getWidth()/1440.0f, getHeight()/800.0f));
    presets.setBounds(1016, 15, 259, 28); panic.setBounds(1285, 15, 124, 28);
    keyboard.setBounds(226, 578, 1183, 198);
    pitchWheel.setBounds(47, 641, 52, 104); modWheel.setBounds(124, 641, 52, 104);
}
void EmberEditor::timerCallback()
{
    presets.setSelectedId(processor.getCurrentProgram()+1, juce::dontSendNotification);
    repaint();
}
