#pragma once
#include <JuceHeader.h>
#include "Engine/Engine.h"

class AgoProcessor : public juce::AudioProcessor, private juce::Timer {
public:
    AgoProcessor();
    ~AgoProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AGO"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // configurazione: copia "master" sul message thread, spinta all'audio thread
    const ago::Config& getConfig() const { return cfgMaster; }
    void setConfig(const ago::Config& c);
    int getConfigSerial() const { return configSerial.load(); }

    // porte MIDI esterne (A e B)
    juce::String getPortDevice(int port) const { return portId[port]; }
    void setPortDevice(int port, const juce::String& identifier);
    juce::String getPortStatus(int port) const { return portStatus[port]; }
    int midiOutDelayMs = 0;

    void getSnapshot(ago::UiSnapshot& s) { const juce::SpinLock::ScopedLockType l(snapLock); s = snap; }
    void requestPanic() { panicRequest = true; }
    void requestResync() { resyncRequest = true; }
    void playAll();
    double getCurrentTempo() const { return shownTempo.load(); }
    // 0 off, 1 leader, 2 leader doppio (fa da follower), 3 follower collegato, 4 follower senza leader, 5 manca il trasporto host
    int getLinkStatus() const { return linkStatus.load(); }
    bool isTransportRunning() const { return transportOn.load(); }

    static int paramValue(juce::AudioProcessorValueTreeState& s, const char* id);
    void setParam(const juce::String& id, int value);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void readParams();
    void applyActions();
    void runTicks(int64_t fromTick, int64_t toTick, double samplesPerTick, double firstTickSample, int numSamples);
    void routeEvents(int sample, int numSamples);
    void timerCallback() override;
    void openPorts();
    void processLinked(int role, int group, int part, juce::MidiBuffer& midi, int n,
                       const juce::Optional<juce::AudioPlayHead::PositionInfo>& pos, bool hostOk);
    void leaveBus();

    ago::Engine engine;
    ago::Config cfgMaster, cfgPending;
    juce::SpinLock cfgLock;
    std::atomic<bool> cfgDirty { true };
    std::atomic<int> configSerial { 1 };

    juce::SpinLock snapLock;
    ago::UiSnapshot snap;
    int snapCounter = 0;

    double sr = 44100.0;
    bool hostWasPlaying = false;
    int64_t nextTick = 0;          // prossimo tick atteso (sync host)
    double internalPpq = 0.0;
    bool internalWasPlaying = false;
    // MIDI clock in
    bool clockRunning = false; int64_t clockTick = 0; double lastClockSample = -1, clockInterval = 0; int64_t sampleCounter = 0;
    std::atomic<double> shownTempo { 120.0 };
    std::atomic<bool> transportOn { false };
    std::atomic<bool> panicRequest { false }, resyncRequest { false }, portsDirty { false };
    std::atomic<int> linkStatus { 0 };
    int lastLinkRole = 0, lastLinkGroup = 0;
    uint64_t myImmId = 0;
    int publishedCfgSerial = 0;
    std::atomic<int> localCfgSerial { 1 };

    std::vector<ago::MidiEv> evs;
    std::vector<ago::ParamAction> actions;
    juce::MidiBuffer outHost, outA, outB;

    juce::String portId[3];
    juce::String portStatus[3];
    std::unique_ptr<juce::MidiOutput> port[3];
    juce::CriticalSection portLock;

    // azioni da applicare ai parametri sul message thread
    juce::AbstractFifo actFifo { 256 };
    std::array<ago::ParamAction, 256> actBuf;

    std::vector<std::atomic<float>*> raw;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgoProcessor)
};
