// AGO - teoria musicale: tonalita', modi, accordi, pool di note
#pragma once
#include <array>
#include <vector>
#include <string>
#include <algorithm>

namespace ago {

// Tonalita' in ordine di circolo delle quinte (come l'encoder dell'NDLR e la CC 73)
inline const char* keyNameCircle(int idx) {
    static const char* n[12] = {"C","G","D","A","E","B","F#","Db","Ab","Eb","Bb","F"};
    return n[((idx % 12) + 12) % 12];
}
inline int keyPcFromCircle(int idx) { return (((idx % 12) + 12) % 12) * 7 % 12; }

inline const char* pcName(int pc) {
    static const char* n[12] = {"C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
    return n[((pc % 12) + 12) % 12];
}

// Nome nota con la convenzione di Ableton (C3 = 60)
inline std::string noteName(int midi) {
    return std::string(pcName(midi)) + std::to_string(midi / 12 - 2);
}

constexpr int kNumModes = 15;
inline const char* modeName(int m) {
    static const char* n[kNumModes] = {"Major","Dorian","Phrygian","Lydian","Mixolydian","Minor",
        "Locrian","Gypsy Min","Harm Minor","Min Penta","Whole Tone","Tonic 2nds","Tonic 3rds",
        "Tonic 4ths","Tonic 6ths"};
    return n[std::clamp(m, 0, kNumModes - 1)];
}

inline const std::vector<int>& modeScale(int m) {
    static const std::vector<int> s[kNumModes] = {
        {0,2,4,5,7,9,11}, {0,2,3,5,7,9,10}, {0,1,3,5,7,8,10}, {0,2,4,6,7,9,11},
        {0,2,4,5,7,9,10}, {0,2,3,5,7,8,10}, {0,1,3,5,6,8,10}, {0,2,3,6,7,8,11},
        {0,2,3,5,7,8,11}, {0,3,5,7,10},     {0,2,4,6,8,10},
        {0,2,7}, {0,4,7}, {0,5,7}, {0,7,9}  // modi "Tonic": tonica + intervallo + quinta
    };
    return s[std::clamp(m, 0, kNumModes - 1)];
}

// semitoni dalla tonica per lo step k della scala (k puo' superare l'ottava)
inline int scaleSemis(int mode, int k) {
    const auto& s = modeScale(mode);
    const int n = (int) s.size();
    int oct = k >= 0 ? k / n : -((-k + n - 1) / n);
    int idx = k - oct * n;
    return s[idx] + 12 * oct;
}

constexpr int kNumChordTypes = 7;
enum ChordType { CT_TRIAD = 0, CT_7TH, CT_SUS2, CT_ALT2, CT_ALT1, CT_SUS4, CT_6TH };
inline const char* chordTypeName(int t) {
    static const char* n[kNumChordTypes] = {"Triad","7th","sus2","alt2","alt1","sus4","6th"};
    return n[std::clamp(t, 0, kNumChordTypes - 1)];
}

// alt1/alt2: tipo fisso per ogni grado (dal manuale)
inline int resolveChordType(int type, int degree) {
    static const int alt1[7] = {CT_TRIAD, CT_7TH, CT_TRIAD, CT_TRIAD, CT_7TH, CT_7TH, CT_6TH};
    static const int alt2[7] = {CT_7TH, CT_7TH, CT_TRIAD, CT_7TH, CT_7TH, CT_6TH, CT_7TH};
    degree = std::clamp(degree, 0, 6);
    if (type == CT_ALT1) return alt1[degree];
    if (type == CT_ALT2) return alt2[degree];
    return type;
}

struct Chord {
    int keyPc = 0, mode = 0, degree = 0, type = 0;   // type gia' risolto
    int rootPc = 0;
    std::vector<int> pcs;      // pitch class dei toni, nell'ordine radice-terza-quinta-(settima)
    std::vector<int> semis;    // semitoni dalla radice (per voicing)
    bool operator==(const Chord& o) const {
        return keyPc == o.keyPc && mode == o.mode && degree == o.degree && type == o.type;
    }
    bool operator!=(const Chord& o) const { return !(*this == o); }
};

inline Chord buildChord(int keyCircle, int mode, int degree, int rawType) {
    Chord c;
    c.keyPc = keyPcFromCircle(keyCircle);
    c.mode = std::clamp(mode, 0, kNumModes - 1);
    c.degree = std::clamp(degree, 0, 6);
    c.type = resolveChordType(rawType, c.degree);
    std::vector<int> steps;
    switch (c.type) {
        case CT_7TH:  steps = {0, 2, 4, 6}; break;
        case CT_SUS2: steps = {0, 1, 4}; break;
        case CT_SUS4: steps = {0, 3, 4}; break;
        case CT_6TH:  steps = {0, 2, 4, 5}; break;
        default:      steps = {0, 2, 4}; break;
    }
    const int rootSemi = scaleSemis(c.mode, c.degree);
    c.rootPc = (c.keyPc + rootSemi) % 12;
    for (int s : steps) {
        int semi = scaleSemis(c.mode, c.degree + s);
        int pc = (c.keyPc + semi) % 12;
        // nelle scale con poche note i toni possono coincidere: niente doppioni
        if (std::find(c.pcs.begin(), c.pcs.end(), pc) == c.pcs.end()) {
            c.pcs.push_back(pc);
            c.semis.push_back(semi - rootSemi);
        }
    }
    return c;
}

// Sonorita' per il numero romano: 0 maggiore, 1 minore, 2 diminuito
inline int chordQuality(const Chord& c) {
    if (c.semis.size() < 2) return 0;
    int third = c.semis.size() > 1 ? c.semis[1] : 4;
    int fifth = c.semis.size() > 2 ? c.semis[2] : 7;
    if (third == 3 && fifth == 6) return 2;
    if (third == 3) return 1;
    return 0;
}

inline std::string romanFor(const Chord& c) {
    static const char* up[7] = {"I","II","III","IV","V","VI","VII"};
    static const char* lo[7] = {"i","ii","iii","iv","v","vi","vii"};
    int q = chordQuality(c);
    std::string s = q == 0 ? up[c.degree] : lo[c.degree];
    if (q == 2) s += "\xc2\xb0";
    return s;
}

inline bool pcIn(const std::vector<int>& pcs, int pc) {
    return std::find(pcs.begin(), pcs.end(), ((pc % 12) + 12) % 12) != pcs.end();
}

} // namespace ago
