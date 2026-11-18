#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParamTable.h"

using namespace ago;
static_assert(std::is_trivially_copyable<Config>::value, "Config deve essere copiabile byte per byte");

juce::AudioProcessorValueTreeState::ParameterLayout AgoProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : agoparams::table()) {
        const juce::ParameterID pid { d.id, 1 };
        if (d.kind == 1) layout.add(std::make_unique<juce::AudioParameterChoice>(pid, d.name, d.labels, d.def));
        else if (d.kind == 2) layout.add(std::make_unique<juce::AudioParameterBool>(pid, d.name, d.def != 0));
        else {
            auto tx = d.text;
            layout.add(std::make_unique<juce::AudioParameterInt>(pid, d.name, d.lo, d.hi, d.def,
                juce::AudioParameterIntAttributes().withStringFromValueFunction([tx](int v, int) { return tx ? tx(v) : juce::String(v); })));
        }
    }
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tempo", 1 }, "Tempo (interno)",
        juce::NormalisableRange<float>(10.f, 300.f, 0.5f), 120.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "sync", 1 }, "Clock",
        juce::StringArray { "Host", "Internal", "MIDI Clock" }, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "play", 1 }, "Play (clock interno)", false));
    return layout;
}

AgoProcessor::AgoProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AGO", createLayout()) {
    for (auto& d : agoparams::table()) raw.push_back(apvts.getRawParameterValue(d.id));
    loadFactory(cfgMaster);
    cfgPending = cfgMaster;
    evs.reserve(1024);
    actions.reserve(64);
    portStatus[1] = portStatus[2] = "non assegnata";
    startTimerHz(20);
}

AgoProcessor::~AgoProcessor() { stopTimer(); }

void AgoProcessor::prepareToPlay(double sampleRate, int) {
    sr = sampleRate;
    outHost.ensureSize(4096); outA.ensureSize(4096); outB.ensureSize(4096);
}

int AgoProcessor::paramValue(juce::AudioProcessorValueTreeState& s, const char* id) {
    if (auto* p = s.getRawParameterValue(id)) return juce::roundToInt(p->load());
    return 0;
}

void AgoProcessor::setParam(const juce::String& id, int value) {
    if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1((float) value));
}

void AgoProcessor::setConfig(const Config& c) {
    cfgMaster = c;
    {
        const juce::SpinLock::ScopedLockType l(cfgLock);
        cfgPending = c;
    }
    cfgDirty = true;
}

void AgoProcessor::playAll() {
    const char* ids[4] = {"padOn", "droneOn", "m1On", "m2On"};
    bool any = false;
    for (auto* id : ids) any |= paramValue(apvts, id) != 0;
    for (auto* id : ids) setParam(id, any ? 0 : 1);
    if (!any) requestResync();
}

void AgoProcessor::readParams() {
    const auto& t = agoparams::table();
    for (size_t i = 0; i < t.size(); ++i) t[i].ref(engine.base) = juce::roundToInt(raw[i]->load());
}

void AgoProcessor::routeEvents(int sample, int numSamples) {
    sample = juce::jlimit(0, juce::jmax(0, numSamples - 1), sample);
    for (auto& e : evs) {
        const int st = e.b0 & 0xF0;
        const int size = (st == 0xC0 || st == 0xD0) ? 2 : 3;
        const uint8_t data[3] = { e.b0, e.b1, e.b2 };
        if (e.portMask & 1) outHost.addEvent(data, size, sample);
        if (e.portMask & 2) outA.addEvent(data, size, sample);
        if (e.portMask & 4) outB.addEvent(data, size, sample);
    }
    evs.clear();
}

void AgoProcessor::applyActions() {
    for (auto& a : actions) {
        if (a.dest == ACT_TEMPO) { setParam("tempo", a.value); continue; }
        if (a.dest == ACT_INVERT) { setParam("invert", a.value); engine.base.invert = a.value; continue; }
        if (a.dest == ACT_RLEN1 || a.dest == ACT_RLEN2) {
            const int k = a.dest - ACT_RLEN1;
            setParam(k == 0 ? "m1RLen" : "m2RLen", a.value); engine.base.m[k].rlen = a.value; continue;
        }
        if (a.dest == ACT_PLAYALL || a.dest == ACT_LOADSEQ) {
            if (a.dest == ACT_LOADSEQ) engine.cfg.activeSeq = a.value;
            int s1, n1, s2, n2;
            actFifo.prepareToWrite(1, s1, n1, s2, n2);
            if (n1 > 0) { actBuf[(size_t) s1] = a; actFifo.finishedWrite(1); }
            continue;
        }
        if (auto* d = agoparams::byDest(a.dest)) {
            setParam(d->id, a.value);
            d->ref(engine.base) = a.value;
        }
    }
    actions.clear();
}

void AgoProcessor::runTicks(int64_t fromTick, int64_t toTick, double samplesPerTick, double firstTickSample, int numSamples) {
    for (int64_t t = fromTick; t < toTick; ++t) {
        engine.tick(t, evs);
        routeEvents((int) std::floor(firstTickSample + (double) (t - fromTick) * samplesPerTick), numSamples);
    }
}

void AgoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals nd;
    buffer.clear();
    const int n = buffer.getNumSamples();

    if (cfgDirty.load()) {
        const juce::SpinLock::ScopedTryLockType l(cfgLock);
        if (l.isLocked()) { engine.cfg = cfgPending; cfgDirty = false; }
    }
    readParams();
    outHost.clear(); outA.clear(); outB.clear();

    if (panicRequest.exchange(false)) { engine.panic(evs); routeEvents(0, n); }
    if (resyncRequest.exchange(false)) engine.resyncMotifs();

    int sync = paramValue(apvts, "sync");
    juce::Optional<juce::AudioPlayHead::PositionInfo> pos;
    if (auto* ph = getPlayHead()) pos = ph->getPosition();
    const bool hostOk = pos.hasValue() && pos->getPpqPosition().hasValue() && pos->getBpm().hasValue();
    if (sync == 0 && !hostOk) sync = 1;    // standalone: clock interno

    // ---- ingresso MIDI (controllo, trasposizione, clock)
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        const int s = meta.samplePosition;
        if (m.isMidiClock() || m.isMidiStart() || m.isMidiContinue() || m.isMidiStop()) {
            if (sync != 2) continue;
            if (m.isMidiStart()) { clockTick = 0; engine.start(0, evs); clockRunning = true; routeEvents(s, n); }
            else if (m.isMidiContinue()) { engine.start(clockTick * 4, evs); clockRunning = true; routeEvents(s, n); }
            else if (m.isMidiStop()) { engine.stop(evs); clockRunning = false; routeEvents(s, n); }
            else {
                const double abs = (double) (sampleCounter + s);
                if (lastClockSample >= 0) {
                    const double iv = abs - lastClockSample;
                    clockInterval = clockInterval <= 0 ? iv : clockInterval * 0.9 + iv * 0.1;
                    const double bpm = 60.0 * sr / (clockInterval * 24.0);
                    engine.setTempo(juce::jlimit(10.0, 400.0, bpm));
                    shownTempo = bpm;
                }
                lastClockSample = abs;
                if (clockRunning) {
                    const double spt = clockInterval > 0 ? clockInterval / 4.0 : 0.0;
                    runTicks(clockTick * 4, clockTick * 4 + 4, spt, (double) s, n);
                    ++clockTick;
                }
            }
            continue;
        }
        if (m.isSysEx()) continue;
        engine.midiIn(m.getRawData(), m.getRawDataSize(), evs, actions);
        routeEvents(s, n);
        if (!actions.empty()) applyActions();
    }

    // ---- avanzamento del tempo
    if (sync == 0) {
        const double bpm = *pos->getBpm();
        engine.setTempo(bpm); shownTempo = bpm;
        const bool playing = pos->getIsPlaying();
        const double ppq0 = *pos->getPpqPosition();
        const double spb = sr * 60.0 / bpm;
        const double ppq1 = ppq0 + n / spb;
        if (playing) {
            const int64_t first = (int64_t) std::ceil(ppq0 * kTicksPerBeat - 1e-6);
            const int64_t last = (int64_t) std::ceil(ppq1 * kTicksPerBeat - 1e-6);
            int64_t from = first;
            if (!hostWasPlaying || std::llabs(first - nextTick) > 2) { engine.start(first, evs); routeEvents(0, n); }
            else from = nextTick;
            for (int64_t t = from; t < last; ++t) {
                engine.tick(t, evs);
                routeEvents((int) std::floor((t / (double) kTicksPerBeat - ppq0) * spb), n);
            }
            nextTick = std::max(last, from);
        } else if (hostWasPlaying) { engine.stop(evs); routeEvents(0, n); }
        hostWasPlaying = playing;
        transportOn = playing;
    } else if (sync == 1) {
        const double bpm = juce::jlimit(10.0, 300.0, (double) apvts.getRawParameterValue("tempo")->load());
        engine.setTempo(bpm); shownTempo = bpm;
        const bool playing = paramValue(apvts, "play") != 0;
        if (playing) {
            if (!internalWasPlaying) { internalPpq = 0; nextTick = 0; engine.start(0, evs); routeEvents(0, n); }
            const double spb = sr * 60.0 / bpm;
            const double ppq1 = internalPpq + n / spb;
            const int64_t last = (int64_t) std::ceil(ppq1 * kTicksPerBeat - 1e-6);
            for (int64_t t = nextTick; t < last; ++t) {
                engine.tick(t, evs);
                routeEvents((int) std::floor((t / (double) kTicksPerBeat - internalPpq) * spb), n);
            }
            nextTick = last;
            internalPpq = ppq1;
        } else if (internalWasPlaying) { engine.stop(evs); routeEvents(0, n); }
        internalWasPlaying = playing;
        hostWasPlaying = false;
        transportOn = playing;
    } else {
        transportOn = clockRunning;
        if (hostWasPlaying || internalWasPlaying) { engine.stop(evs); routeEvents(0, n); hostWasPlaying = internalWasPlaying = false; }
    }
    if (sync != 1 && internalWasPlaying) internalWasPlaying = false;
    sampleCounter += n;

    // ---- uscite
    midi.swapWith(outHost);
    if (!outA.isEmpty() || !outB.isEmpty()) {
        const juce::ScopedTryLock l(portLock);
        if (l.isLocked()) {
            const double now = juce::Time::getMillisecondCounterHiRes() + midiOutDelayMs;
            if (port[1] && !outA.isEmpty()) port[1]->sendBlockOfMessages(outA, now, sr);
            if (port[2] && !outB.isEmpty()) port[2]->sendBlockOfMessages(outB, now, sr);
        }
    }

    if (++snapCounter >= 3) {
        snapCounter = 0;
        const juce::SpinLock::ScopedTryLockType l(snapLock);
        if (l.isLocked()) engine.fillSnapshot(snap);
    }
}

// ------------------------------------------------------------------ porte MIDI
void AgoProcessor::setPortDevice(int p, const juce::String& identifier) {
    if (p < 1 || p > 2) return;
    portId[p] = identifier;
    const juce::ScopedLock l(portLock);
    port[p].reset();
    portStatus[p] = "non assegnata";
    if (identifier.isEmpty()) return;
    port[p] = juce::MidiOutput::openDevice(identifier);
    if (port[p]) { port[p]->startBackgroundThread(); portStatus[p] = "aperta: " + port[p]->getName(); }
    else portStatus[p] = "impossibile aprirla (gia' usata da un altro programma?)";
}

void AgoProcessor::openPorts() { for (int p = 1; p <= 2; ++p) setPortDevice(p, portId[p]); }

void AgoProcessor::timerCallback() {
    if (portsDirty.exchange(false)) openPorts();
    int s1, n1, s2, n2;
    actFifo.prepareToRead(actFifo.getNumReady(), s1, n1, s2, n2);
    auto handle = [this](const ParamAction& a) {
        if (a.dest == ACT_PLAYALL) {
            const char* ids[4] = {"padOn", "droneOn", "m1On", "m2On"};
            for (auto* id : ids) setParam(id, a.value);
            if (a.value) requestResync();
        } else if (a.dest == ACT_LOADSEQ) {
            Config c = cfgMaster; c.activeSeq = a.value; setConfig(c); ++configSerial;
        }
    };
    for (int i = 0; i < n1; ++i) handle(actBuf[(size_t) (s1 + i)]);
    for (int i = 0; i < n2; ++i) handle(actBuf[(size_t) (s2 + i)]);
    actFifo.finishedRead(n1 + n2);
}

// ------------------------------------------------------------------ stato
void AgoProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state = apvts.copyState();
    juce::ValueTree c("AGOCFG");
    juce::MemoryBlock mb(&cfgMaster, sizeof(Config));
    c.setProperty("ver", 1, nullptr);
    c.setProperty("size", (int) sizeof(Config), nullptr);
    c.setProperty("data", mb.toBase64Encoding(), nullptr);
    c.setProperty("portA", portId[1], nullptr);
    c.setProperty("portB", portId[2], nullptr);
    c.setProperty("delay", midiOutDelayMs, nullptr);
    state.removeChild(state.getChildWithName("AGOCFG"), nullptr);
    state.appendChild(c, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void AgoProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (!xml) return;
    auto state = juce::ValueTree::fromXml(*xml);
    auto c = state.getChildWithName("AGOCFG");
    if (c.isValid()) {
        juce::MemoryBlock mb;
        if ((int) c.getProperty("size") == (int) sizeof(Config) && mb.fromBase64Encoding(c.getProperty("data").toString())
            && mb.getSize() == sizeof(Config)) {
            Config cf; std::memcpy(&cf, mb.getData(), sizeof(Config));
            setConfig(cf);
        }
        portId[1] = c.getProperty("portA").toString();
        portId[2] = c.getProperty("portB").toString();
        midiOutDelayMs = (int) c.getProperty("delay", 0);
        state.removeChild(c, nullptr);
    }
    apvts.replaceState(state);
    portsDirty = true;   // le porte si riaprono sul message thread (timer)
    ++configSerial;
}

juce::AudioProcessorEditor* AgoProcessor::createEditor() { return new AgoEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AgoProcessor(); }
