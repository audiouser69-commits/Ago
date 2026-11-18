// AGO Link: un motore condiviso per gruppo tra tutte le istanze caricate nello stesso processo (es. Ableton Live).
// Il leader pubblica parametri e configurazione; la prima istanza che processa un intervallo di tempo fa avanzare
// il motore, le altre leggono gli eventi gia' calcolati. Ogni follower estrae solo la propria parte.
#pragma once
#include <JuceHeader.h>
#include "Engine/Engine.h"

struct LinkBus {
    struct TimedEv { int64_t tick; ago::MidiEv ev; };
    struct ImmEv { uint64_t id; ago::MidiEv ev; };

    juce::SpinLock lock;
    ago::Engine engine;
    void* leader = nullptr;
    int cfgSerial = -1;
    bool running = false;
    int64_t nextTick = 0;        // primo tick non ancora calcolato
    int64_t rangeFrom = 0;       // inizio dell'ultimo intervallo calcolato
    std::vector<TimedEv> buffer; // eventi temporizzati recenti
    std::vector<ImmEv> imm;      // eventi immediati (start/stop/panic)
    uint64_t immNext = 1;
    double tempo = 120.0;

    void pushImmediate(std::vector<ago::MidiEv>& evs) {
        for (auto& e : evs) imm.push_back({ immNext++, e });
        evs.clear();
        if (imm.size() > 4096) imm.erase(imm.begin(), imm.begin() + (long) (imm.size() - 4096));
    }

    void advanceTo(int64_t last, std::vector<ago::MidiEv>& scratch) {
        for (int64_t t = nextTick; t < last; ++t) {
            engine.tick(t, scratch);
            for (auto& e : scratch) buffer.push_back({ t, e });
            scratch.clear();
        }
        if (last > nextTick) nextTick = last;
    }

    void prune(int64_t before) {
        size_t k = 0;
        while (k < buffer.size() && buffer[k].tick < before) ++k;
        if (k > 0) buffer.erase(buffer.begin(), buffer.begin() + (long) k);
    }

    static LinkBus& get(int group) {
        static LinkBus buses[4];
        return buses[juce::jlimit(0, 3, group)];
    }

    LinkBus() { buffer.reserve(8192); imm.reserve(4096); }
};
