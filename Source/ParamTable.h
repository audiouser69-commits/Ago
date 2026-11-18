// Tabella dei parametri automatizzabili: id, nome, range, default, etichette e campo di ago::Params
#pragma once
#include <JuceHeader.h>
#include "Engine/Model.h"
#include <functional>

namespace agoparams {

struct PDef {
    const char* id;
    const char* name;
    int lo, hi, def;
    int kind;                              // 0 intero, 1 scelta, 2 booleano
    std::function<int&(ago::Params&)> ref;
    juce::StringArray labels;              // per le scelte
    std::function<juce::String(int)> text; // per gli interi
    int dest = ago::D_NONE;                // destinazione equivalente (per le azioni MIDI)
};

inline juce::StringArray numbered(int from, int to, const char* extra = nullptr) {
    juce::StringArray a; for (int i = from; i <= to; ++i) a.add(juce::String(i)); if (extra) a.add(extra); return a;
}

inline const std::vector<PDef>& table() {
    using namespace ago;
    static std::vector<PDef> t;
    if (!t.empty()) return t;

    juce::StringArray keys, modes, ctypes;
    for (int i = 0; i < 12; ++i) keys.add(keyNameCircle(i));
    for (int i = 0; i < kNumModes; ++i) modes.add(modeName(i));
    for (int i = 0; i < kNumChordTypes; ++i) ctypes.add(chordTypeName(i));
    const juce::StringArray degrees {"I","II","III","IV","V","VI","VII"};
    const juce::StringArray strum {"Off","1/32","1/16","1/8T","3+1/8T","1/8","3+1/8"};
    const juce::StringArray spread {"1 Thirds","2 Open","3 Bass+Close","4 Random 70%","5 Root+5th","6 3rd/7th"};
    const juce::StringArray quant {"1/4","1/8","Off"};
    const juce::StringArray dtype {"Root","Root+Oct","Root+5th","Root+5th+Oct"};
    const juce::StringArray dtrig {"Key: hold","Key: 1","Key: 1&2","Key: 1&3","Key: 2&4","Key: 1-4","Key: 1 of 3","Key: 1 of 5",
        "Chord: hold","Chord: 1","Chord: 1&2","Chord: 1&3","Chord: 2&4","Chord: 1-4","Chord: 1 of 3","Chord: 1 of 5",
        "Chord: D1 U2 U3 U4","Chord: U2 U3 D4","Chord: D1 U2 D3 D4"};
    const juce::StringArray var {"Forward","Backward","Ping-Pong","Ping-Pong Rep","Odd-Even","Random"};
    const juce::StringArray div {"1/1","1/2","1/4","1/8","1/4T","1/8T","1/16"};
    const juce::StringArray acc {"Rhythm","Humanized","Motif Vel","o.......","o...o...","o..o..o.","o.o.o.o.","oo.o.oo.","o.oo.o.o",".o.o.o.o"};
    const juce::StringArray offs {"0","1/16","1/8","3/16"};
    auto noteTxt = [](int v) { return juce::String(noteName(v)); };
    auto pct = [](int v) { return juce::String(v) + "%"; };
    auto num = [](int v) { return juce::String(v); };

    auto I = [&](const char* id, const char* nm, int lo, int hi, int def, std::function<int&(Params&)> r, std::function<juce::String(int)> tx, int dest) {
        PDef d; d.id = id; d.name = nm; d.lo = lo; d.hi = hi; d.def = def; d.kind = 0; d.ref = r; d.text = tx; d.dest = dest; t.push_back(d);
    };
    auto C = [&](const char* id, const char* nm, const juce::StringArray& l, int def, std::function<int&(Params&)> r, int dest) {
        PDef d; d.id = id; d.name = nm; d.lo = 0; d.hi = l.size() - 1; d.def = def; d.kind = 1; d.ref = r; d.labels = l; d.dest = dest; t.push_back(d);
    };
    auto B = [&](const char* id, const char* nm, int def, std::function<int&(Params&)> r, int dest) {
        PDef d; d.id = id; d.name = nm; d.lo = 0; d.hi = 1; d.def = def; d.kind = 2; d.ref = r; d.dest = dest; t.push_back(d);
    };

    C("key", "Key", keys, 0, [](Params& p) -> int& { return p.key; }, D_KEY);
    C("mode", "Mode", modes, 0, [](Params& p) -> int& { return p.mode; }, D_MODE);
    C("degree", "Chord Degree", degrees, 0, [](Params& p) -> int& { return p.degree; }, D_DEGREE);
    C("ctype", "Chord Type", ctypes, 0, [](Params& p) -> int& { return p.ctype; }, D_CTYPE);
    B("invert", "Chord Invert", 0, [](Params& p) -> int& { return p.invert; }, D_NONE);
    B("seqArm", "Chord Seq Arm", 0, [](Params& p) -> int& { return p.seqArm; }, D_NONE);
    B("modAll", "Mod All", 1, [](Params& p) -> int& { return p.modAll; }, D_NONE);
    B("modPad", "Mod Pad", 1, [](Params& p) -> int& { return p.modPart[0]; }, D_NONE);
    B("modDrone", "Mod Drone", 1, [](Params& p) -> int& { return p.modPart[1]; }, D_NONE);
    B("modM1", "Mod Motif 1", 1, [](Params& p) -> int& { return p.modPart[2]; }, D_NONE);
    B("modM2", "Mod Motif 2", 1, [](Params& p) -> int& { return p.modPart[3]; }, D_NONE);

    B("padOn", "Pad On", 0, [](Params& p) -> int& { return p.padOn; }, D_PAD_ON);
    I("padPos", "Pad Position", 20, 108, 55, [](Params& p) -> int& { return p.padPos; }, noteTxt, D_PAD_POS);
    I("padRange", "Pad Range", 1, 22, 4, [](Params& p) -> int& { return p.padRange; }, num, D_PAD_RANGE);
    C("padSpread", "Pad Spread", spread, 0, [](Params& p) -> int& { return p.padSpread; }, D_PAD_SPREAD);
    C("padStrum", "Pad Strum", strum, 0, [](Params& p) -> int& { return p.padStrum; }, D_PAD_STRUM);
    I("padVel", "Pad Velocity", 0, 127, 90, [](Params& p) -> int& { return p.padVel; }, num, D_PAD_VEL);
    C("padQuant", "Pad Quantize", quant, 0, [](Params& p) -> int& { return p.padQuant; }, D_NONE);
    I("polyChain", "Poly Chain", 1, 4, 1, [](Params& p) -> int& { return p.polyChain; }, num, D_NONE);

    B("droneOn", "Drone On", 0, [](Params& p) -> int& { return p.droneOn; }, D_DR_ON);
    I("droneOct", "Drone Position", 0, 4, 1, [](Params& p) -> int& { return p.droneOct; }, [](int v) { return "Oct " + juce::String(v); }, D_DR_POS);
    C("droneType", "Drone Notes", dtype, 0, [](Params& p) -> int& { return p.droneType; }, D_DR_TYPE);
    C("droneTrig", "Drone Trigger", dtrig, 0, [](Params& p) -> int& { return p.droneTrig; }, D_DR_TRIG);
    I("droneVel", "Drone Velocity", 0, 127, 90, [](Params& p) -> int& { return p.droneVel; }, num, D_NONE);

    for (int k = 0; k < 2; ++k) {
        static const char* ids[2][11] = {
            {"m1On","m1Pos","m1PLen","m1Var","m1Patt","m1Div","m1RLen","m1Acc","m1Rhy","m1Gate","m1Vel"},
            {"m2On","m2Pos","m2PLen","m2Var","m2Patt","m2Div","m2RLen","m2Acc","m2Rhy","m2Gate","m2Vel"}};
        static const char* nms[2][11] = {
            {"M1 On","M1 Position","M1 Patt Len","M1 Variation","M1 Pattern","M1 Clock Div","M1 Rhy Len","M1 Accent","M1 Rhythm","M1 Gate","M1 Velocity"},
            {"M2 On","M2 Position","M2 Patt Len","M2 Variation","M2 Pattern","M2 Clock Div","M2 Rhy Len","M2 Accent","M2 Rhythm","M2 Gate","M2 Velocity"}};
        const int D = k == 0 ? D_M1_GATE : D_M2_GATE;
        B(ids[k][0], nms[k][0], 0, [k](Params& p) -> int& { return p.m[k].on; }, D + 9);
        I(ids[k][1], nms[k][1], 0, 24, k == 0 ? 6 : 3, [k](Params& p) -> int& { return p.m[k].pos; }, [](int v) { return "+" + juce::String(v); }, D + 1);
        I(ids[k][2], nms[k][2], 1, 16, k == 0 ? 8 : 4, [k](Params& p) -> int& { return p.m[k].plen; }, num, D + 2);
        C(ids[k][3], nms[k][3], var, 0, [k](Params& p) -> int& { return p.m[k].var; }, D + 3);
        C(ids[k][4], nms[k][4], numbered(1, 40, "NZ"), k == 0 ? 0 : 17, [k](Params& p) -> int& { return p.m[k].patt; }, D + 4);
        C(ids[k][5], nms[k][5], div, k == 0 ? 3 : 2, [k](Params& p) -> int& { return p.m[k].div; }, D + 5);
        I(ids[k][6], nms[k][6], 4, 32, 8, [k](Params& p) -> int& { return p.m[k].rlen; }, num, D_NONE);
        C(ids[k][7], nms[k][7], acc, 0, [k](Params& p) -> int& { return p.m[k].accent; }, D + 6);
        C(ids[k][8], nms[k][8], numbered(1, 40), k == 0 ? 1 : 3, [k](Params& p) -> int& { return p.m[k].rhy; }, D + 7);
        I(ids[k][9], nms[k][9], 5, 127, 64, [k](Params& p) -> int& { return p.m[k].gate; }, [](int v) { return v >= 127 ? juce::String("Legato") : juce::String(juce::roundToInt(v * 100.0 / 127)) + "%"; }, D + 0);
        I(ids[k][10], nms[k][10], 0, 127, 100, [k](Params& p) -> int& { return p.m[k].vel; }, num, D + 8);
    }
    C("offset", "Motif Offset", offs, 0, [](Params& p) -> int& { return p.offset; }, D_NONE);
    I("humanize", "Humanize", 0, 10, 2, [](Params& p) -> int& { return p.humanize; }, [](int v) { return juce::String(v * 10) + "%"; }, D_HUMANIZE);
    I("velLow", "Velocity Low", 1, 100, 50, [](Params& p) -> int& { return p.velLow; }, pct, D_VEL_LOW);
    I("px", "Perlin X", 0, 100, 20, [](Params& p) -> int& { return p.px; }, num, D_PN_X);
    I("py", "Perlin Y", 0, 100, 20, [](Params& p) -> int& { return p.py; }, num, D_PN_Y);
    I("zoom", "Perlin Zoom", 0, 100, 30, [](Params& p) -> int& { return p.zoom; }, num, D_PN_ZOOM);
    I("spc", "Perlin Spacing", 0, 100, 20, [](Params& p) -> int& { return p.spc; }, num, D_PN_SPC);
    I("ruff", "Perlin Ruff", 0, 100, 60, [](Params& p) -> int& { return p.ruff; }, num, D_PN_RUFF);
    I("per", "Perlin Persistence", 0, 100, 50, [](Params& p) -> int& { return p.per; }, num, D_PN_PER);
    return t;
}

inline const PDef* byDest(int dest) {
    for (auto& d : table()) if (d.dest == dest && dest != ago::D_NONE) return &d;
    return nullptr;
}

} // namespace agoparams
