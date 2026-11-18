#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace agoui {

namespace col {
    const juce::Colour bg       { 0xff11131a };
    const juce::Colour panel    { 0xff1a1e29 };
    const juce::Colour panel2   { 0xff222736 };
    const juce::Colour line     { 0xff2e3446 };
    const juce::Colour text     { 0xffdde1ea };
    const juce::Colour dim      { 0xff7c8497 };
    const juce::Colour pad      { 0xff3fc1c9 };
    const juce::Colour drone    { 0xff9b6dff };
    const juce::Colour m1       { 0xfff5c518 };
    const juce::Colour m2       { 0xffff7a45 };
    const juce::Colour mod      { 0xffff4d5e };
    const juce::Colour blue     { 0xff4d8dff };
}
inline juce::Colour partColour(int p) { const juce::Colour c[4] = { col::pad, col::drone, col::m1, col::m2 }; return c[juce::jlimit(0, 3, p)]; }

class LnF : public juce::LookAndFeel_V4 {
public:
    LnF();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::Font(13.0f); }
    juce::Font getTextButtonFont(juce::TextButton&, int h) override { return juce::Font(juce::jmin(14.0f, h * 0.55f), juce::Font::bold); }
    void drawTabButton(juce::TabBarButton&, juce::Graphics&, bool over, bool down) override;
};

// Manopola collegata a un parametro dell'host
class Knob : public juce::Component {
public:
    Knob(AgoProcessor& p, const juce::String& paramId, const juce::String& label, juce::Colour c);
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::Slider slider;
private:
    juce::String label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class ParamToggle : public juce::TextButton {
public:
    ParamToggle(AgoProcessor& p, const juce::String& paramId, const juce::String& text, juce::Colour on);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

class ParamCombo : public juce::ComboBox {
public:
    ParamCombo(AgoProcessor& p, const juce::String& paramId);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

class Page;

} // namespace agoui

class AgoEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit AgoEditor(AgoProcessor&);
    ~AgoEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    AgoProcessor& proc;
    ago::Config cfg;               // copia locale della configurazione
    ago::UiSnapshot snap;
    void pushConfig() { proc.setConfig(cfg); }
    void showTab(int i) { tabs.setCurrentTabIndex(i); }

private:
    void timerCallback() override;
    void layoutParts(juce::Rectangle<int>);

    agoui::LnF lnf;
    int seenSerial = 0;

    // header
    agoui::ParamCombo syncBox, keyBox, modeBox;
    juce::Slider tempo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tempoAtt;
    agoui::ParamToggle playBtn, modAllBtn, invertBtn;
    juce::TextButton playAllBtn { "PLAY ALL" }, resyncBtn { "RESYNC" }, panicBtn { "PANIC" };
    juce::TextButton chordBtn[7], typeBtn[7];

    // parti
    agoui::ParamToggle partOn[4], partMod[4];
    juce::OwnedArray<agoui::Knob> knobs[4];
    std::unique_ptr<juce::Component> motifView[2];
    std::unique_ptr<juce::Component> keyboard;
    std::unique_ptr<juce::Component> followerOverlay;

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::Array<agoui::Page*> pages;
    juce::Rectangle<int> partArea[4], chordArea, headerArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgoEditor)
};

namespace agoui {
class Page : public juce::Component {
public:
    explicit Page(AgoEditor& e) : ed(e) {}
    virtual void refresh() {}        // configurazione cambiata dall'esterno
    virtual void tick() {}           // ~25 Hz, per gli indicatori
protected:
    AgoEditor& ed;
};
}
