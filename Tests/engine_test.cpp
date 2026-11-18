// Test del motore AGO (senza JUCE): g++ -std=c++17 -O2 -I../Source engine_test.cpp ../Source/Engine/Engine.cpp
#include "Engine/Engine.h"
#include <cstdio>
#include <map>
#include <string>

using namespace ago;
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf("FALLITO: " __VA_ARGS__); std::printf("  [%s:%d]\n", __FILE__, __LINE__); } } while (0)

static std::string std_name(int n) { static const char* s[12] = {"C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B"}; return std::string(s[n % 12]) + std::to_string(n / 12 - 1); }

struct Counter {
    std::map<int, int> open;   // (porta<<16 | ch<<8 | nota) -> on attivi
    int ons = 0, offs = 0;
    void feed(const std::vector<MidiEv>& ev) {
        for (auto& e : ev) {
            int st = e.b0 & 0xF0, ch = e.b0 & 15;
            int key = (e.portMask << 16) | (ch << 8) | e.b1;
            if (st == 0x90 && e.b2 > 0) { ++ons; open[key]++; }
            else if (st == 0x80 || (st == 0x90 && e.b2 == 0)) { ++offs; if (open[key] > 0) open[key]--; }
        }
    }
    int stuck() const { int s = 0; for (auto& kv : open) s += kv.second; return s; }
};

int main() {
    // 1. accordi in C maggiore
    {
        const char* exp[7] = {"I", "ii", "iii", "IV", "V", "vi", "vii\xc2\xb0"};
        for (int d = 0; d < 7; ++d) {
            Chord c = buildChord(0, 0, d, CT_TRIAD);
            CHECK(romanFor(c) == exp[d], "grado %d: %s invece di %s\n", d, romanFor(c).c_str(), exp[d]);
        }
        Chord f7 = buildChord(0, 0, 3, CT_7TH);
        CHECK(f7.pcs.size() == 4 && f7.pcs[0] == 5 && f7.pcs[1] == 9 && f7.pcs[2] == 0 && f7.pcs[3] == 4, "IV7 in C deve essere F A C E\n");
        Chord a1 = buildChord(0, 0, 4, CT_ALT1);  // V in alt1 = 7th
        CHECK(a1.type == CT_7TH, "alt1 sul V deve dare la settima\n");
        Chord am = buildChord(3, 5, 0, CT_TRIAD); // A minore (circolo idx 3 = A)
        CHECK(am.rootPc == 9 && chordQuality(am) == 1, "A minor: radice A, minore\n");
    }

    // 2. tabella del manuale: pattern di accordo
    {
        Engine e;
        Chord c = buildChord(0, 0, 0, CT_TRIAD);
        int pat[8] = {2, 1, 2, 4, 5, 6, 3, 3};
        const char* expC[8] = {"E1","C1","E1","C2","E2","G2","G1","G1"};
        for (int i = 0; i < 8; ++i) {
            std::string n = std_name(e.noteForValue(pat[i], PT_CHORD, 0, c));
            CHECK(n == expC[i], "C maj step %d: %s invece di %s\n", i, n.c_str(), expC[i]);
        }
        Chord f7 = buildChord(0, 0, 3, CT_7TH);
        const char* expF[8] = {"A1","F1","A1","E1","F2","A2","C1","C1"};
        for (int i = 0; i < 8; ++i) {
            std::string n = std_name(e.noteForValue(pat[i], PT_CHORD, 0, f7));
            CHECK(n == expF[i], "F7 step %d: %s invece di %s\n", i, n.c_str(), expF[i]);
        }
    }

    // 3. variazioni e Euclide
    {
        uint32_t r = 1;
        int pp[10] = {0,1,2,3,2,1,0,1,2,3};
        for (int i = 0; i < 10; ++i) CHECK(Engine::patternIndexFor(i, 4, 2, r) == pp[i], "ping-pong %d\n", i);
        int ppr[8] = {0,1,2,3,3,2,1,0};
        for (int i = 0; i < 8; ++i) CHECK(Engine::patternIndexFor(i, 4, 3, r) == ppr[i], "ping-pong rip %d\n", i);
        int oe[5] = {0,2,4,1,3};
        for (int i = 0; i < 5; ++i) CHECK(Engine::patternIndexFor(i, 5, 4, r) == oe[i], "odd-even %d\n", i);
        int hits = 0; for (int i = 0; i < 8; ++i) hits += Engine::euclidHit(i, 8, 5, 0);
        CHECK(hits == 5, "euclide 5/8 colpi %d\n", hits);
    }

    // 4. pad: posizione 48, range 3 in F (IV) -> C3 F3 A3 (MIDI 48 53 57)
    {
        Engine e;
        Params p; p.padPos = 48; p.padRange = 3; p.padSpread = 0;
        auto n = e.computePadNotes(p, buildChord(0, 0, 3, CT_TRIAD), 48, 1);
        CHECK(n.size() == 3 && n[0] == 48 && n[1] == 53 && n[2] == 57, "pad F pos48 range3\n");
        p.padRange = 12; p.padSpread = 4;
        auto s = e.computePadNotes(p, buildChord(0, 0, 0, CT_TRIAD), 24, 1);
        for (int x : s) CHECK(x >= 48 ? (x % 12 == 0 || x % 12 == 7) : x % 12 == 0, "spread 5: nota %d fuori voicing\n", x);
    }

    // 5. motif: ritmo 1 (tutti on), 1/8, 2 battute -> 16 note, nessuna nota appesa
    {
        Engine e;
        e.base.m[0].on = 1; e.base.m[0].rhy = 0; e.base.m[0].div = 3; e.base.m[0].plen = 4; e.base.m[0].rlen = 8;
        std::vector<MidiEv> out; Counter c;
        e.start(0, out); c.feed(out); out.clear();
        for (int64_t t = 0; t < 2 * 384; ++t) { e.tick(t, out); c.feed(out); out.clear(); }
        CHECK(c.ons == 16, "motif 1/8 su 2 battute: %d note invece di 16\n", c.ons);
        e.stop(out); c.feed(out);
        CHECK(c.stuck() == 0, "note appese motif: %d\n", c.stuck());
    }

    // 6. tutto acceso + modulazioni + sequencer di accordi + strum: 64 battute senza note appese
    {
        Engine e;
        Params& p = e.base;
        p.padOn = p.droneOn = 1; p.m[0].on = p.m[1].on = 1;
        p.padRange = 9; p.padSpread = 1; p.padStrum = 4; p.polyChain = 3; p.invert = 1;
        p.droneTrig = 9 + 4; p.droneType = 3;
        p.m[0].patt = kPatternNZ; p.m[1].rhy = 19; p.m[1].div = 5; p.m[1].gate = 127; p.offset = 2;
        p.m[0].accent = 4; p.m[1].var = 5;
        p.seqArm = 1;
        e.cfg.mods[0] = {SRC_LFO1, D_PAD_POS, 40, 1, 0};
        e.cfg.mods[1] = {SRC_MOD4, D_DEGREE, 100, 1, 0};
        e.cfg.mods[2] = {SRC_LFO2, D_M1_POS, 50, 1, 0};
        e.cfg.mods[3] = {SRC_LFO3, D_CC1 + 73, 100, 5, 0};
        e.cfg.mods[4] = {SRC_MOD5, D_M2_ON, 100, 1, 0};
        e.cfg.mods[5] = {SRC_LFO1, D_M1_STEP1 + 2, 80, 1, 0};
        e.cfg.lfo[1].shape = SH_PATT; e.cfg.lfo[1].patt = 3;
        e.cfg.mods[6] = {SRC_LFO2, D_PN_X, 100, 1, 0};
        std::vector<MidiEv> out; Counter c;
        std::map<int, int> perChan;
        e.start(0, out); c.feed(out); out.clear();
        int chordChanges = 0; Chord last = e.currentChord();
        for (int64_t t = 0; t < 64 * 384; ++t) {
            e.tick(t, out);
            for (auto& ev : out) if ((ev.b0 & 0xF0) == 0x90) perChan[(ev.b0 & 15) + 1]++;
            c.feed(out); out.clear();
            if (e.currentChord() != last) { ++chordChanges; last = e.currentChord(); }
        }
        e.stop(out); c.feed(out); out.clear();
        CHECK(c.stuck() == 0, "note appese con tutto acceso: %d\n", c.stuck());
        CHECK(c.ons > 500, "troppe poche note: %d\n", c.ons);
        CHECK(perChan[1] > 0 && perChan[2] > 0 && perChan[3] > 0 && perChan[4] > 0, "polychain/drone/motif sui canali 1-4\n");
        CHECK(chordChanges > 4, "il sequencer/modulazione doveva cambiare accordo (%d)\n", chordChanges);
        std::printf("scenario completo: %d note, %d cambi d'accordo, canali 1-4: %d %d %d %d\n",
                    c.ons, chordChanges, perChan[1], perChan[2], perChan[3], perChan[4]);
    }

    // 7. sequencer di accordi: I (8 ottavi) vi IV V ...
    {
        Engine e;
        e.base.seqArm = 1;
        e.cfg.seqs[0].mode = 0;
        std::vector<MidiEv> out;
        e.start(0, out);
        int degAt[4];
        for (int64_t t = 0; t < 4 * 384; ++t) { e.tick(t, out); out.clear(); if (t % 384 == 10) degAt[t / 384] = e.currentChord().degree; }
        CHECK(degAt[0] == 0 && degAt[1] == 5 && degAt[2] == 3 && degAt[3] == 4, "seq accordi: %d %d %d %d\n", degAt[0], degAt[1], degAt[2], degAt[3]);
    }

    // 7b. SONG a righe: in F maggiore I x3, VII x1, IV x2, V x2, poi ricomincia
    {
        Engine e;
        e.base.seqArm = 1;
        e.base.key = 11;                          // F (circolo delle quinte)
        std::vector<MidiEv> out;
        e.start(0, out);
        const int expDeg[10] = {0, 0, 0, 6, 3, 3, 4, 4, 0, 0};
        const int expRoot[10] = {5, 5, 5, 4, 10, 10, 0, 0, 5, 5};   // F F F E Bb Bb C C F F
        for (int64_t t = 0; t < 10 * 384; ++t) {
            e.tick(t, out); out.clear();
            if (t % 384 == 20) {
                int b = (int) (t / 384);
                Chord c = e.currentChord();
                CHECK(c.degree == expDeg[b] && c.rootPc == expRoot[b], "song battuta %d: grado %d radice %d", b, c.degree, c.rootPc);
            }
        }
        // riga con tonalita' propria: E minore (circolo 4, modo 5), grado I
        e.cfg.seqs[0].rows[1].key = 4; e.cfg.seqs[0].rows[1].mode = 5; e.cfg.seqs[0].rows[1].degree = 0;
        e.start(0, out); out.clear();
        for (int64_t t = 0; t < 4 * 384; ++t) { e.tick(t, out); out.clear(); }
        Chord c = e.currentChord();
        CHECK(c.rootPc == 4 && chordQuality(c) == 1, "riga con tonalita' propria: atteso E minore");
    }

    // 8. ingresso MIDI di controllo
    {
        Engine e;
        std::vector<MidiEv> out; std::vector<ParamAction> act;
        uint8_t m1[3] = {0x9E, 62, 100};          // canale 15, D -> grado II
        e.midiIn(m1, 3, out, act);
        CHECK(act.size() == 1 && act[0].dest == D_DEGREE && act[0].value == 1, "D sul canale di controllo -> II\n");
        act.clear();
        uint8_t m2[3] = {0x9E, 68, 100};          // G#3 -> 7th
        e.midiIn(m2, 3, out, act);
        CHECK(act.size() == 1 && act[0].dest == D_CTYPE && act[0].value == CT_7TH, "G#3 -> 7th\n");
        act.clear();
        // trasposizione: canale 14, E (white idx 2) in D dorico -> F
        e.base.key = 2; e.base.mode = 1;
        e.tick(0, out); out.clear();
        uint8_t m3[3] = {0x9D, 64, 90};
        e.midiIn(m3, 3, out, act);
        CHECK(out.size() == 1 && (out[0].b1 % 12) == 5, "trasposizione E->F in D dorico (%d)\n", out.empty() ? -1 : out[0].b1);
    }

    std::printf(fails ? "\n%d test FALLITI\n" : "\nTutti i test superati\n", fails);
    return fails ? 1 : 0;
}
