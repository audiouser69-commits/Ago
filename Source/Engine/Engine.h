// AGO - motore: 4 parti (Pad, Drone, Motif 1/2), matrice di modulazione, Perlin, sequencer di accordi,
// controllo da tastiera MIDI, Rotator e Interval mode. Nessuna dipendenza da JUCE (testabile da riga di comando).
#pragma once
#include "Model.h"
#include "Perlin.h"
#include <vector>
#include <cstdint>
#include <random>

namespace ago {

// part: 0 pad, 1 drone, 2/3 motif, -1 CC da modulazione, 5 tastiera, 6 thru, 9 panic (vale per tutti)
struct MidiEv { int portMask; uint8_t b0, b1, b2; int part = -1; };

// Azione verso l'host: cambia un parametro (es. accordo scelto da tastiera MIDI)
struct ParamAction { int dest; int value; };   // dest = D_* (interno); D_NONE+value speciali sotto
enum { ACT_TEMPO = 10000, ACT_PLAYALL = 10001, ACT_LOADSEQ = 10002, ACT_INVERT = 10003, ACT_RLEN1 = 10010, ACT_RLEN2 = 10011 };

enum PartId { P_PAD = 0, P_DRONE, P_M1, P_M2 };

struct UiSnapshot {
    Chord chord;
    bool sounding[4][128] = {};
    int motifStep[2] = {-1, -1};     // indice nel pattern dell'ultima nota
    int motifBeat[2] = {-1, -1};     // battito del ritmo
    bool partRunning[4] = {};
    int seqStep = -1;                // indice dello step (sezioni) o della riga (song); -1 = fermo
    int seqRep = 0;                  // ripetizione corrente della riga (song)
    bool seqPlaying = false;
    int nz[2][16] = {};
    double lfo[3] = {};
    Params eff;
};

class Engine {
public:
    Engine();

    Params base;   // valori dai parametri dell'host/UI
    Config cfg;    // configurazione e editor

    void setTempo(double bpm) { tempo = bpm; }
    double getTempo() const { return tempo; }

    // Trasporto: start allinea tutto alla battuta che contiene t (fast-forward silenzioso)
    void start(int64_t t, std::vector<MidiEv>& out);
    void stop(std::vector<MidiEv>& out);
    bool isRunning() const { return running; }
    void tick(int64_t t, std::vector<MidiEv>& out);

    void midiIn(const uint8_t* d, int len, std::vector<MidiEv>& out, std::vector<ParamAction>& actions);
    void panic(std::vector<MidiEv>& out);
    void resyncMotifs() { resyncRequest = true; }

    void fillSnapshot(UiSnapshot& s) const;

    // utilita' pubbliche (usate anche dalla UI e dai test)
    static int stepTicksForDiv(int div);
    static int patternIndexFor(int64_t count, int len, int var, uint32_t& rng);
    static bool euclidHit(int i, int len, int hits, int rot);
    int rhythmValue(const Rhythm& r, int beat, int len) const;
    int noteForValue(int value, int type, int pos, const Chord& c) const;
    void computeNZ(const Params& p, int motif, int out[16]) const;
    std::vector<int> computePadNotes(const Params& p, const Chord& c, int posOverride, uint32_t seed) const;
    std::vector<int> computeDroneNotes(const Params& p, const Chord& c) const;
    const Params& effective() const { return eff; }
    Chord currentChord() const { return chordFrom(eff); }

private:
    struct Sound { int note, ch, mask; };
    struct Pending { int64_t t; int part; bool on; int note, ch, mask, vel; };

    double tempo = 120.0;
    bool running = false;
    bool silent = false;
    int64_t curTick = 0;
    double seconds = 0.0;
    std::mt19937 rng { 12345 };
    uint32_t rngState = 777;
    Perlin perlin;

    Params eff;
    Params prevEff;

    // modulazione
    int64_t lfoCycle[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
    bool lfoActive[8] = {true,true,true,true,true,true,true,true};
    double lfoRand[8] = {};
    double lfoVal[8] = {};       // ultimo valore (unipolare o bipolare)
    double lfoPhase[8] = {};
    int midiVel = 0, midiPB = 8192, midiMW = 0, midiAT = 0;
    int stepOverride[2][16];
    int lastCC[8] = {-1,-1,-1,-1,-1,-1,-1,-1};

    // accordo
    Chord appliedChord;          // accordo usato da pad/drone (quantizzato)
    bool chordPending = false;
    uint32_t chordSerial = 1;

    // pad
    bool padRunning = false, padArmed = false;
    std::vector<Sound> padSnd;
    int padInvertShift = 0;
    int lastPadKey[6] = {-1,-1,-1,-1,-1,-1};

    // drone
    bool droneRunning = false, droneArmed = false;
    std::vector<Sound> droneSnd;
    int lastDroneKey[3] = {-1,-1,-1};

    // motif
    struct MotifRt {
        bool running = false, armed = false;
        int64_t startTick = 0;
        int64_t stepCount = 0;
        int64_t noteCount = 0;
        int lastIdx = -1, lastBeat = -1;
        bool hasNote = false; Sound snd {0,0,0};
        int64_t offTick = 0;
    } mrt[2];
    bool resyncRequest = false;

    // sequencer accordi
    bool seqRunning = false; int64_t seqStart = 0; int seqStepIdx = -1; int seqRepIdx = 0;
    int seqKey = 0, seqMode = 0, seqDeg = 0, seqType = 0;
    bool prevSeqArm = false;

    std::vector<Pending> pending;

    // tastiera
    int kbHeld[128];             // nota in ingresso -> slot uscite
    std::vector<int> kbOut[128];
    int rotIdx = 0;
    int ivLast = -1, ivOct = 0, ivLow = 36, ivHigh = 96; bool ivFollow = false;
    int m4First = -1;

    Chord chordFrom(const Params& p) const;
    void computeEffective();
    double sourceValue(int src, bool& bipolar, bool& isPatt, int& pattVal);
    void updateLfos();

    void emitOn(int part, int note, int ch, int mask, int vel, std::vector<MidiEv>& out);
    void emitOff(int part, int note, int ch, int mask, std::vector<MidiEv>& out);
    void allOff(std::vector<Sound>& snd, int part, std::vector<MidiEv>& out);
    void processPending(int64_t t, std::vector<MidiEv>& out);
    void dropPendingOns(int part);

    void updatePad(int64_t t, bool chordChangeNow, std::vector<MidiEv>& out);
    void updateDrone(int64_t t, bool chordChangeNow, std::vector<MidiEv>& out);
    void motifTick(int k, int64_t t, std::vector<MidiEv>& out);
    void sendCC(int ch, int mask, int cc, int val, std::vector<MidiEv>& out);

    int padChanFor(int i) const;
    int quantTicks() const;
    int motifVelocity(int k, int beatVal, int64_t t);

    void keyboardNote(bool on, int note, int vel, std::vector<MidiEv>& out);
};

} // namespace ago
