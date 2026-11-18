#include "Engine.h"
#include <cmath>
#include <algorithm>

namespace ago {

static const double kTwoPi = 6.283185307179586;

Engine::Engine() {
    loadFactory(cfg);
    for (auto& r : stepOverride) for (int& v : r) v = -1;
    for (int& h : kbHeld) h = 0;
    eff = base; prevEff = base;
    appliedChord = chordFrom(base);
}

// ------------------------------------------------------------------ utilita'
int Engine::stepTicksForDiv(int div) {
    static const int t[7] = {384, 192, 96, 48, 64, 32, 24}; // 1/1 1/2 1/4 1/8 1/4T 1/8T 1/16
    return t[std::clamp(div, 0, 6)];
}

int Engine::patternIndexFor(int64_t c, int L, int var, uint32_t& rng) {
    if (L <= 1) return 0;
    switch (var) {
        case 1: return L - 1 - (int) (c % L);
        case 2: { int P = 2 * L - 2; int x = (int) (c % P); return x < L ? x : P - x; }
        case 3: { int P = 2 * L; int x = (int) (c % P); return x < L ? x : P - 1 - x; }
        case 4: { int x = (int) (c % L); int nEven = (L + 1) / 2; return x < nEven ? 2 * x : 2 * (x - nEven) + 1; }
        case 5: rng = rng * 1664525u + 1013904223u; return (int) ((rng >> 8) % (uint32_t) L);
        default: return (int) (c % L);
    }
}

bool Engine::euclidHit(int i, int len, int hits, int rot) {
    if (len <= 0) return false;
    hits = std::clamp(hits, 0, len);
    if (hits == 0) return false;
    int j = ((i + rot) % len + len) % len;
    return (j * hits) % len < hits;
}

int Engine::rhythmValue(const Rhythm& r, int beat, int len) const {
    if (r.euclid) return euclidHit(beat, len, r.hits, r.rot) ? (beat == 0 ? 120 : 100) : kRest;
    return r.v[std::clamp(beat, 0, 31)];
}

int Engine::noteForValue(int value, int type, int pos, const Chord& c) const {
    int k = std::max(0, value - 1 + pos);
    int n = 0;
    if (type == PT_CHORD) {
        int sz = std::max(1, (int) c.pcs.size());
        n = 24 + c.pcs[k % sz] + 12 * (k / sz);
    } else if (type == PT_SCALE) {
        n = 24 + c.keyPc + scaleSemis(c.mode, k);
    } else {
        n = 24 + c.keyPc + k;
    }
    while (n > 127) n -= 12;
    return std::clamp(n, 0, 127);
}

void Engine::computeNZ(const Params& p, int motif, int out[16]) const {
    const int pool = cfg.nzPool[motif];
    const double zs = 0.04 + p.zoom / 100.0 * 0.9;
    const double x0 = p.px / 100.0 * 24.0;
    const double y0 = p.py / 100.0 * 24.0 + motif * (p.spc / 100.0 * 3.0) + 0.37;
    const double pers = 0.15 + p.per / 100.0 * 0.75;
    const int span = pool == PT_CHORD ? 10 : pool == PT_SCALE ? 16 : 24;
    const double amt = p.ruff / 100.0 * 1.6;
    for (int i = 0; i < 16; ++i) {
        double n = perlin.fractal(x0 + i * zs, y0, pers);
        int v = (int) std::lround(1 + span * 0.5 + n * amt * span * 0.5);
        out[i] = std::clamp(v, 1, patTypeMax(pool));
    }
}

Chord Engine::chordFrom(const Params& p) const {
    return buildChord(p.key, p.mode, p.degree, p.ctype);
}

int Engine::padChanFor(int i) const {
    int chain = std::clamp(eff.polyChain, 1, 4);
    int ch = cfg.padChan + (i % chain);
    return ((ch - 1) % 16) + 1;
}

int Engine::quantTicks() const {
    switch (eff.padQuant) { case 0: return 96; case 1: return 48; default: return 1; }
}

// ------------------------------------------------------------------ voicing del Pad
std::vector<int> Engine::computePadNotes(const Params& p, const Chord& c, int posOverride, uint32_t seed) const {
    std::vector<int> res;
    const int range = std::clamp(p.padRange, 1, 22);
    int spread = range <= 4 ? 0 : std::clamp(p.padSpread, 0, 5);
    const int pos = posOverride;
    const int third = c.pcs.size() > 1 ? c.pcs[1] : -1;
    const int fifth = c.pcs.size() > 2 ? c.pcs[2] : -1;
    const int seventh = c.pcs.size() > 3 ? c.pcs[3] : -1;
    int openToggle = 0;
    uint32_t s = seed * 2654435761u + 1u;
    for (int n = std::max(0, pos); n <= 127 && (int) res.size() < range; ++n) {
        int pc = n % 12;
        if (!pcIn(c.pcs, pc)) continue;
        bool isRoot = pc == c.rootPc;
        bool take = true;
        switch (spread) {
            case 1:
                if (n < 48) take = isRoot;
                else if (n < 60) { take = (openToggle++ % 2) == 0; }
                else if (n < 72) take = true;
                else take = isRoot;
                break;
            case 2: take = n < 48 ? isRoot : true; break;
            case 3: s = s * 1664525u + 1013904223u; take = ((s >> 9) % 100) < 70; break;
            case 4: take = n < 48 ? isRoot : (isRoot || pc == fifth); break;
            case 5: take = n < 48 ? isRoot : (pc == third || pc == seventh); break;
            default: break;
        }
        if (take) res.push_back(n);
    }
    return res;
}

std::vector<int> Engine::computeDroneNotes(const Params& p, const Chord& c) const {
    std::vector<int> r;
    const bool chordMode = p.droneTrig >= 8;
    const int rootPc = chordMode ? c.rootPc : c.keyPc;
    int fifth = 7;
    if (chordMode) { if (c.semis.size() > 2) fifth = c.semis[2]; }
    else { int f = scaleSemis(c.mode, 4); if (f > 0 && f < 12) fifth = f; }
    const int root = 24 + 12 * std::clamp(p.droneOct, 0, 4) + rootPc;
    r.push_back(root);
    switch (p.droneType) {
        case 1: r.push_back(root + 12); break;
        case 2: r.push_back(root + fifth); break;
        case 3: r.push_back(root + fifth); r.push_back(root + 12); break;
        default: break;
    }
    for (int& n : r) n = std::clamp(n, 0, 127);
    return r;
}

// ------------------------------------------------------------------ modulazione
void Engine::updateLfos() {
    const double beats = curTick / (double) kTicksPerBeat;
    struct Def { bool synced; double rate; int shape; int prob; int patt; };
    Def d[8];
    for (int i = 0; i < 3; ++i) {
        const auto& l = cfg.lfo[i];
        d[i] = { l.synced != 0, lfoRateValue(l.synced != 0, l.rateIdx), l.shape, i == 2 ? 100 : l.prob, l.patt };
    }
    d[3] = {false, 40.0, SH_TRI, 50, 0};
    d[4] = {true, 48.0, SH_SINE, 100, 0};
    d[5] = {true, 32.0, SH_RAMP, 90, 0};
    d[6] = {true, 8.0, SH_RANDOM, 75, 0};
    d[7] = {true, 8.0, SH_SQUARE, 15, 0};
    for (int i = 0; i < 8; ++i) {
        double pos = d[i].synced ? beats / d[i].rate : seconds / d[i].rate;
        int64_t cyc = (int64_t) std::floor(pos);
        double ph = pos - (double) cyc;
        if (cyc != lfoCycle[i]) {
            lfoCycle[i] = cyc;
            uint32_t r = (uint32_t) rng();
            lfoActive[i] = (int) (r % 100) < d[i].prob;
            lfoRand[i] = (rng() % 10000) / 10000.0;
        }
        lfoPhase[i] = ph;
        double v = 0;
        switch (d[i].shape) {
            case SH_SINE: v = std::sin(kTwoPi * ph); break;
            case SH_TRI: v = 1.0 - std::fabs(2.0 * ph - 1.0); break;
            case SH_RAMP: v = ph; break;
            case SH_SAW: v = 1.0 - ph; break;
            case SH_SQUARE: v = ph < 0.5 ? 1.0 : 0.0; break;
            case SH_PULSE: v = ph < 0.2 ? 1.0 : 0.0; break;
            case SH_RANDOM: v = lfoRand[i]; break;
            case SH_PATT: v = ph; break;
            default: break;
        }
        lfoVal[i] = v;
    }
}

double Engine::sourceValue(int src, bool& bipolar, bool& isPatt, int& pattVal) {
    bipolar = false; isPatt = false; pattVal = 0;
    if (src >= SRC_LFO1 && src <= SRC_MOD5) {
        int i = src - SRC_LFO1;
        if (!lfoActive[i]) return std::nan("");
        int shape = i < 3 ? cfg.lfo[i].shape : (i == 3 ? SH_TRI : i == 4 ? SH_SINE : i == 5 ? SH_RAMP : i == 6 ? SH_RANDOM : SH_SQUARE);
        if (shape == SH_SINE) bipolar = true;
        if (i < 3 && shape == SH_PATT) {
            isPatt = true;
            const Pattern& pt = cfg.patterns[std::clamp(cfg.lfo[i].patt, 0, kNumPatterns - 1)];
            pattVal = pt.v[std::clamp((int) (lfoPhase[i] * 16.0), 0, 15)];
        }
        return lfoVal[i];
    }
    switch (src) {
        case SRC_VEL: return midiVel / 127.0;
        case SRC_PB:  return midiPB / 16383.0;
        case SRC_MW:  return midiMW / 127.0;
        case SRC_AT:  return midiAT / 127.0;
        default: return std::nan("");
    }
}

void Engine::computeEffective() {
    eff = base;

    // il sequencer di accordi "preme i tasti" al posto dell'utente
    if (seqRunning && seqStepIdx >= 0) {
        eff.key = seqKey; eff.mode = seqMode; eff.degree = seqDeg; eff.ctype = seqType;
    }

    for (auto& r : stepOverride) for (int& v : r) v = -1;
    if (!base.modAll) return;

    for (int s = 0; s < 8; ++s) {
        const ModSlot& m = cfg.mods[s];
        if (m.src == SRC_OFF || m.dst == D_NONE) continue;
        DestInfo di = destInfo(m.dst);
        if (di.part >= 0 && !base.modPart[di.part]) continue;
        bool bip, patt; int pv;
        double u = sourceValue(m.src, bip, patt, pv);
        if (std::isnan(u)) continue;
        if (!patt && m.amount <= 0) continue;
        const double amt = m.amount / 100.0;
        const double span = di.hi - di.lo;

        if (m.dst >= D_CC1) continue;   // le CC vengono inviate in tick()

        if (m.dst >= D_M1_STEP1) {
            int k = m.dst >= D_M2_STEP1 ? 1 : 0;
            int st = m.dst - (k ? D_M2_STEP1 : D_M1_STEP1);
            int pidx = eff.m[k].patt;
            int baseVal = cfg.man[k].on ? cfg.man[k].val[st] : pidx >= kPatternNZ ? 1 : cfg.patterns[pidx].v[st];
            int v = patt ? pv : (int) std::lround(baseVal + (bip ? u : u) * amt * (bip ? span * 0.5 : span));
            stepOverride[k][st] = std::clamp(v, di.lo, di.hi);
            continue;
        }

        int* ptr = destPtr(eff, m.dst);
        if (!ptr) continue;
        int v;
        if (patt) v = pv - di.disp;
        else v = (int) std::lround(*ptr + u * amt * (bip ? span * 0.5 : span));
        *ptr = std::clamp(v, di.lo, di.hi);
    }
}

void Engine::sendCC(int ch, int mask, int cc, int val, std::vector<MidiEv>& out) {
    if (silent) return;
    out.push_back({mask, (uint8_t) (0xB0 | ((ch - 1) & 15)), (uint8_t) (cc & 127), (uint8_t) std::clamp(val, 0, 127)});
}

// ------------------------------------------------------------------ note
void Engine::emitOn(int part, int note, int ch, int mask, int vel, std::vector<MidiEv>& out) {
    if (silent || vel <= 0) return;
    out.push_back({mask, (uint8_t) (0x90 | ((ch - 1) & 15)), (uint8_t) note, (uint8_t) std::clamp(vel, 1, 127), part});
}

void Engine::emitOff(int part, int note, int ch, int mask, std::vector<MidiEv>& out) {
    if (silent) return;
    out.push_back({mask, (uint8_t) (0x80 | ((ch - 1) & 15)), (uint8_t) note, 64, part});
}

void Engine::allOff(std::vector<Sound>& snd, int part, std::vector<MidiEv>& out) {
    for (auto& s : snd) emitOff(part, s.note, s.ch, s.mask, out);
    snd.clear();
}

void Engine::dropPendingOns(int part) {
    pending.erase(std::remove_if(pending.begin(), pending.end(),
                  [part](const Pending& p) { return p.part == part && p.on; }), pending.end());
}

void Engine::processPending(int64_t t, std::vector<MidiEv>& out) {
    // prima i note-off, poi i note-on (legato senza note "incollate")
    for (int pass = 0; pass < 2; ++pass) {
        for (size_t i = 0; i < pending.size();) {
            Pending& p = pending[i];
            if (p.t <= t && p.on == (pass == 1)) {
                if (p.on) {
                    emitOn(p.part, p.note, p.ch, p.mask, p.vel, out);
                    if (p.part == P_PAD) padSnd.push_back({p.note, p.ch, p.mask});
                } else {
                    emitOff(p.part, p.note, p.ch, p.mask, out);
                    if (p.part == P_M1 || p.part == P_M2) {
                        auto& m = mrt[p.part - P_M1];
                        if (m.hasNote && m.snd.note == p.note && m.snd.ch == p.ch) m.hasNote = false;
                    }
                }
                pending.erase(pending.begin() + (long) i);
            } else ++i;
        }
    }
}

// ------------------------------------------------------------------ Pad
void Engine::updatePad(int64_t t, bool chordChangeNow, std::vector<MidiEv>& out) {
    const bool want = eff.padOn != 0;
    if (!want) {
        if (padRunning || padArmed) { dropPendingOns(P_PAD); allOff(padSnd, P_PAD, out); }
        padRunning = padArmed = false;
        return;
    }
    if (!padRunning) {
        if (!padArmed) padArmed = true;
        if (t % quantTicks() != 0) return;
        padRunning = true; padArmed = false;
        chordChangeNow = true;
        for (int& k : lastPadKey) k = -1;
    }

    int key[6] = { eff.padPos, eff.padRange, eff.padSpread, eff.polyChain, (int) chordSerial, padInvertShift };
    bool paramChange = false;
    for (int i = 0; i < 6; ++i) if (key[i] != lastPadKey[i]) paramChange = true;
    if (!paramChange && !chordChangeNow) return;

    const Chord& c = appliedChord;
    int shift = padInvertShift;
    if (chordChangeNow && eff.invert && !padSnd.empty()) {
        double prevMean = 0; for (auto& s : padSnd) prevMean += s.note; prevMean /= (double) padSnd.size();
        double best = 1e9; int bestShift = 0;
        for (int sh = -7; sh <= 7; ++sh) {
            auto cand = computePadNotes(eff, c, eff.padPos + sh, chordSerial);
            if (cand.empty()) continue;
            double m = 0; for (int n : cand) m += n; m /= (double) cand.size();
            double d = std::fabs(m - prevMean) + std::abs(sh) * 0.05;
            if (d < best) { best = d; bestShift = sh; }
        }
        shift = bestShift;
    } else if (!eff.invert) shift = 0;
    padInvertShift = shift;
    key[5] = shift;

    std::vector<int> target = computePadNotes(eff, c, eff.padPos + shift, chordSerial);
    const int mask = portMask(cfg.padPort);

    dropPendingOns(P_PAD);
    // spegne quelle non piu' nel voicing (o su canale diverso)
    std::vector<Sound> keep;
    for (auto& s : padSnd) {
        auto it = std::find(target.begin(), target.end(), s.note);
        int idx = it == target.end() ? -1 : (int) (it - target.begin());
        if (idx >= 0 && padChanFor(idx) == s.ch && s.mask == mask) keep.push_back(s);
        else emitOff(P_PAD, s.note, s.ch, s.mask, out);
    }
    padSnd = keep;

    // accende le nuove con lo strum
    static const int strumT[7] = {0, 12, 24, 32, 32, 48, 48};
    const int st = std::clamp(eff.padStrum, 0, 6);
    int order = 0;
    for (int i = 0; i < (int) target.size(); ++i) {
        int n = target[i];
        bool already = false;
        for (auto& s : padSnd) if (s.note == n) already = true;
        if (already) continue;
        int delay = 0;
        if (st == 4 || st == 6) delay = order < 3 ? 0 : (order - 2) * strumT[st];
        else delay = order * strumT[st];
        ++order;
        if (delay == 0) {
            emitOn(P_PAD, n, padChanFor(i), mask, eff.padVel, out);
            padSnd.push_back({n, padChanFor(i), mask});
        } else {
            pending.push_back({t + delay, P_PAD, true, n, padChanFor(i), mask, eff.padVel});
        }
    }
    for (int i = 0; i < 6; ++i) lastPadKey[i] = key[i];
}

// ------------------------------------------------------------------ Drone
static bool droneCadenceHit(int cad, int64_t t) {
    // cadenze: battute di 4, 3 o 5 quarti; posizioni in ottavi
    static const int bar[11] = {4, 4, 4, 4, 4, 4, 3, 5, 4, 4, 4};
    static const int mask[11] = {0, 0x01, 0x05, 0x11, 0x44, 0x55, 0x01, 0x01, 0xA9, 0x68, 0x59};
    cad = std::clamp(cad, 0, 10);
    if (cad == 0 || t % 48 != 0) return false;
    int64_t barT = (int64_t) bar[cad] * kTicksPerBeat;
    int e = (int) ((t % barT) / 48);
    return e < 8 && ((mask[cad] >> e) & 1);
}

void Engine::updateDrone(int64_t t, bool chordChangeNow, std::vector<MidiEv>& out) {
    const bool want = eff.droneOn != 0;
    if (!want) {
        if (droneRunning || droneArmed) allOff(droneSnd, P_DRONE, out);
        droneRunning = droneArmed = false;
        return;
    }
    bool retrig = false;
    if (!droneRunning) {
        droneArmed = true;
        if (t % quantTicks() != 0) return;
        droneRunning = true; droneArmed = false;
        retrig = true;
    }
    const bool chordMode = eff.droneTrig >= 8;
    const int cad = chordMode ? eff.droneTrig - 8 : eff.droneTrig;
    if (droneCadenceHit(cad, t)) retrig = true;

    const Chord& c = appliedChord;
    auto target = computeDroneNotes(eff, c);
    int key[3] = { eff.droneOct, eff.droneType, chordMode ? (int) chordSerial : -2 };
    bool change = false;
    for (int i = 0; i < 3; ++i) if (key[i] != lastDroneKey[i]) change = true;
    if (chordMode && chordChangeNow) change = true;
    if (!retrig && !change) return;

    const int mask = portMask(cfg.dronePort);
    allOff(droneSnd, P_DRONE, out);
    for (int n : target) { emitOn(P_DRONE, n, cfg.droneChan, mask, eff.droneVel, out); droneSnd.push_back({n, cfg.droneChan, mask}); }
    for (int i = 0; i < 3; ++i) lastDroneKey[i] = key[i];
}

// ------------------------------------------------------------------ Motif
int Engine::motifVelocity(int k, int beatVal, int64_t t) {
    const MotifP& m = eff.m[k];
    const int high = std::clamp(m.vel, 1, 127);
    const int low = std::max(1, high * std::clamp(eff.velLow, 1, 100) / 100);
    static const char* acc[7] = {"o.......", "o...o...", "o..o..o.", "o.o.o.o.", "oo.o.oo.", "o.oo.o.o", ".o.o.o.o"};
    switch (m.accent) {
        case 0: return beatVal;
        case 1: {
            double h = eff.humanize / 10.0;
            double r = ((rng() % 2001) / 1000.0 - 1.0) * h * 0.5;
            return std::clamp((int) std::lround(beatVal * (1.0 + r)), 1, 127);
        }
        case 2: return high;
        default: {
            int e = (int) ((t / 48) % 8);
            return acc[std::clamp(m.accent - 3, 0, 6)][e] == 'o' ? high : low;
        }
    }
}

void Engine::motifTick(int k, int64_t t, std::vector<MidiEv>& out) {
    MotifRt& r = mrt[k];
    const MotifP& m = eff.m[k];
    const int part = P_M1 + k;
    const int chan = k == 0 ? cfg.m1Chan : cfg.m2Chan;
    const int mask = portMask(k == 0 ? cfg.m1Port : cfg.m2Port);

    if (!m.on) {
        if (r.running || r.armed) {
            if (r.hasNote) { emitOff(part, r.snd.note, r.snd.ch, r.snd.mask, out); r.hasNote = false; }
            pending.erase(std::remove_if(pending.begin(), pending.end(), [part](const Pending& p) { return p.part == part; }), pending.end());
        }
        r.running = r.armed = false; r.lastIdx = r.lastBeat = -1;
        return;
    }
    if (!r.running) {
        r.armed = true;
        if (t % (4 * kTicksPerBeat) != 0) return;   // parte al prossimo downbeat
        r.running = true; r.armed = false;
        r.startTick = t; r.stepCount = 0; r.noteCount = 0;
    }

    const int off = k == 1 ? std::clamp(eff.offset, 0, 3) * 24 : 0;
    const int stepT = stepTicksForDiv(m.div);
    const int64_t rel = t - r.startTick - off;
    if (rel < 0 || rel % stepT != 0) return;

    if (cfg.man[k].on) {
        // ---- sequenza manuale: gli step scritti a mano, sempre sui gradi dell'armonia corrente
        const ManualSeq& ms = cfg.man[k];
        const int len = std::clamp(ms.len, 1, kManSteps);
        const int step = (int) (r.stepCount % len);
        r.stepCount++;
        r.lastBeat = step; r.lastIdx = step;
        const int stt = ms.st[step];
        if (stt != MS_NOTE) return;
        int ties = 0;
        for (int j = 1; j < len; ++j) { if (ms.st[(step + j) % len] == MS_TIE) ++ties; else break; }
        const int full = stepT * (1 + ties);
        const int gate = std::clamp(m.gate, 5, 127);
        const int dur = gate >= 127 ? full : std::max(1, (int) std::lround(full * gate / 127.0));
        const int value = (step < 16 && stepOverride[k][step] >= 0) ? stepOverride[k][step] : std::clamp(ms.val[step], 1, patTypeMax(ms.type));
        const Chord c = chordFrom(eff);
        const int note = noteForValue(value, ms.type, m.pos, c);
        const int vel = motifVelocity(k, std::clamp(ms.vel[step], 1, 127), t);
        if (r.hasNote) {
            int pn = r.snd.note, pc = r.snd.ch;
            pending.erase(std::remove_if(pending.begin(), pending.end(),
                [part, pn, pc](const Pending& p) { return p.part == part && !p.on && p.note == pn && p.ch == pc; }), pending.end());
            emitOff(part, r.snd.note, r.snd.ch, r.snd.mask, out);
            r.hasNote = false;
        }
        emitOn(part, note, chan, mask, vel, out);
        r.hasNote = true; r.snd = {note, chan, mask};
        pending.push_back({t + dur, part, false, note, chan, mask, 0});
        return;
    }

    const int rlen = std::clamp(m.rlen, 4, 32);
    const int beat = (int) (r.stepCount % rlen);
    const Rhythm& rh = cfg.rhythms[std::clamp(m.rhy, 0, kNumRhythms - 1)];
    const int bv = rhythmValue(rh, beat, rlen);
    r.lastBeat = beat;
    r.stepCount++;

    if (bv == kRest || bv == kTie) return;

    // durata: il gate si estende sui TIE successivi
    int ties = 0;
    for (int j = 1; j < rlen; ++j) {
        if (rhythmValue(rh, (beat + j) % rlen, rlen) == kTie) ++ties; else break;
    }
    const int full = stepT * (1 + ties);
    const int gate = std::clamp(m.gate, 5, 127);
    const int dur = gate >= 127 ? full : std::max(1, (int) std::lround(full * gate / 127.0));

    // nota dal pattern
    const int plen = std::clamp(m.plen, 1, 16);
    const int idx = patternIndexFor(r.noteCount, plen, m.var, rngState);
    r.noteCount++;
    int value, type;
    if (m.patt >= kPatternNZ) {
        int nz[16]; computeNZ(eff, k, nz);
        value = nz[idx]; type = cfg.nzPool[k];
    } else {
        const Pattern& p = cfg.patterns[std::clamp(m.patt, 0, kNumPatterns - 1)];
        value = stepOverride[k][idx] >= 0 ? stepOverride[k][idx] : p.v[idx];
        type = p.type;
    }
    const Chord c = chordFrom(eff);
    const int note = noteForValue(value, type, m.pos, c);
    const int vel = motifVelocity(k, bv, t);
    r.lastIdx = idx;

    if (r.hasNote) {
        // la nota precedente ancora accesa: spegnila (gli off in coda per lei si annullano)
        int pn = r.snd.note, pc = r.snd.ch;
        pending.erase(std::remove_if(pending.begin(), pending.end(),
            [part, pn, pc](const Pending& p) { return p.part == part && !p.on && p.note == pn && p.ch == pc; }), pending.end());
        emitOff(part, r.snd.note, r.snd.ch, r.snd.mask, out);
        r.hasNote = false;
    }
    emitOn(part, note, chan, mask, vel, out);
    r.hasNote = true; r.snd = {note, chan, mask};
    pending.push_back({t + dur, part, false, note, chan, mask, 0});
}

// ------------------------------------------------------------------ trasporto
void Engine::start(int64_t t, std::vector<MidiEv>& out) {
    stop(out);
    running = true;
    const int64_t barT = 4 * kTicksPerBeat;
    const int64_t s = (t / barT) * barT;
    // fast-forward silenzioso dall'inizio della battuta, cosi' i loop dell'host restano coerenti
    std::vector<MidiEv> dummy;
    silent = true;
    for (int64_t x = s; x < t; ++x) tick(x, dummy);
    silent = false;
    // gli on rimasti dal fast-forward vanno riaccesi davvero: si ripartira' alla prossima occasione
    padRunning = false; droneRunning = false;
    padSnd.clear(); droneSnd.clear();
    for (auto& m : mrt) m.hasNote = false;
    pending.clear();
}

void Engine::stop(std::vector<MidiEv>& out) {
    dropPendingOns(P_PAD);
    allOff(padSnd, P_PAD, out);
    allOff(droneSnd, P_DRONE, out);
    for (int k = 0; k < 2; ++k) {
        auto& m = mrt[k];
        if (m.hasNote) emitOff(P_M1 + k, m.snd.note, m.snd.ch, m.snd.mask, out);
        m = MotifRt();
    }
    for (auto& p : pending) if (!p.on) emitOff(p.part, p.note, p.ch, p.mask, out);
    pending.clear();
    padRunning = padArmed = droneRunning = droneArmed = false;
    seqRunning = false; seqStepIdx = -1; prevSeqArm = false;
    running = false;
}

void Engine::panic(std::vector<MidiEv>& out) {
    std::vector<MidiEv> tmp;
    bool wasSilent = silent; silent = false;
    allOff(padSnd, P_PAD, tmp);
    allOff(droneSnd, P_DRONE, tmp);
    for (int k = 0; k < 2; ++k) { auto& m = mrt[k]; if (m.hasNote) emitOff(P_M1 + k, m.snd.note, m.snd.ch, m.snd.mask, tmp); m.hasNote = false; }
    for (auto& p : pending) if (!p.on) emitOff(p.part, p.note, p.ch, p.mask, tmp);
    pending.clear();
    for (int ch = 0; ch < 16; ++ch) {
        tmp.push_back({7, (uint8_t) (0xB0 | ch), 123, 0, 9});
        tmp.push_back({7, (uint8_t) (0xB0 | ch), 120, 0, 9});
    }
    silent = wasSilent;
    out.insert(out.end(), tmp.begin(), tmp.end());
    // forza la riaccensione delle parti al prossimo tick
    padRunning = droneRunning = false;
}

void Engine::tick(int64_t t, std::vector<MidiEv>& out) {
    curTick = t;
    seconds += 60.0 / (tempo * kTicksPerBeat);
    updateLfos();

    // --- sequencer di accordi
    {
        const bool arm = base.seqArm != 0;
        if (arm && !prevSeqArm) { seqRunning = false; }
        if (!arm) { seqRunning = false; seqStepIdx = -1; }
        if (arm && !seqRunning && t % (4 * kTicksPerBeat) == 0) { seqRunning = true; seqStart = t; }
        prevSeqArm = arm;
        if (seqRunning) {
            const ChordSeq& sq = cfg.seqs[std::clamp(cfg.activeSeq, 0, 4)];
            if (sq.mode == 1) {
                // SONG a righe: ogni riga suona dur8 ottavi per reps volte, poi la successiva; a fine lista ricomincia
                const int nr = std::clamp(sq.numRows, 0, kMaxSongRows);
                int64_t total = 0;
                for (int r = 0; r < nr; ++r) total += (int64_t) std::max(1, sq.rows[r].dur8) * std::max(1, sq.rows[r].reps);
                if (total <= 0) seqStepIdx = -1;
                else {
                    int64_t e = ((t - seqStart) / 48) % total, acc = 0;
                    for (int r = 0; r < nr; ++r) {
                        const SongRow& row = sq.rows[r];
                        const int64_t d = std::max(1, row.dur8), len = d * std::max(1, row.reps);
                        if (e < acc + len) {
                            seqStepIdx = r; seqRepIdx = (int) ((e - acc) / d);
                            seqKey = row.key < 0 ? base.key : row.key;
                            seqMode = row.mode < 0 ? base.mode : row.mode;
                            seqDeg = row.degree; seqType = row.type;
                            break;
                        }
                        acc += len;
                    }
                }
            } else {
            // lista piatta di step
            int64_t total = 0;
            for (int s = 0; s < 8; ++s) {
                if (sq.songRep[s] <= 0) continue;
                int64_t secLen = 0;
                for (auto& st : sq.sec[std::clamp(sq.songSec[s], 0, 2)]) secLen += st.dur8;
                total += secLen * sq.songRep[s];
            }
            if (total <= 0) { seqStepIdx = -1; }
            else {
                int64_t e = ((t - seqStart) / 48) % total;
                int64_t acc = 0; bool found = false;
                for (int s = 0; s < 8 && !found; ++s) {
                    if (sq.songRep[s] <= 0) continue;
                    const int sec = std::clamp(sq.songSec[s], 0, 2);
                    for (int rep = 0; rep < sq.songRep[s] && !found; ++rep)
                        for (int i = 0; i < 6 && !found; ++i) {
                            const ChordStep& st = sq.sec[sec][i];
                            if (st.dur8 <= 0) continue;
                            if (e < acc + st.dur8) {
                                found = true;
                                seqStepIdx = sec * 6 + i;
                                seqKey = st.key; seqMode = st.mode; seqDeg = st.degree; seqType = st.type;
                            }
                            acc += st.dur8;
                        }
                }
            }
            }
        }
    }

    computeEffective();

    // --- CC modulate
    for (int s = 0; s < 8; ++s) {
        const ModSlot& m = cfg.mods[s];
        if (m.src == SRC_OFF || m.dst < D_CC1 || !base.modAll) { lastCC[s] = -1; continue; }
        bool bip, patt; int pv;
        double u = sourceValue(m.src, bip, patt, pv);
        if (std::isnan(u)) continue;
        int v = patt ? pv : bip ? (int) std::lround(64 + u * 63 * m.amount / 100.0) : (int) std::lround(u * 127 * m.amount / 100.0);
        v = std::clamp(v, 0, 127);
        if (v != lastCC[s]) { sendCC(m.ccChan, portMask(m.ccPort), m.dst - D_CC1 + 1, v, out); lastCC[s] = v; }
    }

    // --- accordo per pad/drone: quantizzato
    const Chord now = chordFrom(eff);
    if (now != appliedChord) chordPending = true;
    bool chordChangeNow = false;
    if (chordPending && t % quantTicks() == 0) {
        appliedChord = now; chordPending = false; ++chordSerial; chordChangeNow = true;
    }

    if (resyncRequest) {
        for (auto& m : mrt) { if (m.running) { m.running = false; } }
        resyncRequest = false;
    }

    processPending(t, out);
    updatePad(t, chordChangeNow, out);
    updateDrone(t, chordChangeNow, out);
    motifTick(0, t, out);
    motifTick(1, t, out);
    prevEff = eff;
}

// ------------------------------------------------------------------ ingresso MIDI
void Engine::keyboardNote(bool on, int note, int vel, std::vector<MidiEv>& out) {
    const int ch = cfg.transChan;
    const int mask = portMask(cfg.kbPort);
    const Chord c = chordFrom(eff);
    auto send = [&](bool o, int n, int v) {
        n = std::clamp(n, 0, 127);
        out.push_back({mask, (uint8_t) ((o ? 0x90 : 0x80) | ((ch - 1) & 15)), (uint8_t) n, (uint8_t) (o ? std::clamp(v, 1, 127) : 64), 5});
    };
    if (!on) {
        for (int n : kbOut[note]) send(false, n, 0);
        kbOut[note].clear();
        return;
    }
    // quantizza alla scala corrente
    auto toScale = [&](int n) {
        for (int d = 0; d < 12; ++d) {
            for (int sgn : {0, -1, 1}) {
                int m = n + sgn * d;
                int rel = ((m - c.keyPc) % 12 + 12) % 12;
                const auto& sc = modeScale(c.mode);
                if (std::find(sc.begin(), sc.end(), rel) != sc.end()) return m;
            }
        }
        return n;
    };
    std::vector<int> outs;
    if (cfg.rotatorOn) {
        const Pattern& p = cfg.patterns[39];
        int par = p.v[0] - 20;
        int rot = p.v[1 + (rotIdx % 4)] - 20;
        rotIdx = (rotIdx + 1) % 4;
        int base = toScale(note);
        outs = {base, toScale(base + par), toScale(base + rot)};
    } else if (cfg.intervalOn) {
        const int home = 48 + (ivFollow ? c.rootPc : c.keyPc) + 12 * ivOct;
        // tabella: tasti bianchi = salti di grado, neri = funzioni
        const auto& sc = modeScale(c.mode);
        const int n = (int) sc.size();
        auto degreeOf = [&](int midi) {          // indice di scala assoluto (dal C-1 della tonica)
            int rel = midi - c.keyPc;
            int oct = (int) std::floor(rel / 12.0);
            int pc = rel - oct * 12;
            int best = 0;
            for (int i = 0; i < n; ++i) if (sc[i] <= pc) best = i;
            return oct * n + best;
        };
        auto fromDegree = [&](int dg) { return c.keyPc + scaleSemis(c.mode, dg); };
        if (ivLast < 0) ivLast = home;
        int jump = 0; bool play = true; bool special = true;
        switch (note) {
            case 62: ivLast = home; jump = 0; special = false; break;
            case 63: ivLast = home; play = false; break;
            case 61: jump = 0; special = false; break;
            case 66: { int r = (int) (rng() % 10); jump = r < 5 ? -(r + 1) : (r - 4); special = false; } break;
            case 68: ivOct = std::max(-3, ivOct - 1); play = false; break;
            case 70: ivOct = std::min(3, ivOct + 1); play = false; break;
            case 58: ivFollow = !ivFollow; play = false; break;
            case 49: ivLow = std::max(0, ivLow - 1); play = false; break;
            case 51: ivLow = std::min(ivHigh - 12, ivLow + 1); play = false; break;
            case 54: ivHigh = std::max(ivLow + 12, ivHigh - 1); play = false; break;
            case 56: ivHigh = std::min(127, ivHigh + 1); play = false; break;
            default: {
                static const int white[] = {53, 55, 57, 59, 60, 64, 65, 67, 69, 71};
                static const int jumps[] = {-5, -4, -3, -2, -1, 1, 2, 3, 4, 5};
                bool f = false;
                for (int i = 0; i < 10; ++i) if (white[i] == note) { jump = jumps[i]; f = true; }
                if (!f) return;
                special = false;
            }
        }
        (void) special;
        if (!play) return;
        int dg = degreeOf(ivLast) + jump;
        int m = fromDegree(dg);
        while (m > ivHigh) m -= 12 * std::max(1, (ivHigh - ivLow) / 12);
        while (m < ivLow) m += 12 * std::max(1, (ivHigh - ivLow) / 12);
        ivLast = m;
        outs = {m};
    } else {
        // trasposizione da C maggiore (tasti bianchi) alla tonalita'/modo correnti
        static const int whiteIdx[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
        int pc = note % 12, oct = note / 12;
        int sharp = 0;
        if (whiteIdx[pc] < 0) { pc -= 1; sharp = 1; }
        int dg = whiteIdx[pc];
        int m = 12 * oct + c.keyPc + scaleSemis(c.mode, dg) + sharp;
        if (c.keyPc > 6) m -= 12;
        outs = {m};
    }
    for (int o : kbOut[note]) send(false, o, 0);
    kbOut[note].clear();
    for (int o : outs) { send(true, o, vel); kbOut[note].push_back(std::clamp(o, 0, 127)); }
}

void Engine::midiIn(const uint8_t* d, int len, std::vector<MidiEv>& out, std::vector<ParamAction>& act) {
    if (len < 1) return;
    const int st = d[0] & 0xF0;
    const int ch = (d[0] & 0x0F) + 1;
    const int d1 = len > 1 ? d[1] : 0, d2 = len > 2 ? d[2] : 0;
    const bool noteOn = st == 0x90 && d2 > 0;
    const bool noteOff = st == 0x80 || (st == 0x90 && d2 == 0);

    if (ch == cfg.transChan && (noteOn || noteOff)) {
        keyboardNote(noteOn, d1, d2, out);
        return;
    }

    if (ch == cfg.ctrlChan) {
        if (noteOn) {
            midiVel = d2;
            const Chord c = chordFrom(eff);
            static const int whiteDeg[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
            const int pc = d1 % 12;
            if (cfg.kbMode <= 1) {
                if (whiteDeg[pc] >= 0) act.push_back({D_DEGREE, whiteDeg[pc]});
                else if (cfg.kbMode == 1) {
                    int type = -1;
                    switch (d1) {
                        case 61: type = CT_ALT1; break; case 63: type = CT_ALT2; break;
                        case 66: type = CT_TRIAD; break; case 68: type = CT_7TH; break;
                        case 70: type = CT_6TH; break; case 73: type = CT_SUS2; break; case 75: type = CT_SUS4; break;
                        default: {
                            static const int byPc[12] = {-1, CT_ALT1, -1, CT_ALT2, -1, -1, CT_TRIAD, -1, CT_7TH, -1, CT_6TH, -1};
                            type = byPc[pc];
                        }
                    }
                    if (type >= 0) act.push_back({D_CTYPE, type});
                }
            } else {
                const auto& sc = modeScale(c.mode);
                int rel = ((d1 - c.keyPc) % 12 + 12) % 12;
                auto it = std::find(sc.begin(), sc.end(), rel);
                if (it != sc.end()) {
                    int deg = (int) (it - sc.begin());
                    if (cfg.kbMode == 3 && m4First >= 0) {
                        int interval = ((deg - m4First) % 7 + 7) % 7 + 1;  // 2a, 3a ...
                        static const int typeFor[8] = {-1, -1, CT_SUS2, CT_TRIAD, CT_SUS4, CT_TRIAD, CT_6TH, CT_7TH};
                        if (typeFor[interval] >= 0) act.push_back({D_CTYPE, typeFor[interval]});
                    } else {
                        act.push_back({D_DEGREE, std::min(deg, 6)});
                        m4First = deg;
                    }
                }
            }
            return;
        }
        if (noteOff) { m4First = -1; return; }
        if (st == 0xE0) { midiPB = d1 | (d2 << 7); return; }
        if (st == 0xD0) { midiAT = d1; return; }
        if (st == 0xC0) return;
        if (st == 0xB0) {
            if (d1 == 1) midiMW = d2;
            auto mp = [](int v, int lo1, int hi1, int lo2, int hi2) {
                v = std::clamp(v, lo1, hi1);
                return lo2 + (int) std::lround((v - lo1) * (double) (hi2 - lo2) / std::max(1, hi1 - lo1));
            };
            switch (d1) {
                case 26: act.push_back({D_DEGREE, std::clamp(d2 - 1, 0, 6)}); break;
                case 27: act.push_back({D_CTYPE, std::clamp(d2 - 1, 0, 6)}); break;
                case 73: act.push_back({D_KEY, std::clamp(d2 - 1, 0, 11)}); break;
                case 74: act.push_back({D_MODE, std::clamp(d2, 0, kNumModes - 1)}); break;
                case 72: act.push_back({ACT_TEMPO, std::clamp(d2, 5, 127) * 2}); break;
                case 69: act.push_back({ACT_INVERT, d2 < 63 ? 1 : 0}); break;
                case 28: act.push_back({D_PAD_POS, mp(d2, 1, 100, 20, 108)}); break;
                case 29: act.push_back({D_PAD_STRUM, std::clamp(d2 - 1, 0, 6)}); break;
                case 30: act.push_back({D_PAD_RANGE, mp(d2, 1, 100, 1, 22)}); break;
                case 31: act.push_back({D_PAD_SPREAD, std::clamp(d2 - 1, 0, 5)}); break;
                case 63: act.push_back({D_PAD_VEL, d2}); break;
                case 85: act.push_back({D_PAD_ON, d2 >= 63}); break;
                case 32: act.push_back({D_DR_POS, std::clamp(d2 - 2, 0, 4)}); break;
                case 33: act.push_back({D_DR_TYPE, std::clamp(d2 - 1, 0, 3)}); break;
                case 34: act.push_back({D_DR_TRIG, std::clamp(d2 - 1, 0, 18)}); break;
                case 86: act.push_back({D_DR_ON, d2 >= 63}); break;
                case 59: act.push_back({D_HUMANIZE, std::clamp(d2, 0, 10)}); break;
                case 66: act.push_back({D_VEL_LOW, std::clamp(d2, 1, 100)}); break;
                case 90: act.push_back({ACT_PLAYALL, d2 >= 63}); break;
                case 89: act.push_back({ACT_LOADSEQ, std::clamp(d2 - 1, 0, 4)}); break;
                case 102: act.push_back({D_PN_X, std::clamp(d2, 0, 100)}); break;
                case 103: act.push_back({D_PN_Y, std::clamp(d2, 0, 100)}); break;
                case 104: act.push_back({D_PN_ZOOM, std::clamp(d2, 0, 100)}); break;
                case 105: act.push_back({D_PN_SPC, std::clamp(d2, 0, 100)}); break;
                case 106: act.push_back({D_PN_RUFF, std::clamp(d2, 0, 100)}); break;
                case 107: act.push_back({D_PN_PER, std::clamp(d2, 0, 100)}); break;
                default: {
                    // Motif 1: 35-42, 80, 87, 91  -  Motif 2: 43-50, 65, 88, 92
                    for (int k = 0; k < 2; ++k) {
                        const int b = k == 0 ? 35 : 43;
                        const int D = k == 0 ? D_M1_GATE : D_M2_GATE;
                        if (d1 == b)     act.push_back({D + 1, mp(d2, 1, 10, 0, 24)});
                        if (d1 == b + 1) act.push_back({D + 2, std::clamp(d2, 1, 16)});
                        if (d1 == b + 2) act.push_back({D + 3, std::clamp(d2 - 1, 0, 5)});
                        if (d1 == b + 3) act.push_back({D + 4, std::clamp(d2 - 1, 0, 39)});
                        if (d1 == b + 4) act.push_back({D + 5, std::clamp(d2 - 1, 0, 5)});
                        if (d1 == b + 5) act.push_back({ACT_RLEN1 + k, std::clamp(d2, 4, 32)});
                        if (d1 == b + 6) act.push_back({D + 6, std::clamp(d2 - 1, 0, 9)});
                        if (d1 == b + 7) act.push_back({D + 7, std::clamp(d2 - 1, 0, 39)});
                        if (d1 == (k == 0 ? 80 : 65)) act.push_back({D + 8, d2});
                        if (d1 == (k == 0 ? 87 : 88)) act.push_back({D + 9, d2 >= 63});
                        if (d1 == (k == 0 ? 91 : 92)) act.push_back({D + 0, std::clamp(d2, 5, 127)});
                    }
                }
            }
            return;
        }
        return;
    }

    // altri canali: MIDI thru verso l'host
    if (cfg.passThru && st != 0xF0 && len <= 3)
        out.push_back({1, d[0], (uint8_t) d1, (uint8_t) d2, 6});
}

// ------------------------------------------------------------------ UI
void Engine::fillSnapshot(UiSnapshot& s) const {
    s.chord = chordFrom(eff);
    for (auto& a : s.sounding) for (bool& b : a) b = false;
    for (auto& x : padSnd) s.sounding[P_PAD][x.note] = true;
    for (auto& x : droneSnd) s.sounding[P_DRONE][x.note] = true;
    for (int k = 0; k < 2; ++k) {
        if (mrt[k].hasNote) s.sounding[P_M1 + k][mrt[k].snd.note] = true;
        s.motifStep[k] = mrt[k].lastIdx;
        s.motifBeat[k] = mrt[k].lastBeat;
        s.partRunning[P_M1 + k] = mrt[k].running;
        computeNZ(eff, k, s.nz[k]);
    }
    s.partRunning[P_PAD] = padRunning;
    s.partRunning[P_DRONE] = droneRunning;
    s.seqStep = seqRunning ? seqStepIdx : -1;
    s.seqRep = seqRepIdx;
    s.seqPlaying = seqRunning;
    for (int i = 0; i < 3; ++i) s.lfo[i] = lfoVal[i];
    s.eff = eff;
}

} // namespace ago
