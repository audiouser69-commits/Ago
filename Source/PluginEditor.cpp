#include "PluginEditor.h"
#include "ParamTable.h"

using namespace agoui;
using namespace ago;

// =====================================================================================  LookAndFeel
LnF::LnF() {
    setColour(juce::ResizableWindow::backgroundColourId, col::bg);
    setColour(juce::ComboBox::backgroundColourId, col::panel2);
    setColour(juce::ComboBox::outlineColourId, col::line);
    setColour(juce::ComboBox::textColourId, col::text);
    setColour(juce::ComboBox::arrowColourId, col::dim);
    setColour(juce::PopupMenu::backgroundColourId, col::panel2);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, col::blue.withAlpha(0.5f));
    setColour(juce::PopupMenu::textColourId, col::text);
    setColour(juce::PopupMenu::headerTextColourId, col::m1);
    setColour(juce::Label::textColourId, col::text);
    setColour(juce::Slider::textBoxTextColourId, col::text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::backgroundColourId, col::panel2);
    setColour(juce::Slider::trackColourId, col::blue);
    setColour(juce::Slider::thumbColourId, col::text);
    setColour(juce::TextButton::buttonColourId, col::panel2);
    setColour(juce::TextButton::textColourOffId, col::text);
    setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    setColour(juce::TabbedComponent::backgroundColourId, col::panel);
    setColour(juce::TabbedComponent::outlineColourId, col::line);
    setColour(juce::TabbedButtonBar::tabOutlineColourId, col::line);
    setColour(juce::ToggleButton::textColourId, col::text);
    setColour(juce::ToggleButton::tickColourId, col::m1);
}

void LnF::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) {
    auto r = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.0f);
    const float d = juce::jmin(r.getWidth(), r.getHeight());
    auto c = r.withSizeKeepingCentre(d, d);
    const auto accent = s.findColour(juce::Slider::rotarySliderFillColourId);
    const float ang = a0 + pos * (a1 - a0);
    g.setColour(col::panel2);
    g.fillEllipse(c.reduced(d * 0.16f));
    juce::Path bgArc, arc;
    bgArc.addCentredArc(c.getCentreX(), c.getCentreY(), d * 0.45f, d * 0.45f, 0, a0, a1, true);
    arc.addCentredArc(c.getCentreX(), c.getCentreY(), d * 0.45f, d * 0.45f, 0, a0, ang, true);
    g.setColour(col::line);
    g.strokePath(bgArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Point<float> p1 = c.getCentre().getPointOnCircumference(d * 0.12f, ang);
    juce::Point<float> p2 = c.getCentre().getPointOnCircumference(d * 0.32f, ang);
    g.setColour(col::text);
    g.drawLine({ p1, p2 }, 2.0f);
}

void LnF::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& bc, bool over, bool down) {
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    auto c = b.getToggleState() ? b.findColour(juce::TextButton::buttonOnColourId) : bc;
    if (over) c = c.brighter(0.15f);
    if (down) c = c.darker(0.2f);
    g.setColour(c);
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(col::line);
    g.drawRoundedRectangle(r, 5.0f, 1.0f);
}

void LnF::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool) {
    auto r = b.getLocalBounds().toFloat();
    auto box = r.removeFromLeft(r.getHeight()).reduced(4.0f);
    g.setColour(col::panel2); g.fillRoundedRectangle(box, 3.0f);
    g.setColour(over ? col::text : col::line); g.drawRoundedRectangle(box, 3.0f, 1.0f);
    if (b.getToggleState()) { g.setColour(b.findColour(juce::ToggleButton::tickColourId)); g.fillRoundedRectangle(box.reduced(3.0f), 2.0f); }
    g.setColour(col::text); g.setFont(13.0f);
    g.drawText(b.getButtonText(), r.withTrimmedLeft(4), juce::Justification::centredLeft);
}

void LnF::drawTabButton(juce::TabBarButton& b, juce::Graphics& g, bool over, bool) {
    auto r = b.getLocalBounds().toFloat().reduced(2.0f, 3.0f);
    const bool front = b.isFrontTab();
    g.setColour(front ? col::panel2 : (over ? col::panel2.withAlpha(0.5f) : col::panel));
    g.fillRoundedRectangle(r, 4.0f);
    if (front) { g.setColour(col::m1); g.fillRect(r.removeFromBottom(2.0f)); }
    g.setColour(front ? col::text : col::dim);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

// =====================================================================================  controlli
Knob::Knob(AgoProcessor& p, const juce::String& id, const juce::String& l, juce::Colour c) : label(l) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 90, 15);
    slider.setColour(juce::Slider::rotarySliderFillColourId, c);
    slider.setScrollWheelEnabled(true);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, id, slider);
}
void Knob::resized() { slider.setBounds(getLocalBounds().withTrimmedTop(14)); }
void Knob::paint(juce::Graphics& g) {
    g.setColour(col::dim); g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(label.toUpperCase(), getLocalBounds().removeFromTop(14), juce::Justification::centred);
}

ParamToggle::ParamToggle(AgoProcessor& p, const juce::String& id, const juce::String& text, juce::Colour on) : juce::TextButton(text) {
    setClickingTogglesState(true);
    setColour(juce::TextButton::buttonOnColourId, on);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, id, *this);
}

ParamCombo::ParamCombo(AgoProcessor& p, const juce::String& id) {
    if (auto* prm = dynamic_cast<juce::AudioParameterChoice*>(p.apvts.getParameter(id)))
        addItemList(prm->choices, 1);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, id, *this);
}

static void fillCombo(juce::ComboBox& c, const juce::StringArray& items) { c.clear(juce::dontSendNotification); c.addItemList(items, 1); }
static juce::StringArray rangeItems(int a, int b, const juce::String& pre = {}) { juce::StringArray s; for (int i = a; i <= b; ++i) s.add(pre + juce::String(i)); return s; }

static juce::Label* mkLabel(juce::Component& parent, juce::OwnedArray<juce::Label>& store, const juce::String& t, float size = 12.0f, juce::Colour c = col::dim) {
    auto* l = store.add(new juce::Label({}, t));
    l->setFont(juce::Font(size, juce::Font::bold));
    l->setColour(juce::Label::textColourId, c);
    parent.addAndMakeVisible(l);
    return l;
}

// =====================================================================================  tastiera visiva
class KeyboardView : public juce::Component {
public:
    explicit KeyboardView(AgoEditor& e) : ed(e) {}
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(col::panel); g.fillRoundedRectangle(r, 6.0f);
        r = r.reduced(6.0f, 4.0f);
        const int lo = 24, hi = 108;
        int whites = 0; for (int n = lo; n <= hi; ++n) if (!isBlack(n)) ++whites;
        const float ww = r.getWidth() / (float) whites;
        const auto& s = ed.snap;
        const int keyPc = s.chord.keyPc;
        const auto& sc = modeScale(s.chord.mode);
        auto inScale = [&](int n) { int rel = ((n - keyPc) % 12 + 12) % 12; return std::find(sc.begin(), sc.end(), rel) != sc.end(); };
        // bianchi
        int wi = 0; std::map<int, juce::Rectangle<float>> rects;
        for (int n = lo; n <= hi; ++n) {
            if (isBlack(n)) continue;
            auto k = juce::Rectangle<float>(r.getX() + wi * ww, r.getY(), ww - 1.0f, r.getHeight());
            g.setColour(pcIn(s.chord.pcs, n % 12) ? juce::Colour(0xffd9dce6) : juce::Colour(0xffa9adb8));
            g.fillRect(k);
            rects[n] = k; ++wi;
        }
        for (int n = lo; n <= hi; ++n) {
            if (!isBlack(n)) continue;
            auto left = rects[n - 1];
            auto k = juce::Rectangle<float>(left.getRight() - ww * 0.32f, r.getY(), ww * 0.62f, r.getHeight() * 0.6f);
            g.setColour(pcIn(s.chord.pcs, n % 12) ? juce::Colour(0xff4a5064) : juce::Colour(0xff1c1f28));
            g.fillRect(k);
            rects[n] = k;
        }
        // punti scala
        for (int n = lo; n <= hi; ++n) {
            if (!inScale(n)) continue;
            auto k = rects[n];
            g.setColour(col::blue.withAlpha(0.55f));
            g.fillEllipse(k.getCentreX() - 2, k.getBottom() - 7, 4, 4);
        }
        // note suonate
        for (int p = 0; p < 4; ++p)
            for (int n = lo; n <= hi; ++n)
                if (s.sounding[p][n]) {
                    auto k = rects[n];
                    const float d = juce::jmin(k.getWidth() - 2.0f, 10.0f);
                    const float y = k.getY() + 4 + p * (d + 2);
                    g.setColour(partColour(p)); g.fillEllipse(k.getCentreX() - d / 2, y, d, d);
                }
        g.setColour(col::dim); g.setFont(10.0f);
        for (int n = lo; n <= hi; n += 12) g.drawText(noteName(n), rects[n].withTrimmedTop(rects[n].getHeight() - 22).toNearestInt(), juce::Justification::centredTop);
    }
    static bool isBlack(int n) { int pc = n % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
private:
    AgoEditor& ed;
};

// =====================================================================================  mini-display motif
class MotifView : public juce::Component {
public:
    MotifView(AgoEditor& e, int k) : ed(e), m(k) {}
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(col::bg); g.fillRoundedRectangle(r, 4.0f);
        r = r.reduced(6.0f, 4.0f);
        const auto& p = ed.snap.eff.m[m];
        const juce::Colour c = partColour(2 + m);
        // pattern
        auto pr = r.removeFromTop(r.getHeight() * 0.58f);
        int vals[16]; int type = PT_CHORD;
        if (p.patt >= kPatternNZ) { for (int i = 0; i < 16; ++i) vals[i] = ed.snap.nz[m][i]; type = ed.cfg.nzPool[m]; }
        else { const auto& pt = ed.cfg.patterns[juce::jlimit(0, 39, p.patt)]; for (int i = 0; i < 16; ++i) vals[i] = pt.v[i]; type = pt.type; }
        int mx = 1; for (int v : vals) mx = juce::jmax(mx, v);
        const float bw = pr.getWidth() / 16.0f;
        for (int i = 0; i < 16; ++i) {
            const bool in = i < p.plen;
            const float h = juce::jmax(2.0f, pr.getHeight() * vals[i] / (float) mx);
            auto b = juce::Rectangle<float>(pr.getX() + i * bw + 1, pr.getBottom() - h, bw - 2, h);
            g.setColour(i == ed.snap.motifStep[m] && ed.snap.partRunning[2 + m] ? juce::Colours::white : (in ? c : col::line));
            g.fillRect(b);
        }
        g.setColour(col::dim); g.setFont(10.0f);
        g.drawText(juce::String(patTypeName(type)) + (p.patt >= kPatternNZ ? "  NZ" : "  P" + juce::String(p.patt + 1)), pr.toNearestInt(), juce::Justification::topLeft);
        // ritmo
        r.removeFromTop(4.0f);
        const auto& rh = ed.cfg.rhythms[juce::jlimit(0, 39, p.rhy)];
        const int len = juce::jlimit(4, 32, p.rlen);
        const float cw = r.getWidth() / 32.0f;
        int noteBeats = 0;
        for (int i = 0; i < 32; ++i) {
            int v = rh.euclid ? (Engine::euclidHit(i, len, rh.hits, rh.rot) ? 100 : kRest) : rh.v[i];
            auto b = juce::Rectangle<float>(r.getX() + i * cw + 1, r.getY(), cw - 2, r.getHeight());
            const bool in = i < len;
            if (in && v > 0) ++noteBeats;
            juce::Colour cc = !in ? col::panel2 : v == kRest ? col::panel2.brighter(0.1f) : v == kTie ? juce::Colour(0xff3fbf6f) : c.withAlpha(0.35f + 0.65f * v / 127.0f);
            if (in && i == ed.snap.motifBeat[m] && ed.snap.partRunning[2 + m]) cc = juce::Colours::white;
            g.setColour(cc); g.fillRect(b);
        }
        g.setColour(col::dim);
        g.drawText(juce::String(p.plen) + ":" + juce::String(noteBeats), getLocalBounds().reduced(6, 2), juce::Justification::topRight);
    }
private:
    AgoEditor& ed; int m;
};

// =====================================================================================  overlay follower
class FollowerOverlay : public juce::Component {
public:
    explicit FollowerOverlay(AgoEditor& e) : ed(e), role(e.proc, "linkRole"), group(e.proc, "linkGroup"), part(e.proc, "linkPart") {
        for (auto* c : { &role, &group, &part }) addAndMakeVisible(c);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(col::bg);
        auto r = getLocalBounds().reduced(30, 20);
        g.setColour(col::panel); g.fillRoundedRectangle(r.toFloat(), 10.0f);
        r = r.reduced(30, 24);
        g.setColour(col::text); g.setFont(juce::Font(26.0f, juce::Font::bold));
        g.drawText("AGO FOLLOWER", r.removeFromTop(36), juce::Justification::centredLeft);
        r.removeFromTop(50);
        const int st = ed.proc.getLinkStatus();
        juce::String status = st == 3 ? "Collegato al leader: segue accordi, tempo e sequencer."
                            : st == 2 ? "C'e' gia' un leader in questo gruppo: questa istanza fa da follower."
                            : st == 5 ? "In attesa del trasporto dell'host (premi Play in Live)."
                            : "Nessun leader in questo gruppo: imposta Link = Leader su un'altra istanza di AGO.";
        g.setColour(st == 3 || st == 2 ? col::m1 : col::mod);
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText(status, r.removeFromTop(26), juce::Justification::centredLeft);
        r.removeFromTop(14);
        g.setColour(col::text); g.setFont(14.0f);
        g.drawFittedText(
            "Questa istanza non genera note proprie: legge il motore del leader dello stesso gruppo e manda all'uscita MIDI "
            "della traccia solo la parte scelta, a tempo e senza latenza.\n\n"
            "In Ableton Live:\n"
            "1.  Traccia 'AGO leader' con Link = Leader: accordi, parti, pattern e modulazioni si regolano li'.\n"
            "2.  Una traccia per parte con AGO in Link = Follower (es. Pad, Motif 1...).\n"
            "3.  Sulla traccia del synth (DeepMind, Pro-800, emulatore...): MIDI From = la traccia del follower, sotto scegli 'AGO'. "
            "Monitor In, oppure arma per registrare le note in clip.\n\n"
            "Le parti devono essere accese nel leader. Il canale MIDI lo decide la traccia del synth (Live unifica i canali).",
            r, juce::Justification::topLeft, 20);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(60, 44).withTrimmedTop(46).removeFromTop(30);
        role.setBounds(r.removeFromLeft(140)); r.removeFromLeft(10);
        group.setBounds(r.removeFromLeft(140)); r.removeFromLeft(10);
        part.setBounds(r.removeFromLeft(160));
    }
private:
    AgoEditor& ed;
    ParamCombo role, group, part;
};

// =====================================================================================  pagine
namespace {

// ---------- matrice di modulazione
class ModPage : public Page {
public:
    explicit ModPage(AgoEditor& e) : Page(e) {
        juce::StringArray srcs; for (int s = 0; s < kNumSrc; ++s) srcs.add(srcName(s));
        for (int i = 0; i < 8; ++i) {
            auto& r = rows[i];
            mkLabel(*this, labels, "SLOT " + juce::String(i + 1), 12.0f, col::text);
            fillCombo(r.src, srcs);
            r.dst.clear();
            int lastSection = -2;
            for (int d = 0; d < kNumDests; ++d) {
                auto di = destInfo(d);
                int section = d >= D_CC1 ? 10 : d >= D_M1_STEP1 ? 9 : di.part;
                if (section != lastSection && d > 0) {
                    static const char* sn[] = {"PAD","DRONE","MOTIF 1","MOTIF 2"};
                    r.dst.addSectionHeading(section == 10 ? "MIDI CC" : section == 9 ? "STEP" : section < 0 ? "GLOBALE" : sn[section]);
                    lastSection = section;
                }
                r.dst.addItem(di.name, d + 1);
            }
            r.amt.setSliderStyle(juce::Slider::LinearHorizontal);
            r.amt.setRange(0, 100, 1);
            r.amt.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18);
            r.amt.setTextValueSuffix("%");
            fillCombo(r.ccCh, rangeItems(1, 16, "Ch "));
            juce::StringArray ports; for (int p = 0; p < kNumPortChoices; ++p) ports.add(portName(p));
            fillCombo(r.ccPort, ports);
            for (juce::Component* c : { (juce::Component*) &r.src, (juce::Component*) &r.dst, (juce::Component*) &r.amt, (juce::Component*) &r.ccCh, (juce::Component*) &r.ccPort })
                addAndMakeVisible(c);
            r.src.onChange = [this, i] { ed.cfg.mods[i].src = rows[i].src.getSelectedId() - 1; ed.pushConfig(); };
            r.dst.onChange = [this, i] { ed.cfg.mods[i].dst = rows[i].dst.getSelectedId() - 1; ed.pushConfig(); updateVis(i); };
            r.amt.onValueChange = [this, i] { ed.cfg.mods[i].amount = (int) rows[i].amt.getValue(); ed.pushConfig(); };
            r.ccCh.onChange = [this, i] { ed.cfg.mods[i].ccChan = rows[i].ccCh.getSelectedId(); ed.pushConfig(); };
            r.ccPort.onChange = [this, i] { ed.cfg.mods[i].ccPort = rows[i].ccPort.getSelectedId() - 1; ed.pushConfig(); };
        }
        auto* h = mkLabel(*this, labels, "Sorgenti: LFO 1-3 (tab LFO), MOD 1-5 preset, ingressi MIDI sul canale di controllo.  "
                                       "Sine = bipolare attorno al valore, le altre forme salgono sopra il valore. PATT sostituisce il valore con i passi del pattern.", 11.0f);
        h->setJustificationType(juce::Justification::topLeft);
        help = h;
        refresh();
    }
    void refresh() override {
        for (int i = 0; i < 8; ++i) {
            auto& m = ed.cfg.mods[i]; auto& r = rows[i];
            r.src.setSelectedId(m.src + 1, juce::dontSendNotification);
            r.dst.setSelectedId(m.dst + 1, juce::dontSendNotification);
            r.amt.setValue(m.amount, juce::dontSendNotification);
            r.ccCh.setSelectedId(m.ccChan, juce::dontSendNotification);
            r.ccPort.setSelectedId(m.ccPort + 1, juce::dontSendNotification);
            updateVis(i);
        }
    }
    void updateVis(int i) { const bool cc = ed.cfg.mods[i].dst >= D_CC1; rows[i].ccCh.setVisible(cc); rows[i].ccPort.setVisible(cc); }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        help->setBounds(r.removeFromBottom(34));
        const int colW = r.getWidth() / 2;
        for (int i = 0; i < 8; ++i) {
            auto cell = juce::Rectangle<int>(r.getX() + (i / 4) * colW, r.getY() + (i % 4) * (r.getHeight() / 4), colW - 10, r.getHeight() / 4).reduced(0, 4);
            auto& row = rows[i];
            labels[i]->setBounds(cell.removeFromLeft(58));
            row.src.setBounds(cell.removeFromLeft(110).withSizeKeepingCentre(110, 26)); cell.removeFromLeft(6);
            row.dst.setBounds(cell.removeFromLeft(150).withSizeKeepingCentre(150, 26)); cell.removeFromLeft(6);
            if (row.ccCh.isVisible()) {
                row.ccPort.setBounds(cell.removeFromRight(78).withSizeKeepingCentre(78, 26)); cell.removeFromRight(4);
                row.ccCh.setBounds(cell.removeFromRight(68).withSizeKeepingCentre(68, 26)); cell.removeFromRight(4);
            }
            row.amt.setBounds(cell.withSizeKeepingCentre(cell.getWidth(), 26));
        }
    }
private:
    struct Row { juce::ComboBox src, dst, ccCh, ccPort; juce::Slider amt; } rows[8];
    juce::OwnedArray<juce::Label> labels;
    juce::Label* help = nullptr;
};

// ---------- LFO
class LfoPage : public Page {
public:
    explicit LfoPage(AgoEditor& e) : Page(e) {
        juce::StringArray shapes; for (int s = 0; s < kNumShapes; ++s) shapes.add(shapeName(s));
        for (int i = 0; i < 3; ++i) {
            auto& r = rows[i];
            mkLabel(*this, labels, "LFO " + juce::String(i + 1), 14.0f, col::text);
            r.sync.setButtonText("Sync al tempo");
            r.rate.setSliderStyle(juce::Slider::LinearHorizontal);
            r.rate.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 18);
            r.rate.textFromValueFunction = [this, i](double v) {
                const bool s = ed.cfg.lfo[i].synced != 0;
                const double x = lfoRateValue(s, (int) v);
                return s ? juce::String((int) x) + " beat" : juce::String(x, 1) + " s";
            };
            fillCombo(r.shape, shapes);
            fillCombo(r.patt, rangeItems(1, 40, "PATT "));
            r.prob.setSliderStyle(juce::Slider::LinearHorizontal);
            r.prob.setRange(1, 100, 1); r.prob.setTextValueSuffix("% prob");
            r.prob.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 18);
            for (juce::Component* c : { (juce::Component*) &r.sync, (juce::Component*) &r.rate, (juce::Component*) &r.shape, (juce::Component*) &r.patt, (juce::Component*) &r.prob })
                addAndMakeVisible(c);
            r.sync.onClick = [this, i] { auto& l = ed.cfg.lfo[i]; l.synced = rows[i].sync.getToggleState(); l.rateIdx = juce::jmin(l.rateIdx, lfoRateCount(l.synced) - 1); ed.pushConfig(); refresh(); };
            r.rate.onValueChange = [this, i] { ed.cfg.lfo[i].rateIdx = (int) rows[i].rate.getValue(); ed.pushConfig(); };
            r.shape.onChange = [this, i] { ed.cfg.lfo[i].shape = rows[i].shape.getSelectedId() - 1; rows[i].patt.setEnabled(ed.cfg.lfo[i].shape == SH_PATT); ed.pushConfig(); };
            r.patt.onChange = [this, i] { ed.cfg.lfo[i].patt = rows[i].patt.getSelectedId() - 1; ed.pushConfig(); };
            r.prob.onValueChange = [this, i] { ed.cfg.lfo[i].prob = (int) rows[i].prob.getValue(); ed.pushConfig(); };
        }
        rows[2].prob.setEnabled(false);
        auto* h = mkLabel(*this, labels, "MOD preset (fissi): MOD 1 = 40 s triangolo 50%   MOD 2 = 48 beat sine 100%   MOD 3 = 32 beat rampa 90%   "
                                       "MOD 4 = S&H ogni 8 beat 75%   MOD 5 = 8 beat quadra 15%.   La probabilita' si rilancia a ogni ciclo.", 11.0f);
        help = h;
        refresh();
    }
    void refresh() override {
        for (int i = 0; i < 3; ++i) {
            auto& l = ed.cfg.lfo[i]; auto& r = rows[i];
            r.sync.setToggleState(l.synced != 0, juce::dontSendNotification);
            r.rate.setRange(0, lfoRateCount(l.synced != 0) - 1, 1);
            r.rate.setValue(l.rateIdx, juce::dontSendNotification);
            r.rate.updateText();
            r.shape.setSelectedId(l.shape + 1, juce::dontSendNotification);
            r.patt.setSelectedId(l.patt + 1, juce::dontSendNotification);
            r.patt.setEnabled(l.shape == SH_PATT);
            r.prob.setValue(i == 2 ? 100 : l.prob, juce::dontSendNotification);
        }
    }
    void tick() override { repaint(meterArea); }
    void paint(juce::Graphics& g) override {
        for (int i = 0; i < 3; ++i) {
            auto m = meters[i];
            g.setColour(col::bg); g.fillRoundedRectangle(m.toFloat(), 3.0f);
            double v = ed.snap.lfo[i];
            const bool bip = ed.cfg.lfo[i].shape == SH_SINE;
            float x = (float) (bip ? (v * 0.5 + 0.5) : v);
            g.setColour(col::blue);
            g.fillRect(m.withWidth(juce::jmax(2, (int) (m.getWidth() * juce::jlimit(0.0f, 1.0f, x)))).reduced(0, 3));
        }
    }
    void resized() override {
        auto r = getLocalBounds().reduced(12);
        help->setBounds(r.removeFromBottom(30));
        meterArea = {};
        for (int i = 0; i < 3; ++i) {
            auto row = r.removeFromTop(56).reduced(0, 10);
            labels[i]->setBounds(row.removeFromLeft(60));
            rows[i].sync.setBounds(row.removeFromLeft(130));
            rows[i].rate.setBounds(row.removeFromLeft(280)); row.removeFromLeft(10);
            rows[i].shape.setBounds(row.removeFromLeft(120)); row.removeFromLeft(8);
            rows[i].patt.setBounds(row.removeFromLeft(100)); row.removeFromLeft(10);
            rows[i].prob.setBounds(row.removeFromLeft(240)); row.removeFromLeft(14);
            meters[i] = row.removeFromLeft(juce::jmin(200, row.getWidth()));
            meterArea = meterArea.getUnion(meters[i]);
        }
    }
private:
    struct Row { juce::ToggleButton sync; juce::Slider rate, prob; juce::ComboBox shape, patt; } rows[3];
    juce::Rectangle<int> meters[3], meterArea;
    juce::OwnedArray<juce::Label> labels;
    juce::Label* help = nullptr;
};

// ---------- editor a barre (pattern)
class BarEditor : public juce::Component {
public:
    std::function<void()> onEdit;
    int* values = nullptr; int count = 16; int maxV = 20; int highlight = -1; int activeLen = 16;
    juce::Colour colour = col::m1;
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(col::bg); g.fillRoundedRectangle(r, 4.0f);
        if (!values) return;
        r = r.reduced(6.0f);
        const float bw = r.getWidth() / count;
        for (int i = 0; i < count; ++i) {
            const float h = r.getHeight() * values[i] / (float) maxV;
            auto b = juce::Rectangle<float>(r.getX() + i * bw + 2, r.getBottom() - h, bw - 4, h);
            g.setColour(i == highlight ? juce::Colours::white : (i < activeLen ? colour : colour.withAlpha(0.3f)));
            g.fillRect(b);
            g.setColour(col::text); g.setFont(11.0f);
            g.drawText(juce::String(values[i]), juce::Rectangle<float>(b.getX(), r.getY(), b.getWidth(), 14), juce::Justification::centred);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override { edit(e); }
    void mouseDrag(const juce::MouseEvent& e) override { edit(e); }
private:
    void edit(const juce::MouseEvent& e) {
        if (!values) return;
        auto r = getLocalBounds().toFloat().reduced(6.0f);
        int i = (int) ((e.position.x - r.getX()) / (r.getWidth() / count));
        if (i < 0 || i >= count) return;
        int v = juce::roundToInt((r.getBottom() - e.position.y) / r.getHeight() * maxV);
        v = juce::jlimit(1, maxV, v);
        if (values[i] != v) { values[i] = v; repaint(); if (onEdit) onEdit(); }
    }
};

class PatternPage : public Page {
public:
    explicit PatternPage(AgoEditor& e) : Page(e) {
        juce::StringArray items; for (int i = 0; i < 40; ++i) items.add("Pattern " + juce::String(i + 1) + (i < 20 ? "  (fabbrica)" : i == 39 ? "  (Rotator)" : ""));
        fillCombo(sel, items); sel.setSelectedId(1, juce::dontSendNotification);
        fillCombo(type, { "CHORD (1-20)", "SCALE (1-40)", "CHROMATIC (1-60)" });
        fillCombo(copyTo, rangeItems(21, 40, "-> "));
        for (juce::Component* c : { (juce::Component*) &sel, (juce::Component*) &type, (juce::Component*) &copyTo, (juce::Component*) &copyBtn,
                                    (juce::Component*) &useM1, (juce::Component*) &useM2, (juce::Component*) &resetBtn, (juce::Component*) &bars })
            addAndMakeVisible(c);
        sel.onChange = [this] { refresh(); };
        type.onChange = [this] { auto& p = cur(); p.type = type.getSelectedId() - 1; for (int& v : p.v) v = juce::jlimit(1, patTypeMax(p.type), v); ed.pushConfig(); refresh(); };
        bars.onEdit = [this] { ed.pushConfig(); };
        copyBtn.onClick = [this] { int dst = copyTo.getSelectedId() + 19; if (dst >= 20) { ed.cfg.patterns[dst] = cur(); ed.pushConfig(); sel.setSelectedId(dst + 1); } };
        useM1.onClick = [this] { ed.proc.setParam("m1Patt", sel.getSelectedId() - 1); };
        useM2.onClick = [this] { ed.proc.setParam("m2Patt", sel.getSelectedId() - 1); };
        resetBtn.onClick = [this] { Config f; loadFactory(f); int i = sel.getSelectedId() - 1; ed.cfg.patterns[i] = f.patterns[i]; ed.pushConfig(); refresh(); };
        copyTo.setSelectedId(1, juce::dontSendNotification);
        mkLabel(*this, labels, "Trascina sulle barre per scrivere i valori. CHORD segue l'accordo, SCALE e CHROMATIC restano fissi sulla tonica.", 11.0f);
        refresh();
    }
    Pattern& cur() { return ed.cfg.patterns[juce::jlimit(0, 39, sel.getSelectedId() - 1)]; }
    void refresh() override {
        auto& p = cur();
        type.setSelectedId(p.type + 1, juce::dontSendNotification);
        bars.values = p.v; bars.maxV = patTypeMax(p.type);
        bars.repaint();
    }
    void tick() override {
        const int i = sel.getSelectedId() - 1;
        int hl = -1, len = 16;
        for (int k = 0; k < 2; ++k)
            if (ed.snap.eff.m[k].patt == i) { len = ed.snap.eff.m[k].plen; if (ed.snap.partRunning[2 + k]) hl = ed.snap.motifStep[k]; bars.colour = partColour(2 + k); break; }
        if (hl != bars.highlight || len != bars.activeLen) { bars.highlight = hl; bars.activeLen = len; bars.repaint(); }
    }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        labels[0]->setBounds(r.removeFromBottom(22));
        auto top = r.removeFromTop(30);
        sel.setBounds(top.removeFromLeft(220)); top.removeFromLeft(8);
        type.setBounds(top.removeFromLeft(160)); top.removeFromLeft(16);
        useM1.setBounds(top.removeFromLeft(90)); top.removeFromLeft(6);
        useM2.setBounds(top.removeFromLeft(90)); top.removeFromLeft(16);
        copyTo.setBounds(top.removeFromLeft(80)); top.removeFromLeft(4);
        copyBtn.setBounds(top.removeFromLeft(70)); top.removeFromLeft(16);
        resetBtn.setBounds(top.removeFromLeft(130));
        r.removeFromTop(8);
        bars.setBounds(r);
    }
private:
    juce::ComboBox sel, type, copyTo;
    juce::TextButton copyBtn { "Copia" }, useM1 { "Usa su M1" }, useM2 { "Usa su M2" }, resetBtn { "Ripristina fabbrica" };
    BarEditor bars;
    juce::OwnedArray<juce::Label> labels;
};

// ---------- editor dei ritmi
class BeatEditor : public juce::Component {
public:
    std::function<void()> onEdit;
    Rhythm* r = nullptr; int len = 32; int highlight = -1; juce::Colour colour = col::m1;
    void paint(juce::Graphics& g) override {
        auto a = getLocalBounds().toFloat();
        g.setColour(col::bg); g.fillRoundedRectangle(a, 4.0f);
        if (!r) return;
        a = a.reduced(6.0f);
        const float bw = a.getWidth() / 32.0f;
        for (int i = 0; i < 32; ++i) {
            int v = r->euclid ? (Engine::euclidHit(i, len, r->hits, r->rot) ? 100 : kRest) : r->v[i];
            auto cell = juce::Rectangle<float>(a.getX() + i * bw + 1, a.getY(), bw - 2, a.getHeight());
            const bool in = i < len;
            g.setColour(in ? col::panel2 : col::panel2.withAlpha(0.4f)); g.fillRect(cell);
            if (v == kRest) { g.setColour(col::mod.withAlpha(in ? 0.8f : 0.3f)); g.drawText("R", cell.removeFromBottom(16), juce::Justification::centred); }
            else if (v == kTie) { g.setColour(juce::Colour(0xff3fbf6f).withAlpha(in ? 1.0f : 0.3f)); g.fillRect(cell.withHeight(6).withY(cell.getCentreY() - 3)); g.drawText("T", cell.removeFromBottom(16), juce::Justification::centred); }
            else {
                const float h = cell.getHeight() * v / 127.0f;
                g.setColour((i == highlight ? juce::Colours::white : colour).withAlpha(in ? 1.0f : 0.3f));
                g.fillRect(cell.withTop(cell.getBottom() - h));
            }
            if (i % 4 == 0) { g.setColour(col::dim); g.setFont(10.0f); g.drawText(juce::String(i + 1), cell.withHeight(12).translated(0, -1), juce::Justification::centred); }
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if (!r || r->euclid) return;
        int i = idx(e); if (i < 0) return;
        if (e.mods.isPopupMenu() || e.mods.isCtrlDown()) {
            r->v[i] = r->v[i] == kRest ? kTie : r->v[i] == kTie ? 100 : kRest;
            repaint(); if (onEdit) onEdit(); dragging = false; return;
        }
        dragging = true; set(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override { if (dragging && r && !r->euclid) set(e); }
private:
    bool dragging = false;
    int idx(const juce::MouseEvent& e) const { auto a = getLocalBounds().toFloat().reduced(6.0f); int i = (int) ((e.position.x - a.getX()) / (a.getWidth() / 32.0f)); return i >= 0 && i < 32 ? i : -1; }
    void set(const juce::MouseEvent& e) {
        int i = idx(e); if (i < 0) return;
        auto a = getLocalBounds().toFloat().reduced(6.0f);
        int v = juce::jlimit(1, 127, juce::roundToInt((a.getBottom() - e.position.y) / a.getHeight() * 127));
        r->v[i] = v; repaint(); if (onEdit) onEdit();
    }
};

class RhythmPage : public Page {
public:
    explicit RhythmPage(AgoEditor& e) : Page(e) {
        juce::StringArray items; for (int i = 0; i < 40; ++i) items.add("Rhythm " + juce::String(i + 1) + (i < 20 ? "  (fabbrica)" : ""));
        fillCombo(sel, items); sel.setSelectedId(1, juce::dontSendNotification);
        euclid.setButtonText("Euclideo");
        for (auto* s : { &hits, &rot }) { s->setSliderStyle(juce::Slider::LinearHorizontal); s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18); }
        hits.setRange(0, 32, 1); rot.setRange(0, 31, 1);
        fillCombo(copyTo, rangeItems(21, 40, "-> ")); copyTo.setSelectedId(1, juce::dontSendNotification);
        for (juce::Component* c : { (juce::Component*) &sel, (juce::Component*) &euclid, (juce::Component*) &hits, (juce::Component*) &rot,
                                    (juce::Component*) &copyTo, (juce::Component*) &copyBtn, (juce::Component*) &useM1, (juce::Component*) &useM2,
                                    (juce::Component*) &resetBtn, (juce::Component*) &beats })
            addAndMakeVisible(c);
        mkLabel(*this, labels, "colpi", 11.0f); mkLabel(*this, labels, "rotazione", 11.0f);
        mkLabel(*this, labels, "Trascina = velocity.  Tasto destro (o Ctrl+clic) = REST -> TIE -> nota.  REST salta la nota del pattern, TIE allunga la precedente.", 11.0f);
        sel.onChange = [this] { refresh(); };
        euclid.onClick = [this] { cur().euclid = euclid.getToggleState(); ed.pushConfig(); refresh(); };
        hits.onValueChange = [this] { cur().hits = (int) hits.getValue(); ed.pushConfig(); beats.repaint(); };
        rot.onValueChange = [this] { cur().rot = (int) rot.getValue(); ed.pushConfig(); beats.repaint(); };
        beats.onEdit = [this] { ed.pushConfig(); };
        copyBtn.onClick = [this] { int dst = copyTo.getSelectedId() + 19; ed.cfg.rhythms[dst] = cur(); ed.pushConfig(); sel.setSelectedId(dst + 1); };
        useM1.onClick = [this] { ed.proc.setParam("m1Rhy", sel.getSelectedId() - 1); };
        useM2.onClick = [this] { ed.proc.setParam("m2Rhy", sel.getSelectedId() - 1); };
        resetBtn.onClick = [this] { Config f; loadFactory(f); int i = sel.getSelectedId() - 1; ed.cfg.rhythms[i] = f.rhythms[i]; ed.pushConfig(); refresh(); };
        refresh();
    }
    Rhythm& cur() { return ed.cfg.rhythms[juce::jlimit(0, 39, sel.getSelectedId() - 1)]; }
    void refresh() override {
        auto& r = cur();
        euclid.setToggleState(r.euclid != 0, juce::dontSendNotification);
        hits.setValue(r.hits, juce::dontSendNotification); rot.setValue(r.rot, juce::dontSendNotification);
        hits.setEnabled(r.euclid); rot.setEnabled(r.euclid);
        beats.r = &r; beats.repaint();
    }
    void tick() override {
        const int i = sel.getSelectedId() - 1;
        int hl = -1, len = 32;
        for (int k = 0; k < 2; ++k)
            if (ed.snap.eff.m[k].rhy == i) { len = ed.snap.eff.m[k].rlen; if (ed.snap.partRunning[2 + k]) hl = ed.snap.motifBeat[k]; beats.colour = partColour(2 + k); break; }
        if (hl != beats.highlight || len != beats.len) { beats.highlight = hl; beats.len = len; beats.repaint(); }
    }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        labels[2]->setBounds(r.removeFromBottom(22));
        auto top = r.removeFromTop(30);
        sel.setBounds(top.removeFromLeft(200)); top.removeFromLeft(10);
        euclid.setBounds(top.removeFromLeft(100));
        labels[0]->setBounds(top.removeFromLeft(40)); hits.setBounds(top.removeFromLeft(150)); top.removeFromLeft(6);
        labels[1]->setBounds(top.removeFromLeft(62)); rot.setBounds(top.removeFromLeft(150)); top.removeFromLeft(12);
        useM1.setBounds(top.removeFromLeft(84)); top.removeFromLeft(4);
        useM2.setBounds(top.removeFromLeft(84)); top.removeFromLeft(10);
        copyTo.setBounds(top.removeFromLeft(74)); top.removeFromLeft(4);
        copyBtn.setBounds(top.removeFromLeft(60)); top.removeFromLeft(8);
        resetBtn.setBounds(top.removeFromLeft(juce::jmin(130, top.getWidth())));
        r.removeFromTop(8);
        beats.setBounds(r);
    }
private:
    juce::ComboBox sel, copyTo;
    juce::ToggleButton euclid;
    juce::Slider hits, rot;
    juce::TextButton copyBtn { "Copia" }, useM1 { "Usa su M1" }, useM2 { "Usa su M2" }, resetBtn { "Ripristina fabbrica" };
    BeatEditor beats;
    juce::OwnedArray<juce::Label> labels;
};

// ---------- Perlin
class PerlinPage : public Page {
public:
    explicit PerlinPage(AgoEditor& e) : Page(e) {
        const char* ids[6] = {"px", "py", "zoom", "spc", "ruff", "per"};
        const char* nm[6] = {"X", "Y", "Zoom", "Spacing", "Ruff", "Persist"};
        for (int i = 0; i < 6; ++i) addAndMakeVisible(knobs.add(new Knob(e.proc, ids[i], nm[i], col::blue)));
        for (int k = 0; k < 2; ++k) {
            fillCombo(pool[k], { "CHORD", "SCALE", "CHROMATIC" });
            addAndMakeVisible(pool[k]);
            pool[k].onChange = [this, k] { ed.cfg.nzPool[k] = pool[k].getSelectedId() - 1; ed.pushConfig(); };
            mkLabel(*this, labels, k == 0 ? "M1 note pool" : "M2 note pool", 11.0f, partColour(2 + k));
            useBtn[k].setButtonText(k == 0 ? "M1 -> NZ" : "M2 -> NZ");
            addAndMakeVisible(useBtn[k]);
            useBtn[k].onClick = [this, k] { ed.proc.setParam(k == 0 ? "m1Patt" : "m2Patt", kPatternNZ); };
        }
        mkLabel(*this, labels, "Il pattern NZ legge il rumore di Perlin lungo un percorso: muovi X/Y per variazioni morbide, Zoom per salti piu' grandi, Spacing separa i due motif.", 11.0f);
        refresh();
    }
    void refresh() override { for (int k = 0; k < 2; ++k) pool[k].setSelectedId(ed.cfg.nzPool[k] + 1, juce::dontSendNotification); }
    void tick() override { repaint(view); }
    void paint(juce::Graphics& g) override {
        g.setColour(col::bg); g.fillRoundedRectangle(view.toFloat(), 4.0f);
        auto r = view.reduced(8);
        for (int k = 0; k < 2; ++k) {
            auto row = r.removeFromTop(r.getHeight() / (2 - k)).reduced(0, 4);
            const int mx = patTypeMax(ed.cfg.nzPool[k]);
            const float bw = row.getWidth() / 16.0f;
            const auto& p = ed.snap.eff.m[k];
            for (int i = 0; i < 16; ++i) {
                const float h = row.getHeight() * ed.snap.nz[k][i] / (float) juce::jmin(mx, 30);
                const bool active = p.patt >= kPatternNZ && ed.snap.partRunning[2 + k] && i == ed.snap.motifStep[k];
                g.setColour(active ? juce::Colours::white : partColour(2 + k).withAlpha(i < p.plen ? 1.0f : 0.3f));
                g.fillRect(juce::Rectangle<float>(row.getX() + i * bw + 2, row.getBottom() - juce::jmin((float) row.getHeight(), h), bw - 4, juce::jmin((float) row.getHeight(), h)));
            }
        }
    }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        labels[2]->setBounds(r.removeFromBottom(22));
        auto left = r.removeFromLeft(470);
        auto kr = left.removeFromTop(100);
        for (int i = 0; i < 6; ++i) knobs[i]->setBounds(kr.removeFromLeft(76).reduced(2));
        left.removeFromTop(10);
        for (int k = 0; k < 2; ++k) {
            auto row = left.removeFromTop(32);
            labels[k]->setBounds(row.removeFromLeft(100));
            pool[k].setBounds(row.removeFromLeft(140).reduced(0, 3)); row.removeFromLeft(10);
            useBtn[k].setBounds(row.removeFromLeft(100).reduced(0, 3));
        }
        r.removeFromLeft(10);
        view = r;
    }
private:
    juce::OwnedArray<Knob> knobs;
    juce::ComboBox pool[2];
    juce::TextButton useBtn[2];
    juce::Rectangle<int> view;
    juce::OwnedArray<juce::Label> labels;
};

// ---------- sequencer di accordi
// ---------- SONG a righe (progressione di accordi)
class SongRowsView : public juce::Component {
public:
    std::function<void()> onStructureChange;   // righe aggiunte/tolte/spostate
    explicit SongRowsView(AgoEditor& e) : ed(e) {}
    ChordSeq& seq() { return ed.cfg.seqs[juce::jlimit(0, 4, ed.cfg.activeSeq)]; }
    static constexpr int rowH = 30;

    void rebuild() {
        rows.clear();
        const int n = juce::jlimit(0, kMaxSongRows, seq().numRows);
        for (int i = 0; i < n; ++i) addAndMakeVisible(rows.add(new Row(*this, i)));
        setSize(juce::jmax(100, getWidth()), juce::jmax(rowH, n * rowH + 4));
        resized();
        updateValues();
    }
    void updateValues() { for (auto* r : rows) r->load(); }
    void tickUi() {
        const int playing = ed.snap.seqPlaying && seq().mode == 1 ? ed.snap.seqStep : -1;
        for (auto* r : rows) r->setPlaying(r->idx == playing ? ed.snap.seqRep : -1);
        for (auto* r : rows) r->updateChordName();
    }
    void resized() override { for (auto* r : rows) r->setBounds(0, r->idx * rowH, getWidth(), rowH); }

    void addRow() {
        auto& q = seq();
        if (q.numRows >= kMaxSongRows) return;
        SongRow nr = q.numRows > 0 ? q.rows[q.numRows - 1] : SongRow();
        q.rows[q.numRows++] = nr;
        ed.pushConfig();
        rebuild();
        if (onStructureChange) onStructureChange();
    }
    void removeRow(int i) {
        auto& q = seq();
        for (int k = i; k < q.numRows - 1; ++k) q.rows[k] = q.rows[k + 1];
        q.numRows = juce::jmax(0, q.numRows - 1);
        ed.pushConfig();
        juce::MessageManager::callAsync([sp = juce::Component::SafePointer<SongRowsView>(this)] { if (sp) { sp->rebuild(); if (sp->onStructureChange) sp->onStructureChange(); } });
    }
    void moveRow(int i, int d) {
        auto& q = seq();
        int j = i + d;
        if (j < 0 || j >= q.numRows) return;
        std::swap(q.rows[i], q.rows[j]);
        ed.pushConfig();
        updateValues();
    }

private:
    struct Row : public juce::Component {
        Row(SongRowsView& o, int i) : owner(o), idx(i) {
            juce::StringArray degs, types, keys { "Tonalita' globale" }, modes { "Modo globale" }, durs, reps;
            static const char* r[7] = {"I","II","III","IV","V","VI","VII"};
            for (int k = 0; k < 7; ++k) { degs.add(r[k]); types.add(chordTypeName(k)); }
            for (int k = 0; k < 12; ++k) keys.add(keyNameCircle(k));
            for (int k = 0; k < kNumModes; ++k) modes.add(modeName(k));
            for (int k = 0; k < kNumRowDurs; ++k) durs.add(rowDurName(k));
            for (int k = 1; k <= 16; ++k) reps.add("x" + juce::String(k));
            fillCombo(deg, degs); fillCombo(type, types); fillCombo(key, keys); fillCombo(mode, modes); fillCombo(dur, durs); fillCombo(rep, reps);
            for (auto* c : { &deg, &type, &key, &mode, &dur, &rep }) { addAndMakeVisible(c); c->onChange = [this] { store(); }; }
            for (auto* b : { &up, &down, &del }) addAndMakeVisible(b);
            del.setColour(juce::TextButton::buttonColourId, col::mod.withAlpha(0.3f));
            up.onClick = [this] { owner.moveRow(idx, -1); };
            down.onClick = [this] { owner.moveRow(idx, 1); };
            del.onClick = [this] { owner.removeRow(idx); };
        }
        void load() {
            const auto& r = owner.seq().rows[idx];
            deg.setSelectedId(r.degree + 1, juce::dontSendNotification);
            type.setSelectedId(r.type + 1, juce::dontSendNotification);
            key.setSelectedId(r.key + 2, juce::dontSendNotification);
            mode.setSelectedId(r.mode + 2, juce::dontSendNotification);
            dur.setSelectedId(rowDurIndex(r.dur8) + 1, juce::dontSendNotification);
            rep.setSelectedId(juce::jlimit(1, 16, r.reps), juce::dontSendNotification);
            updateChordName();
        }
        void store() {
            auto& r = owner.seq().rows[idx];
            r.degree = deg.getSelectedId() - 1; r.type = type.getSelectedId() - 1;
            r.key = key.getSelectedId() - 2; r.mode = mode.getSelectedId() - 2;
            r.dur8 = rowDurValue(dur.getSelectedId() - 1); r.reps = rep.getSelectedId();
            owner.ed.pushConfig();
            updateChordName();
        }
        void updateChordName() {
            const auto& r = owner.seq().rows[idx];
            const int gk = AgoProcessor::paramValue(owner.ed.proc.apvts, "key");
            const int gm = AgoProcessor::paramValue(owner.ed.proc.apvts, "mode");
            Chord c = buildChord(r.key < 0 ? gk : r.key, r.mode < 0 ? gm : r.mode, r.degree, r.type);
            juce::String t = juce::String(romanFor(c)) + "  " + pcName(c.rootPc) + " " + chordTypeName(c.type);
            if (t != chordName) { chordName = t; repaint(); }
        }
        void setPlaying(int repIdx) { if (repIdx != playRep) { playRep = repIdx; repaint(); } }
        void paint(juce::Graphics& g) override {
            auto b = getLocalBounds().reduced(0, 1).toFloat();
            g.setColour(playRep >= 0 ? col::m1.withAlpha(0.28f) : (idx % 2 ? col::panel2.withAlpha(0.5f) : col::panel));
            g.fillRoundedRectangle(b, 4.0f);
            g.setColour(col::dim); g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawText(juce::String(idx + 1), getLocalBounds().withWidth(30), juce::Justification::centred);
            g.setColour(col::m1); g.setFont(juce::Font(13.0f, juce::Font::bold));
            g.drawText(chordName, nameArea, juce::Justification::centredLeft);
            if (playRep >= 0) {
                g.setColour(col::text); g.setFont(11.0f);
                g.drawText(juce::String(playRep + 1) + "/" + juce::String(owner.seq().rows[idx].reps), nameArea, juce::Justification::centredRight);
            }
        }
        void resized() override {
            auto r = getLocalBounds().reduced(2, 3);
            r.removeFromLeft(30);
            deg.setBounds(r.removeFromLeft(62)); r.removeFromLeft(4);
            type.setBounds(r.removeFromLeft(78)); r.removeFromLeft(4);
            key.setBounds(r.removeFromLeft(130)); r.removeFromLeft(4);
            mode.setBounds(r.removeFromLeft(120)); r.removeFromLeft(4);
            dur.setBounds(r.removeFromLeft(100)); r.removeFromLeft(4);
            rep.setBounds(r.removeFromLeft(58)); r.removeFromLeft(10);
            del.setBounds(r.removeFromRight(28)); r.removeFromRight(4);
            down.setBounds(r.removeFromRight(28)); r.removeFromRight(2);
            up.setBounds(r.removeFromRight(28)); r.removeFromRight(10);
            nameArea = r;
        }
        SongRowsView& owner; int idx;
        juce::ComboBox deg, type, key, mode, dur, rep;
        juce::TextButton up { "^" }, down { "v" }, del { "X" };
        juce::String chordName; int playRep = -1; juce::Rectangle<int> nameArea;
    };
    AgoEditor& ed;
    juce::OwnedArray<Row> rows;
};

class ChordSeqPage : public Page {
public:
    explicit ChordSeqPage(AgoEditor& e) : Page(e), arm(e.proc, "seqArm", "ARM", col::m1), rowsView(e) {
        juce::StringArray keys, modes, degs, types, durs;
        for (int i = 0; i < 12; ++i) keys.add(keyNameCircle(i));
        for (int i = 0; i < kNumModes; ++i) modes.add(modeName(i));
        for (int i = 0; i < 7; ++i) { static const char* r[7] = {"I","II","III","IV","V","VI","VII"}; degs.add(r[i]); types.add(chordTypeName(i)); }
        for (int i = 0; i <= 63; ++i) durs.add(i == 0 ? juce::String("--") : juce::String(i / 2.0, i % 2 ? 1 : 0) + " q");
        fillCombo(slot, rangeItems(1, 5, "Sequenza "));
        addAndMakeVisible(slot); addAndMakeVisible(arm);
        slot.onChange = [this] { ed.cfg.activeSeq = slot.getSelectedId() - 1; ed.pushConfig(); refresh(); };
        for (int s = 0; s < 3; ++s)
            for (int i = 0; i < 6; ++i) {
                auto& st = steps[s][i];
                fillCombo(st.dur, durs); fillCombo(st.key, keys); fillCombo(st.mode, modes); fillCombo(st.deg, degs); fillCombo(st.type, types);
                for (auto* c : { &st.dur, &st.key, &st.mode, &st.deg, &st.type }) { addAndMakeVisible(c); c->onChange = [this, s, i] { store(s, i); }; }
            }
        for (int j = 0; j < 8; ++j) {
            fillCombo(song[j].sec, { "A", "B", "C" });
            fillCombo(song[j].rep, rangeItems(0, 8, "x"));
            addAndMakeVisible(song[j].sec); addAndMakeVisible(song[j].rep);
            song[j].sec.onChange = [this, j] { seq().songSec[j] = song[j].sec.getSelectedId() - 1; ed.pushConfig(); };
            song[j].rep.onChange = [this, j] { seq().songRep[j] = song[j].rep.getSelectedId() - 1; ed.pushConfig(); };
        }
        for (auto* t : { "A", "B", "C" }) mkLabel(*this, labels, juce::String("SEZIONE ") + t, 12.0f, col::text);
        mkLabel(*this, labels, "SONG", 12.0f, col::text);
        mkLabel(*this, labels, "Durata in quarti (passo 1/8). Step vuoti (--) ignorati. ARM avvia la sequenza al prossimo downbeat, a trasporto in corsa.", 11.0f);
        mkLabel(*this, labels, "Ogni riga: accordo per la durata, ripetuto xN, poi la successiva; a fine lista ricomincia. Tonalita'/modo globale = quelli in alto (traspongono tutta la song).", 11.0f);
        mkLabel(*this, labels, "", 10.0f);   // area intestazioni (disegnate in paint)
        for (auto* b : { &modeSec, &modeSong }) { b->setClickingTogglesState(false); b->setColour(juce::TextButton::buttonOnColourId, col::blue); addAndMakeVisible(b); }
        modeSec.onClick = [this] { seq().mode = 0; ed.pushConfig(); refresh(); };
        modeSong.onClick = [this] { seq().mode = 1; ed.pushConfig(); refresh(); };
        addRowBtn.setColour(juce::TextButton::buttonColourId, col::m1.withAlpha(0.35f));
        addAndMakeVisible(addRowBtn);
        addRowBtn.onClick = [this] { rowsView.addRow(); vp.setViewPosition(0, rowsView.getHeight()); updateTotal(); };
        rowsView.onStructureChange = [this] { updateTotal(); };
        vp.setViewedComponent(&rowsView, false);
        vp.setScrollBarsShown(true, false);
        addAndMakeVisible(vp);
        addAndMakeVisible(totalLbl);
        totalLbl.setFont(juce::Font(12.0f, juce::Font::bold));
        totalLbl.setColour(juce::Label::textColourId, col::dim);
        refresh();
    }
    void updateTotal() {
        int64_t e = 0; for (int i = 0; i < seq().numRows; ++i) e += (int64_t) seq().rows[i].dur8 * seq().rows[i].reps;
        totalLbl.setText(juce::String(seq().numRows) + " righe  -  " + juce::String(e / 8.0, e % 8 ? 2 : 0) + " battute", juce::dontSendNotification);
    }
    ChordSeq& seq() { return ed.cfg.seqs[juce::jlimit(0, 4, ed.cfg.activeSeq)]; }
    void store(int s, int i) {
        auto& st = steps[s][i]; auto& c = seq().sec[s][i];
        c.dur8 = st.dur.getSelectedId() - 1; c.key = st.key.getSelectedId() - 1; c.mode = st.mode.getSelectedId() - 1;
        c.degree = st.deg.getSelectedId() - 1; c.type = st.type.getSelectedId() - 1;
        ed.pushConfig();
    }
    void refresh() override {
        slot.setSelectedId(ed.cfg.activeSeq + 1, juce::dontSendNotification);
        const bool songMode = seq().mode == 1;
        modeSec.setToggleState(!songMode, juce::dontSendNotification);
        modeSong.setToggleState(songMode, juce::dontSendNotification);
        for (int s = 0; s < 3; ++s) for (int i = 0; i < 6; ++i) for (auto* c : { &steps[s][i].dur, &steps[s][i].key, &steps[s][i].mode, &steps[s][i].deg, &steps[s][i].type }) c->setVisible(!songMode);
        for (int j = 0; j < 8; ++j) { song[j].sec.setVisible(!songMode); song[j].rep.setVisible(!songMode); }
        for (int k = 0; k < 5; ++k) labels[k]->setVisible(!songMode);
        labels[5]->setVisible(songMode); labels[6]->setVisible(songMode);
        vp.setVisible(songMode); addRowBtn.setVisible(songMode); totalLbl.setVisible(songMode);
        rowsView.rebuild();
        updateTotal();
        resized();
        repaint();
        for (int s = 0; s < 3; ++s)
            for (int i = 0; i < 6; ++i) {
                auto& st = steps[s][i]; auto& c = seq().sec[s][i];
                st.dur.setSelectedId(c.dur8 + 1, juce::dontSendNotification); st.key.setSelectedId(c.key + 1, juce::dontSendNotification);
                st.mode.setSelectedId(c.mode + 1, juce::dontSendNotification); st.deg.setSelectedId(c.degree + 1, juce::dontSendNotification);
                st.type.setSelectedId(c.type + 1, juce::dontSendNotification);
            }
        for (int j = 0; j < 8; ++j) {
            song[j].sec.setSelectedId(seq().songSec[j] + 1, juce::dontSendNotification);
            song[j].rep.setSelectedId(seq().songRep[j] + 1, juce::dontSendNotification);
        }
    }
    void tick() override {
        if (seq().mode == 1) { rowsView.tickUi(); if (shown != -1) { shown = -1; repaint(); } return; }
        if (ed.snap.seqStep != shown) { shown = ed.snap.seqStep; repaint(); }
    }
    void paint(juce::Graphics& g) override {
        if (seq().mode == 1) {
            auto h = labels[6]->getBounds();
            const int x0 = vp.getX() + 2 + 30;
            const int xs[7] = {0, 66, 148, 282, 406, 510, 578};
            const char* nm[7] = {"GRADO", "TIPO", "TONALITA'", "MODO", "DURATA", "RIP.", "ACCORDO"};
            g.setColour(col::dim); g.setFont(juce::Font(10.0f, juce::Font::bold));
            g.drawText("#", juce::Rectangle<int>(vp.getX(), h.getY(), 32, h.getHeight()), juce::Justification::centred);
            for (int i = 0; i < 7; ++i) g.drawText(nm[i], juce::Rectangle<int>(x0 + xs[i] + 4, h.getY(), 120, h.getHeight()), juce::Justification::centredLeft);
            return;
        }
        if (shown >= 0) {
            auto r = steps[shown / 6][shown % 6].dur.getBounds().getUnion(steps[shown / 6][shown % 6].type.getBounds()).expanded(3, 2);
            g.setColour(col::m1.withAlpha(0.35f)); g.fillRoundedRectangle(r.toFloat(), 4.0f);
        }
    }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        labels[4]->setBounds(r.removeFromBottom(20));
        auto top = r.removeFromTop(28);
        slot.setBounds(top.removeFromLeft(140)); top.removeFromLeft(10);
        arm.setBounds(top.removeFromLeft(80)); top.removeFromLeft(20);
        modeSec.setBounds(top.removeFromLeft(150)); top.removeFromLeft(4);
        modeSong.setBounds(top.removeFromLeft(150)); top.removeFromLeft(20);
        addRowBtn.setBounds(top.removeFromLeft(150)); top.removeFromLeft(14);
        totalLbl.setBounds(top);
        if (seq().mode == 1) {
            labels[5]->setBounds(getLocalBounds().reduced(10).removeFromBottom(20));
            r.removeFromBottom(4);
            r.removeFromTop(6);
            labels[6]->setBounds(r.removeFromTop(16).withTrimmedLeft(4));
            vp.setBounds(r);
            rowsView.setSize(r.getWidth() - vp.getScrollBarThickness(), rowsView.getHeight());
            return;
        }
        auto songRow = r.removeFromBottom(30);
        labels[3]->setBounds(songRow.removeFromLeft(50));
        for (int j = 0; j < 8; ++j) {
            song[j].sec.setBounds(songRow.removeFromLeft(52).reduced(1, 2));
            song[j].rep.setBounds(songRow.removeFromLeft(56).reduced(1, 2)); songRow.removeFromLeft(12);
        }
        r.removeFromTop(4);
        const int colW = r.getWidth() / 3;
        for (int s = 0; s < 3; ++s) {
            auto c = juce::Rectangle<int>(r.getX() + s * colW, r.getY(), colW - 12, r.getHeight());
            labels[s]->setBounds(c.removeFromTop(18));
            const int rh = juce::jmin(28, c.getHeight() / 6);
            for (int i = 0; i < 6; ++i) {
                auto row = c.removeFromTop(rh).reduced(0, 2);
                auto& st = steps[s][i];
                const int w = row.getWidth();
                st.dur.setBounds(row.removeFromLeft(w * 18 / 100)); row.removeFromLeft(2);
                st.key.setBounds(row.removeFromLeft(w * 15 / 100)); row.removeFromLeft(2);
                st.mode.setBounds(row.removeFromLeft(w * 28 / 100)); row.removeFromLeft(2);
                st.deg.setBounds(row.removeFromLeft(w * 15 / 100)); row.removeFromLeft(2);
                st.type.setBounds(row);
            }
        }
    }
private:
    juce::ComboBox slot;
    ParamToggle arm;
    juce::TextButton modeSec { "SEZIONI A/B/C" }, modeSong { "SONG A RIGHE" }, addRowBtn { "+ AGGIUNGI RIGA" };
    juce::Viewport vp;
    SongRowsView rowsView;
    juce::Label totalLbl;
    struct Step { juce::ComboBox dur, key, mode, deg, type; } steps[3][6];
    struct Song { juce::ComboBox sec, rep; } song[8];
    int shown = -1;
    juce::OwnedArray<juce::Label> labels;
};

// ---------- MIDI
class MidiPage : public Page {
public:
    explicit MidiPage(AgoEditor& e) : Page(e) {
        juce::StringArray chans = rangeItems(1, 16, "Ch "), ports;
        for (int p = 0; p < kNumPortChoices; ++p) ports.add(portName(p));
        const char* nm[4] = {"PAD", "DRONE", "MOTIF 1", "MOTIF 2"};
        for (int i = 0; i < 4; ++i) {
            mkLabel(*this, partLabels, nm[i], 12.0f, partColour(i));
            fillCombo(ch[i], chans); fillCombo(port[i], ports);
            addAndMakeVisible(ch[i]); addAndMakeVisible(port[i]);
            ch[i].onChange = [this, i] { chanRef(i) = ch[i].getSelectedId(); ed.pushConfig(); };
            port[i].onChange = [this, i] { portRef(i) = port[i].getSelectedId() - 1; ed.pushConfig(); };
        }
        for (int p = 0; p < 2; ++p) {
            mkLabel(*this, labels, p == 0 ? "PORT A" : "PORT B", 12.0f, col::text);
            addAndMakeVisible(dev[p]); addAndMakeVisible(status[p]);
            status[p].setFont(11.0f); status[p].setColour(juce::Label::textColourId, col::dim);
            dev[p].onChange = [this, p] {
                const int id = dev[p].getSelectedId();
                ed.proc.setPortDevice(p + 1, id <= 1 ? juce::String() : devices[id - 2].identifier);
                status[p].setText(ed.proc.getPortStatus(p + 1), juce::dontSendNotification);
            };
        }
        addAndMakeVisible(refreshBtn);
        refreshBtn.onClick = [this] { scanDevices(); };
        fillCombo(ctrl, chans); fillCombo(trans, chans); fillCombo(kbPort, ports);
        fillCombo(kbMode, { "White Keys", "White & Black", "NDLR-style Set Key", "Set Key + Type" });
        for (juce::Component* c : { (juce::Component*) &ctrl, (juce::Component*) &trans, (juce::Component*) &kbMode, (juce::Component*) &kbPort, (juce::Component*) &thru, (juce::Component*) &delay })
            addAndMakeVisible(c);
        mkLabel(*this, labels, "Canale controllo", 11.0f); mkLabel(*this, labels, "Canale trasposizione", 11.0f);
        mkLabel(*this, labels, "Tastiera", 11.0f); mkLabel(*this, labels, "Uscita tastiera", 11.0f);
        mkLabel(*this, labels, "Ritardo porte A/B", 11.0f);
        thru.setButtonText("MIDI thru (altri canali -> Host)");
        delay.setSliderStyle(juce::Slider::LinearHorizontal); delay.setRange(0, 250, 1); delay.setTextValueSuffix(" ms");
        delay.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 18);
        ctrl.onChange = [this] { ed.cfg.ctrlChan = ctrl.getSelectedId(); ed.pushConfig(); };
        trans.onChange = [this] { ed.cfg.transChan = trans.getSelectedId(); ed.pushConfig(); };
        kbMode.onChange = [this] { ed.cfg.kbMode = kbMode.getSelectedId() - 1; ed.pushConfig(); };
        kbPort.onChange = [this] { ed.cfg.kbPort = kbPort.getSelectedId() - 1; ed.pushConfig(); };
        thru.onClick = [this] { ed.cfg.passThru = thru.getToggleState(); ed.pushConfig(); };
        delay.onValueChange = [this] { ed.proc.midiOutDelayMs = (int) delay.getValue(); };
        linkRole = std::make_unique<ParamCombo>(e.proc, "linkRole");
        linkGroup = std::make_unique<ParamCombo>(e.proc, "linkGroup");
        linkPart = std::make_unique<ParamCombo>(e.proc, "linkPart");
        for (auto* c : { linkRole.get(), linkGroup.get(), linkPart.get() }) addAndMakeVisible(c);
        mkLabel(*this, labels, "Link (multi-traccia)", 11.0f, col::m1); mkLabel(*this, labels, "Gruppo", 11.0f); mkLabel(*this, labels, "Parte (follower)", 11.0f);
        addAndMakeVisible(knobs.add(new Knob(e.proc, "offset", "M2 Offset", col::m2)));
        addAndMakeVisible(knobs.add(new Knob(e.proc, "humanize", "Humanize", col::m1)));
        addAndMakeVisible(knobs.add(new Knob(e.proc, "velLow", "Vel Low", col::m1)));
        auto* h = mkLabel(*this, labels, "Live unisce su un solo canale il MIDI che esce da un plugin. Per separare le parti dentro Live usa LINK: "
            "un AGO Leader + un AGO Follower per parte su tracce diverse (MIDI From = traccia del follower). In alternativa Port A/B apre "
            "direttamente un dispositivo MIDI (USB del synth o loopMIDI) mantenendo i canali: la porta va disattivata come uscita nelle preferenze di Live.", 11.0f);
        h->setJustificationType(juce::Justification::topLeft);
        help = h;
        scanDevices();
        refresh();
    }
    int& chanRef(int i) { return i == 0 ? ed.cfg.padChan : i == 1 ? ed.cfg.droneChan : i == 2 ? ed.cfg.m1Chan : ed.cfg.m2Chan; }
    int& portRef(int i) { return i == 0 ? ed.cfg.padPort : i == 1 ? ed.cfg.dronePort : i == 2 ? ed.cfg.m1Port : ed.cfg.m2Port; }
    void scanDevices() {
        devices = juce::MidiOutput::getAvailableDevices();
        for (int p = 0; p < 2; ++p) {
            dev[p].clear(juce::dontSendNotification);
            dev[p].addItem("(nessuna)", 1);
            int sel = 1;
            for (int i = 0; i < devices.size(); ++i) {
                dev[p].addItem(devices[i].name, i + 2);
                if (devices[i].identifier == ed.proc.getPortDevice(p + 1)) sel = i + 2;
            }
            dev[p].setSelectedId(sel, juce::dontSendNotification);
            status[p].setText(ed.proc.getPortStatus(p + 1), juce::dontSendNotification);
        }
    }
    void refresh() override {
        for (int i = 0; i < 4; ++i) { ch[i].setSelectedId(chanRef(i), juce::dontSendNotification); port[i].setSelectedId(portRef(i) + 1, juce::dontSendNotification); }
        ctrl.setSelectedId(ed.cfg.ctrlChan, juce::dontSendNotification);
        trans.setSelectedId(ed.cfg.transChan, juce::dontSendNotification);
        kbMode.setSelectedId(ed.cfg.kbMode + 1, juce::dontSendNotification);
        kbPort.setSelectedId(ed.cfg.kbPort + 1, juce::dontSendNotification);
        thru.setToggleState(ed.cfg.passThru != 0, juce::dontSendNotification);
        delay.setValue(ed.proc.midiOutDelayMs, juce::dontSendNotification);
        scanDevices();
    }
    void resized() override {
        auto r = getLocalBounds().reduced(10);
        auto left = r.removeFromLeft(330);
        for (int i = 0; i < 4; ++i) {
            auto row = left.removeFromTop(32).reduced(0, 3);
            partLabels[i]->setBounds(row.removeFromLeft(80));
            ch[i].setBounds(row.removeFromLeft(90)); row.removeFromLeft(6);
            port[i].setBounds(row.removeFromLeft(110));
        }
        left.removeFromTop(8);
        auto kr = left.removeFromTop(90);
        for (auto* k : knobs) k->setBounds(kr.removeFromLeft(90).reduced(4));
        r.removeFromLeft(16);
        auto mid = r.removeFromLeft(420);
        for (int p = 0; p < 2; ++p) {
            auto row = mid.removeFromTop(30).reduced(0, 2);
            labels[p]->setBounds(row.removeFromLeft(60));
            dev[p].setBounds(row.removeFromLeft(280));
            status[p].setBounds(mid.removeFromTop(18).withTrimmedLeft(60));
        }
        refreshBtn.setBounds(mid.removeFromTop(28).withTrimmedLeft(60).withWidth(150).reduced(0, 2));
        mid.removeFromTop(6);
        help->setBounds(mid);
        r.removeFromLeft(16);
        auto right = r;
        auto rowL = [&](int li, juce::Component& c) { auto row = right.removeFromTop(30).reduced(0, 3); labels[li]->setBounds(row.removeFromLeft(130)); c.setBounds(row); };
        rowL(2, ctrl); rowL(3, trans); rowL(4, kbMode); rowL(5, kbPort);
        thru.setBounds(right.removeFromTop(28));
        rowL(6, delay);
        right.removeFromTop(6);
        rowL(7, *linkRole); rowL(8, *linkGroup); rowL(9, *linkPart);
    }
private:
    juce::ComboBox ch[4], port[4], dev[2], ctrl, trans, kbMode, kbPort;
    juce::Label status[2];
    juce::ToggleButton thru;
    juce::Slider delay;
    juce::TextButton refreshBtn { "Aggiorna dispositivi" };
    juce::Array<juce::MidiDeviceInfo> devices;
    juce::OwnedArray<juce::Label> labels, partLabels;
    juce::OwnedArray<Knob> knobs;
    juce::Label* help = nullptr;
    std::unique_ptr<ParamCombo> linkRole, linkGroup, linkPart;
};

// ---------- Rotator / Interval
class KeysPage : public Page {
public:
    explicit KeysPage(AgoEditor& e) : Page(e) {
        rot.setButtonText("ROTATOR"); iv.setButtonText("INTERVAL MODE");
        addAndMakeVisible(rot); addAndMakeVisible(iv);
        rot.onClick = [this] { ed.cfg.rotatorOn = rot.getToggleState(); if (ed.cfg.rotatorOn) ed.cfg.intervalOn = 0; ed.pushConfig(); refresh(); };
        iv.onClick = [this] { ed.cfg.intervalOn = iv.getToggleState(); if (ed.cfg.intervalOn) ed.cfg.rotatorOn = 0; ed.pushConfig(); refresh(); };
        const char* nm[5] = {"Parallela", "Rotante 1", "Rotante 2", "Rotante 3", "Rotante 4"};
        for (int i = 0; i < 5; ++i) {
            auto* s = sl.add(new juce::Slider());
            s->setSliderStyle(juce::Slider::LinearVertical);
            s->setRange(-19, 40, 1);
            s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
            s->setTextValueSuffix(" st");
            addAndMakeVisible(s);
            s->onValueChange = [this, i] { ed.cfg.patterns[39].v[i] = juce::jlimit(1, 60, (int) sl[i]->getValue() + 20); ed.pushConfig(); };
            mkLabel(*this, labels, nm[i], 11.0f);
        }
        auto* h = mkLabel(*this, labels,
            "Entrambe le modalita' leggono le note sul canale di trasposizione (tab MIDI) ed escono sullo stesso canale.\n\n"
            "ROTATOR: ogni nota suona tre note - la tua, la parallela e una delle 4 rotanti a turno (intervalli in semitoni, quantizzati alla scala). "
            "Gli intervalli sono i primi 5 passi del pattern 40.\n\n"
            "INTERVAL: D3 = nota base, i tasti bianchi sopra/sotto saltano di 1-5 gradi di scala dalla nota precedente. Db3 ripete, Eb3 torna alla base in silenzio, "
            "F#3 salto casuale, G#3/Bb3 ottava giu'/su, Bb2 segue l'accordo, Db2/Eb2 e F#2/G#2 spostano i limiti di avvolgimento.\n\n"
            "Spenti entrambi, il canale traspone dai tasti bianchi di C maggiore alla tonalita' e al modo correnti.", 12.0f, col::text);
        h->setJustificationType(juce::Justification::topLeft);
        help = h;
        refresh();
    }
    void refresh() override {
        rot.setToggleState(ed.cfg.rotatorOn != 0, juce::dontSendNotification);
        iv.setToggleState(ed.cfg.intervalOn != 0, juce::dontSendNotification);
        for (int i = 0; i < 5; ++i) sl[i]->setValue(ed.cfg.patterns[39].v[i] - 20, juce::dontSendNotification);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(12);
        auto left = r.removeFromLeft(380);
        auto top = left.removeFromTop(30);
        rot.setBounds(top.removeFromLeft(130)); iv.setBounds(top.removeFromLeft(170));
        left.removeFromTop(10);
        auto lr = left.removeFromTop(16);
        for (int i = 0; i < 5; ++i) labels[i]->setBounds(lr.removeFromLeft(74));
        for (int i = 0; i < 5; ++i) sl[i]->setBounds(left.removeFromLeft(74));
        r.removeFromLeft(20);
        help->setBounds(r);
    }
private:
    juce::ToggleButton rot, iv;
    juce::OwnedArray<juce::Slider> sl;
    juce::OwnedArray<juce::Label> labels;
    juce::Label* help = nullptr;
};

} // namespace

// =====================================================================================  editor
AgoEditor::AgoEditor(AgoProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
      syncBox(p, "sync"), keyBox(p, "key"), modeBox(p, "mode"),
      playBtn(p, "play", "PLAY", col::m1), modAllBtn(p, "modAll", "MOD", col::mod), invertBtn(p, "invert", "INVERT", col::blue),
      partOn { { p, "padOn", "PAD", col::pad }, { p, "droneOn", "DRONE", col::drone }, { p, "m1On", "MOTIF 1", col::m1 }, { p, "m2On", "MOTIF 2", col::m2 } },
      partMod { { p, "modPad", "MOD", col::mod }, { p, "modDrone", "MOD", col::mod }, { p, "modM1", "MOD", col::mod }, { p, "modM2", "MOD", col::mod } } {
    setLookAndFeel(&lnf);
    cfg = proc.getConfig();
    seenSerial = proc.getConfigSerial();

    for (auto* c : { (juce::Component*) &syncBox, (juce::Component*) &keyBox, (juce::Component*) &modeBox, (juce::Component*) &tempo,
                     (juce::Component*) &playBtn, (juce::Component*) &modAllBtn, (juce::Component*) &invertBtn,
                     (juce::Component*) &playAllBtn, (juce::Component*) &resyncBtn, (juce::Component*) &panicBtn })
        addAndMakeVisible(c);
    tempo.setSliderStyle(juce::Slider::LinearBar);
    tempo.setColour(juce::Slider::trackColourId, col::blue.withAlpha(0.5f));
    tempoAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "tempo", tempo);
    tempo.setTextValueSuffix(" BPM");
    playAllBtn.onClick = [this] { proc.playAll(); };
    resyncBtn.onClick = [this] { proc.requestResync(); };
    panicBtn.onClick = [this] { proc.requestPanic(); };
    panicBtn.setColour(juce::TextButton::buttonColourId, col::mod.withAlpha(0.35f));

    for (int i = 0; i < 7; ++i) {
        addAndMakeVisible(chordBtn[i]);
        chordBtn[i].onClick = [this, i] { proc.setParam("degree", i); };
        chordBtn[i].setColour(juce::TextButton::buttonOnColourId, col::m1);
        typeBtn[i].setButtonText(chordTypeName(i));
        typeBtn[i].setColour(juce::TextButton::buttonOnColourId, col::blue);
        addAndMakeVisible(typeBtn[i]);
        typeBtn[i].onClick = [this, i] { proc.setParam("ctype", i); };
    }

    for (int i = 0; i < 4; ++i) { addAndMakeVisible(partOn[i]); addAndMakeVisible(partMod[i]); }
    auto K = [&](int part, const char* id, const char* nm) { addAndMakeVisible(knobs[part].add(new Knob(proc, id, nm, partColour(part)))); };
    K(0, "padPos", "Position"); K(0, "padRange", "Range"); K(0, "padSpread", "Spread"); K(0, "padStrum", "Strum");
    K(0, "padVel", "Velocity"); K(0, "padQuant", "Quantize"); K(0, "polyChain", "Poly Chain");
    K(1, "droneOct", "Position"); K(1, "droneType", "Notes"); K(1, "droneTrig", "Trigger"); K(1, "droneVel", "Velocity");
    for (int k = 0; k < 2; ++k) {
        const int part = 2 + k;
        const juce::String pre = k == 0 ? "m1" : "m2";
        K(part, (pre + "Pos").toRawUTF8(), "Position"); K(part, (pre + "PLen").toRawUTF8(), "Patt Len");
        K(part, (pre + "Var").toRawUTF8(), "Variation"); K(part, (pre + "Patt").toRawUTF8(), "Pattern");
        K(part, (pre + "Div").toRawUTF8(), "Clock Div"); K(part, (pre + "RLen").toRawUTF8(), "Rhy Len");
        K(part, (pre + "Acc").toRawUTF8(), "Accent"); K(part, (pre + "Rhy").toRawUTF8(), "Rhythm");
        K(part, (pre + "Gate").toRawUTF8(), "Gate"); K(part, (pre + "Vel").toRawUTF8(), "Velocity");
        motifView[k] = std::make_unique<MotifView>(*this, k);
        addAndMakeVisible(*motifView[k]);
    }
    keyboard = std::make_unique<KeyboardView>(*this);
    addAndMakeVisible(*keyboard);

    auto addPage = [&](const juce::String& name, agoui::Page* pg) { pages.add(pg); tabs.addTab(name, col::panel, pg, true); };
    addPage("MOD MATRIX", new ModPage(*this));
    addPage("LFO", new LfoPage(*this));
    addPage("PATTERN", new PatternPage(*this));
    addPage("RHYTHM", new RhythmPage(*this));
    addPage("PERLIN", new PerlinPage(*this));
    addPage("CHORD SEQ / SONG", new ChordSeqPage(*this));
    addPage("MIDI / PORTE", new MidiPage(*this));
    addPage("ROTATOR / INTERVAL", new KeysPage(*this));
    tabs.setTabBarDepth(30);
    tabs.setOutline(0);
    addAndMakeVisible(tabs);

    followerOverlay = std::make_unique<FollowerOverlay>(*this);
    addChildComponent(*followerOverlay);

    setSize(1200, 840);
    startTimerHz(25);
}

AgoEditor::~AgoEditor() { stopTimer(); setLookAndFeel(nullptr); }

void AgoEditor::timerCallback() {
    proc.getSnapshot(snap);
    if (proc.getConfigSerial() != seenSerial) {
        seenSerial = proc.getConfigSerial();
        cfg = proc.getConfig();
        for (auto* pg : pages) pg->refresh();
    }
    // tasti accordo
    const int key = AgoProcessor::paramValue(proc.apvts, "key");
    const int mode = AgoProcessor::paramValue(proc.apvts, "mode");
    const int ctype = AgoProcessor::paramValue(proc.apvts, "ctype");
    for (int i = 0; i < 7; ++i) {
        Chord c = buildChord(key, mode, i, ctype);
        chordBtn[i].setButtonText(juce::String(romanFor(c)) + "  " + pcName(c.rootPc));
        chordBtn[i].setToggleState(snap.eff.degree == i, juce::dontSendNotification);
        typeBtn[i].setToggleState(ctype == i, juce::dontSendNotification);
    }
    const int ls = proc.getLinkStatus();
    const bool follower = AgoProcessor::paramValue(proc.apvts, "linkRole") == 2 || ls == 2;
    if (follower != followerOverlay->isVisible()) { followerOverlay->setVisible(follower); if (follower) followerOverlay->toFront(false); }
    if (follower) followerOverlay->repaint();
    for (auto* c : std::initializer_list<juce::Component*> { &keyBox, &modeBox, &invertBtn, &playBtn, &syncBox, &tempo, &playAllBtn, &resyncBtn })
        c->setEnabled(!follower);
    for (int i = 0; i < 7; ++i) { chordBtn[i].setEnabled(!follower); typeBtn[i].setEnabled(!follower); }
    keyboard->repaint();
    for (auto& v : motifView) v->repaint();
    if (auto* pg = dynamic_cast<agoui::Page*>(tabs.getCurrentContentComponent())) pg->tick();
    repaint(headerArea);
    for (auto a : partArea) repaint(a.removeFromTop(30));
}

void AgoEditor::paint(juce::Graphics& g) {
    g.fillAll(col::bg);
    // header
    auto h = headerArea;
    g.setColour(col::text); g.setFont(juce::Font(30.0f, juce::Font::bold));
    g.drawText("AGO", h.removeFromLeft(90).withTrimmedLeft(14), juce::Justification::centredLeft);
    g.setColour(col::dim); g.setFont(11.0f);
    {
        auto sub = h.removeFromLeft(130);
        const int ls = proc.getLinkStatus();
        const int grp = AgoProcessor::paramValue(proc.apvts, "linkGroup") + 1;
        static const char* partN[5] = {"PAD", "DRONE", "MOTIF 1", "MOTIF 2", "TUTTE"};
        juce::String badge = ls == 1 ? "LINK LEADER  G" + juce::String(grp)
                           : (ls == 2 || ls == 3 || ls == 4) ? "FOLLOWER G" + juce::String(grp) + "  " + partN[juce::jlimit(0, 4, AgoProcessor::paramValue(proc.apvts, "linkPart"))]
                           : ls == 5 ? "LINK: serve l'host" : "harmonic conductor";
        g.setColour(ls == 0 ? col::dim : (ls == 1 || ls == 3 || ls == 2) ? col::m1 : col::mod);
        g.setFont(juce::Font(11.0f, ls == 0 ? juce::Font::plain : juce::Font::bold));
        g.drawText(badge, sub, juce::Justification::centredLeft);
    }
    // display accordo
    auto disp = juce::Rectangle<int>(536, 8, 336, 46);
    g.setColour(col::panel); g.fillRoundedRectangle(disp.toFloat(), 6.0f);
    const auto& c = snap.chord;
    g.setColour(col::m1); g.setFont(juce::Font(26.0f, juce::Font::bold));
    g.drawText(romanFor(c), disp.withWidth(90), juce::Justification::centred);
    g.setColour(col::text); g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText(juce::String(pcName(c.rootPc)) + " " + chordTypeName(c.type), disp.withTrimmedLeft(86).withWidth(130), juce::Justification::centredLeft);
    g.setColour(col::dim); g.setFont(12.0f);
    g.drawText(juce::String(pcName(c.keyPc)) + " " + modeName(c.mode), disp.withTrimmedLeft(196).withTrimmedRight(10).withTrimmedBottom(23), juce::Justification::centredRight);
    juce::String clk = juce::String(proc.getCurrentTempo(), 1) + " BPM";
    if (snap.seqPlaying) clk = "SEQ  " + clk;
    g.setColour(proc.isTransportRunning() ? col::m1 : col::dim);
    g.drawText(clk, disp.withTrimmedLeft(196).withTrimmedRight(10).withTrimmedTop(23), juce::Justification::centredRight);

    // pannelli delle parti
    const char* names[4] = {"", "", "", ""};
    for (int i = 0; i < 4; ++i) {
        auto a = partArea[i];
        g.setColour(col::panel); g.fillRoundedRectangle(a.toFloat(), 8.0f);
        g.setColour(partColour(i).withAlpha(snap.partRunning[i] ? 0.9f : 0.25f));
        g.fillRoundedRectangle(a.toFloat().removeFromTop(4.0f), 2.0f);
        juce::ignoreUnused(names);
        if (snap.partRunning[i]) { g.setColour(partColour(i)); g.fillEllipse((float) a.getRight() - 16, (float) a.getY() + 12, 8, 8); }
    }
}

void AgoEditor::resized() {
    auto r = getLocalBounds();
    headerArea = r.removeFromTop(62);
    {
        auto h = headerArea.reduced(10, 14);
        panicBtn.setBounds(h.removeFromRight(70)); h.removeFromRight(6);
        resyncBtn.setBounds(h.removeFromRight(76)); h.removeFromRight(6);
        playAllBtn.setBounds(h.removeFromRight(86)); h.removeFromRight(6);
        modAllBtn.setBounds(h.removeFromRight(56)); h.removeFromRight(16);
        auto left = juce::Rectangle<int>(236, h.getY(), 290, h.getHeight());
        syncBox.setBounds(left.removeFromLeft(96)); left.removeFromLeft(6);
        playBtn.setBounds(left.removeFromLeft(56)); left.removeFromLeft(6);
        tempo.setBounds(left);
    }
    chordArea = r.removeFromTop(78).reduced(10, 4);
    {
        auto c = chordArea;
        auto kb = c.removeFromLeft(150);
        keyBox.setBounds(kb.removeFromTop(32).reduced(0, 2));
        modeBox.setBounds(kb.removeFromTop(32).reduced(0, 2));
        c.removeFromLeft(8);
        invertBtn.setBounds(c.removeFromRight(80).withSizeKeepingCentre(80, 32)); c.removeFromRight(8);
        auto types = c.removeFromBottom(26);
        const int bw = c.getWidth() / 7;
        for (int i = 0; i < 7; ++i) chordBtn[i].setBounds(c.removeFromLeft(bw).reduced(3, 2));
        const int tw = types.getWidth() / 7;
        for (int i = 0; i < 7; ++i) typeBtn[i].setBounds(types.removeFromLeft(tw).reduced(3, 1));
    }
    keyboard->setBounds(r.removeFromTop(64).reduced(10, 4));
    auto parts = r.removeFromTop(300).reduced(10, 4);
    layoutParts(parts);
    tabs.setBounds(r.reduced(10, 4));
    if (followerOverlay) followerOverlay->setBounds(0, keyboard->getBottom() + 4, getWidth(), getHeight() - keyboard->getBottom() - 4);
}

void AgoEditor::layoutParts(juce::Rectangle<int> r) {
    const int widths[4] = { 300, 180, 0, 0 };
    const int gap = 8;
    const int rest = (r.getWidth() - widths[0] - widths[1] - 3 * gap) / 2;
    int x = r.getX();
    for (int i = 0; i < 4; ++i) {
        const int w = i < 2 ? widths[i] : rest;
        partArea[i] = { x, r.getY(), w, r.getHeight() };
        x += w + gap;
        auto a = partArea[i].reduced(8);
        auto head = a.removeFromTop(30).withTrimmedTop(4);
        partOn[i].setBounds(head.removeFromLeft(juce::jmin(110, head.getWidth() / 2)));
        head.removeFromLeft(6);
        partMod[i].setBounds(head.removeFromLeft(50));
        a.removeFromTop(6);
        const int cols = i == 0 ? 4 : i == 1 ? 2 : 5;
        if (i >= 2) {
            motifView[i - 2]->setBounds(a.removeFromBottom(62));
            a.removeFromBottom(6);
        }
        const int kw = a.getWidth() / cols;
        const int kh = juce::jmin(98, a.getHeight() / 2);
        for (int k = 0; k < knobs[i].size(); ++k)
            knobs[i][k]->setBounds(a.getX() + (k % cols) * kw, a.getY() + (k / cols) * kh, kw, kh);
    }
}
