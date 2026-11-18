// AGO - modello dati: parametri di performance, configurazione, pattern/ritmi di fabbrica
#pragma once
#include "Theory.h"
#include <cstdint>
#include <cstring>

namespace ago {

constexpr int kTicksPerBeat = 96;      // risoluzione interna (96 PPQ)
constexpr int kNumPatterns = 40;       // 1-20 fabbrica, 21-40 utente
constexpr int kNumRhythms = 40;
constexpr int kPatternNZ = 40;         // indice speciale: pattern Perlin "NZ"
constexpr int kRest = 0, kTie = -1;

enum PatType { PT_CHORD = 0, PT_SCALE, PT_CHROM };
inline const char* patTypeName(int t) { static const char* n[3] = {"CHORD","SCALE","CHROM"}; return n[std::clamp(t,0,2)]; }
inline int patTypeMax(int t) { return t == PT_CHORD ? 20 : t == PT_SCALE ? 40 : 60; }

struct Pattern { int type = PT_CHORD; int v[16] = {}; };
struct Rhythm  { int euclid = 0; int hits = 5; int rot = 0; int v[32] = {}; };

// Porte MIDI di uscita (come le porte dell'NDLR): Host = uscita MIDI del plugin, A/B = dispositivi Windows
constexpr int kNumPortChoices = 7;
inline const char* portName(int p) {
    static const char* n[kNumPortChoices] = {"Host","Port A","Port B","Host+A","Host+B","A+B","All"};
    return n[std::clamp(p, 0, kNumPortChoices - 1)];
}
inline int portMask(int p) { static const int m[kNumPortChoices] = {1,2,4,3,5,6,7}; return m[std::clamp(p,0,kNumPortChoices-1)]; }

// ---------------------------------------------------------------- parametri di performance
struct MotifP {
    int on = 0, pos = 6, plen = 8, var = 0, patt = 0, div = 3, rlen = 8, accent = 0, rhy = 0, gate = 64, vel = 100;
};

struct Params {
    int key = 0, mode = 0, degree = 0, ctype = 0, invert = 0;
    int padOn = 0, padPos = 55, padRange = 4, padStrum = 0, padSpread = 0, padVel = 90, padQuant = 0, polyChain = 1;
    int droneOn = 0, droneOct = 1, droneType = 0, droneTrig = 0, droneVel = 90;
    MotifP m[2];
    int offset = 0, humanize = 0, velLow = 50;
    int px = 20, py = 20, zoom = 30, spc = 20, ruff = 60, per = 50;
    int modAll = 1;
    int seqArm = 0;
    int modPart[4] = {1, 1, 1, 1};   // pad, drone, motif1, motif2
};

// ---------------------------------------------------------------- modulazione
enum ModSrc { SRC_OFF = 0, SRC_LFO1, SRC_LFO2, SRC_LFO3, SRC_MOD1, SRC_MOD2, SRC_MOD3, SRC_MOD4, SRC_MOD5,
              SRC_VEL, SRC_PB, SRC_MW, SRC_AT, kNumSrc };
inline const char* srcName(int s) {
    static const char* n[kNumSrc] = {"SRC OFF","LFO 1","LFO 2","LFO 3","MOD 1","MOD 2","MOD 3","MOD 4","MOD 5",
                                     "Velocity","Pitch Bend","Mod Wheel","After Touch"};
    return n[std::clamp(s, 0, kNumSrc - 1)];
}

enum LfoShape { SH_SINE = 0, SH_TRI, SH_RAMP, SH_SAW, SH_SQUARE, SH_PULSE, SH_RANDOM, SH_PATT, kNumShapes };
inline const char* shapeName(int s) {
    static const char* n[kNumShapes] = {"Sine","Triangle","Ramp","Saw","Square","Pulse 80/20","Random","PATT"};
    return n[std::clamp(s, 0, kNumShapes - 1)];
}

struct LfoCfg { int synced = 1; int rateIdx = 7; int shape = SH_SINE; int patt = 0; int prob = 100; };

// velocita' LFO: non sincronizzato 0.4s..4.8s (passo 0.2) poi 5..40s (passo 1); sincronizzato 1..48 beat
inline int lfoRateCount(bool synced) { return synced ? 48 : 23 + 36; }
inline double lfoRateValue(bool synced, int idx) {
    if (synced) return std::clamp(idx, 0, 47) + 1;
    idx = std::clamp(idx, 0, 58);
    return idx < 23 ? 0.4 + 0.2 * idx : 5.0 + (idx - 23);
}

struct ModSlot { int src = SRC_OFF; int dst = 0; int amount = 0; int ccChan = 1; int ccPort = 0; };

// Destinazioni: ognuna ha range interno [lo,hi] e un offset di visualizzazione (valore mostrato = interno + disp)
struct DestInfo { const char* name; int lo, hi, disp, part; }; // part: -1 globale, 0 pad, 1 drone, 2 m1, 3 m2

enum {
    D_NONE = 0, D_KEY, D_MODE, D_DEGREE, D_CTYPE,
    D_PAD_POS, D_PAD_RANGE, D_PAD_VEL, D_PAD_ON, D_PAD_STRUM, D_PAD_SPREAD,
    D_DR_POS, D_DR_TYPE, D_DR_TRIG, D_DR_ON,
    D_M1_GATE, D_M1_POS, D_M1_PLEN, D_M1_VAR, D_M1_PATT, D_M1_DIV, D_M1_ACC, D_M1_RHY, D_M1_VEL, D_M1_ON,
    D_M2_GATE, D_M2_POS, D_M2_PLEN, D_M2_VAR, D_M2_PATT, D_M2_DIV, D_M2_ACC, D_M2_RHY, D_M2_VEL, D_M2_ON,
    D_VEL_LOW, D_HUMANIZE,
    D_PN_X, D_PN_Y, D_PN_ZOOM, D_PN_SPC, D_PN_RUFF, D_PN_PER,
    D_M1_STEP1,                  // 16 step Motif 1
    D_M2_STEP1 = D_M1_STEP1 + 16, // 16 step Motif 2
    D_CC1 = D_M2_STEP1 + 16,      // CC 1..127
    kNumDests = D_CC1 + 127
};

inline DestInfo destInfo(int d) {
    static const DestInfo base[] = {
        {"OFF",0,0,0,-1},
        {"KEY",0,11,1,-1},{"MODE",0,kNumModes-1,1,-1},{"CHORD DEGREE",0,6,1,-1},{"CHORD TYPE",0,6,1,-1},
        {"PAD POSITION",20,108,0,0},{"PAD RANGE",1,22,0,0},{"PAD VELOCITY",0,127,0,0},{"PAD ON/OFF",0,1,0,0},
        {"PAD STRUM",0,6,1,0},{"PAD SPREAD",0,5,1,0},
        {"DRONE POSITION",0,4,0,1},{"DRONE TYPE",0,3,1,1},{"DRONE TRIGGER",0,18,1,1},{"DRONE ON/OFF",0,1,0,1},
        {"M1 GATE",5,127,0,2},{"M1 POSITION",0,24,0,2},{"M1 PATT LEN",1,16,0,2},{"M1 VARIANT",0,5,1,2},
        {"M1 PATTERN",0,40,1,2},{"M1 CLK DIV",0,6,1,2},{"M1 ACCENT",0,9,1,2},{"M1 RHYTHM",0,39,1,2},
        {"M1 VELOCITY",0,127,0,2},{"M1 ON/OFF",0,1,0,2},
        {"M2 GATE",5,127,0,3},{"M2 POSITION",0,24,0,3},{"M2 PATT LEN",1,16,0,3},{"M2 VARIANT",0,5,1,3},
        {"M2 PATTERN",0,40,1,3},{"M2 CLK DIV",0,6,1,3},{"M2 ACCENT",0,9,1,3},{"M2 RHYTHM",0,39,1,3},
        {"M2 VELOCITY",0,127,0,3},{"M2 ON/OFF",0,1,0,3},
        {"VELOCITY LOW",1,100,0,-1},{"HUMANIZE",0,10,0,-1},
        {"PERLIN X",0,100,0,-1},{"PERLIN Y",0,100,0,-1},{"PERLIN ZOOM",0,100,0,-1},{"PERLIN SPC",0,100,0,-1},
        {"PERLIN RUFF",0,100,0,-1},{"PERLIN PER",0,100,0,-1},
    };
    static char stepNames[32][16];
    static char ccNames[127][10];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 16; ++i) {
            std::snprintf(stepNames[i], 16, "M1 STEP %d", i + 1);
            std::snprintf(stepNames[16 + i], 16, "M2 STEP %d", i + 1);
        }
        for (int i = 0; i < 127; ++i) std::snprintf(ccNames[i], 10, "CC %d", i + 1);
        init = true;
    }
    if (d < D_M1_STEP1) return base[std::clamp(d, 0, D_M1_STEP1 - 1)];
    if (d < D_CC1) { int i = d - D_M1_STEP1; return {stepNames[i], 1, 60, 0, i < 16 ? 2 : 3}; }
    return {ccNames[std::clamp(d - D_CC1, 0, 126)], 0, 127, 0, -1};
}

inline int* destPtr(Params& p, int d) {
    switch (d) {
        case D_KEY: return &p.key; case D_MODE: return &p.mode; case D_DEGREE: return &p.degree; case D_CTYPE: return &p.ctype;
        case D_PAD_POS: return &p.padPos; case D_PAD_RANGE: return &p.padRange; case D_PAD_VEL: return &p.padVel;
        case D_PAD_ON: return &p.padOn; case D_PAD_STRUM: return &p.padStrum; case D_PAD_SPREAD: return &p.padSpread;
        case D_DR_POS: return &p.droneOct; case D_DR_TYPE: return &p.droneType; case D_DR_TRIG: return &p.droneTrig; case D_DR_ON: return &p.droneOn;
        case D_VEL_LOW: return &p.velLow; case D_HUMANIZE: return &p.humanize;
        case D_PN_X: return &p.px; case D_PN_Y: return &p.py; case D_PN_ZOOM: return &p.zoom;
        case D_PN_SPC: return &p.spc; case D_PN_RUFF: return &p.ruff; case D_PN_PER: return &p.per;
        default: break;
    }
    for (int k = 0; k < 2; ++k) {
        int b = k == 0 ? D_M1_GATE : D_M2_GATE;
        MotifP& m = p.m[k];
        switch (d - b) {
            case 0: return &m.gate; case 1: return &m.pos; case 2: return &m.plen; case 3: return &m.var;
            case 4: return &m.patt; case 5: return &m.div; case 6: return &m.accent; case 7: return &m.rhy;
            case 8: return &m.vel; case 9: return &m.on; default: break;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------- sequencer di accordi
struct ChordStep { int dur8 = 0; int key = 0; int mode = 0; int degree = 0; int type = 0; };
// riga della SONG: grado/tipo, tonalita' e modo (-1 = quelli globali), durata in ottavi, ripetizioni
constexpr int kMaxSongRows = 64;
struct SongRow { int degree = 0; int type = 0; int key = -1; int mode = -1; int dur8 = 8; int reps = 1; };

struct ChordSeq {
    ChordStep sec[3][6];
    int songSec[8] = {0, 1, 0, 2, 0, 0, 0, 0};
    int songRep[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int mode = 1;                 // 0 = sezioni A/B/C (come l'NDLR), 1 = SONG a righe
    int numRows = 0;
    SongRow rows[kMaxSongRows];
};

// durate disponibili per le righe (in ottavi) ed etichette
constexpr int kNumRowDurs = 10;
inline int rowDurValue(int i) { static const int v[kNumRowDurs] = {2, 4, 6, 8, 12, 16, 24, 32, 48, 64}; return v[std::clamp(i, 0, kNumRowDurs - 1)]; }
inline const char* rowDurName(int i) {
    static const char* n[kNumRowDurs] = {"1/4 batt.", "1/2 batt.", "3/4 batt.", "1 battuta", "1.5 batt.", "2 battute", "3 battute", "4 battute", "6 battute", "8 battute"};
    return n[std::clamp(i, 0, kNumRowDurs - 1)];
}
inline int rowDurIndex(int dur8) { for (int i = 0; i < kNumRowDurs; ++i) if (rowDurValue(i) == dur8) return i; return 3; }

// ---------------------------------------------------------------- configurazione (non automatizzabile)
struct Config {
    Pattern patterns[kNumPatterns];
    Rhythm rhythms[kNumRhythms];
    LfoCfg lfo[3];
    ModSlot mods[8];
    ChordSeq seqs[5];
    int activeSeq = 0;

    // MIDI: canali 1..16, porte (indice portName)
    int padChan = 1, droneChan = 2, m1Chan = 3, m2Chan = 4;
    int padPort = 0, dronePort = 0, m1Port = 0, m2Port = 0;
    int ctrlChan = 15, transChan = 14, kbMode = 1, kbPort = 0;
    int passThru = 1;
    int rotatorOn = 0, intervalOn = 0;
    int nzPool[2] = {PT_CHORD, PT_SCALE};
    int ccVolcaChan = 0;
};

// ---------------------------------------------------------------- contenuti di fabbrica
inline void fillPattern(Pattern& p, int type, std::initializer_list<int> vals) {
    p.type = type;
    int i = 0, n = (int) vals.size();
    std::vector<int> v(vals);
    for (int k = 0; k < 16; ++k) p.v[k] = v[k % n];
    (void) i;
}

inline void fillRhythm(Rhythm& r, const char* s) {
    int n = (int) std::strlen(s);
    for (int k = 0; k < 32; ++k) {
        char c = s[k % n];
        r.v[k] = c == 'X' ? 127 : c == 'x' ? 100 : c == 'o' ? 72 : c == '1' ? 35 : c == '-' ? kTie : kRest;
    }
}

inline void loadFactory(Config& c) {
    Pattern* p = c.patterns;
    fillPattern(p[0],  PT_CHORD, {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16});
    fillPattern(p[1],  PT_CHORD, {1,2,3,4,5,6,5,4,3,2,1,2,3,4,5,6});
    fillPattern(p[2],  PT_CHORD, {1,3,2,4,3,5,4,6,5,7,6,8,7,9,8,10});
    fillPattern(p[3],  PT_CHORD, {1,1,2,1,3,1,4,1,5,1,4,1,3,1,2,1});
    fillPattern(p[4],  PT_CHORD, {8,7,6,5,4,3,2,1,8,7,6,5,4,3,2,1});
    fillPattern(p[5],  PT_CHORD, {1,4,2,5,3,6,4,7,5,8,6,9,7,10,8,11});
    fillPattern(p[6],  PT_CHORD, {1,2,3,2,4,3,5,4,6,5,4,3,2,3,2,1});
    fillPattern(p[7],  PT_CHORD, {1,3,2,3,1,3,2,3,4,6,5,6,4,6,5,6});
    fillPattern(p[8],  PT_CHORD, {1,4,7,4,2,5,8,5,3,6,9,6,4,7,10,7});
    fillPattern(p[9],  PT_CHORD, {3,2,1,2,3,4,5,4,3,2,1,2,3,4,6,4});
    fillPattern(p[10], PT_SCALE, {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16});
    fillPattern(p[11], PT_SCALE, {1,3,5,3,2,4,6,4,3,5,7,5,4,6,8,6});
    fillPattern(p[12], PT_SCALE, {8,7,6,5,4,3,2,1,8,7,6,5,4,3,2,1});
    fillPattern(p[13], PT_SCALE, {1,2,3,1,4,3,2,5,1,2,3,1,6,5,4,8});
    fillPattern(p[14], PT_SCALE, {1,5,4,3,2,3,4,5,1,5,4,3,2,3,4,8});
    fillPattern(p[15], PT_SCALE, {1,1,8,1,1,8,5,3,1,1,8,1,1,10,8,5});
    fillPattern(p[16], PT_CHROM, {1,13,1,13,8,13,1,11,1,13,1,13,8,13,4,6});
    fillPattern(p[17], PT_CHORD, {1});
    fillPattern(p[18], PT_CHORD, {1,4,1,4,2,5,2,5,3,6,3,6,2,5,2,5});
    fillPattern(p[19], PT_SCALE, {1,3,2,4,3,5,4,6,5,7,6,8,7,9,8,10});
    for (int i = 20; i < 40; ++i) fillPattern(p[i], PT_CHORD, {1,2,3,4,5,6,7,8});
    // pattern 40: intervalli del Rotator (0 = 20). Default: parallela +7, rotanti +3 +5 -2 +10
    fillPattern(p[39], PT_CHROM, {27,23,25,18,30,20,20,20});

    const char* rh[20] = {"XXXXXXXX","X.x.x.x.","X-x-x-x-","X..x..x.","X.xoX.xo","X..x..x...x..x..",
        "X---x---","X.o.x.o.oxo.x.o.","Xxo.Xxo.","X...x...x...x.x.","X--x--x-","x.xx.xx.",
        "X.x..x.x..x.x...","X-------x-------","Xoxo","X..oX.o.X..oXooo","X.......x.......",
        "X-x.x-x.","x1x1X1x1","X.x.x.x."};
    for (int i = 0; i < 20; ++i) fillRhythm(c.rhythms[i], rh[i]);
    c.rhythms[19].euclid = 1; c.rhythms[19].hits = 5; c.rhythms[19].rot = 0;
    for (int i = 20; i < 40; ++i) fillRhythm(c.rhythms[i], "x.x.x.x.");

    c.lfo[0] = {1, 15, SH_SINE, 0, 100};
    c.lfo[1] = {1, 31, SH_TRI, 0, 100};
    c.lfo[2] = {0, 30, SH_RANDOM, 0, 100};

    // sequenza di esempio: A = I-vi-IV-V, B = ii-V, song A A B A
    for (auto& s : c.seqs) {
        int degs[4] = {0, 5, 3, 4};
        for (int i = 0; i < 4; ++i) s.sec[0][i] = {8, 0, 0, degs[i], 0};
        s.sec[1][0] = {8, 0, 0, 1, 1}; s.sec[1][1] = {8, 0, 0, 4, 1};
        s.sec[2][0] = {16, 0, 0, 3, 0};
        s.songSec[0] = 0; s.songRep[0] = 2;
        s.songSec[1] = 1; s.songRep[1] = 1;
        s.songSec[2] = 0; s.songRep[2] = 1;
        // SONG di esempio: I x3, VII x1, IV x2, V x2 (1 battuta ciascuna, tonalita' globale)
        const int dg[4] = {0, 6, 3, 4}, rp[4] = {3, 1, 2, 2};
        s.numRows = 4;
        for (int i = 0; i < 4; ++i) { s.rows[i] = SongRow(); s.rows[i].degree = dg[i]; s.rows[i].reps = rp[i]; }
    }
}

} // namespace ago
