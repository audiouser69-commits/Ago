// Test della modalita' Link: 1 leader + follower Pad/Drone/M1/M2/Tutte, ordine di processo variabile come in Live,
// loop del trasporto, stop, rimozione del leader. Ogni follower deve emettere solo la sua parte, agli stessi campioni del leader.
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <map>
#include <random>

struct FakeHead : juce::AudioPlayHead {
    double ppq = 0, bpm = 124; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo p; p.setPpqPosition(ppq); p.setBpm(bpm); p.setIsPlaying(playing); return p;
    }
};

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf("FALLITO: " __VA_ARGS__); std::printf("\n"); } } while (0)

struct Tally {
    std::map<int, int> open; int ons = 0;
    std::vector<std::tuple<long, int, int, int>> log;   // (blocco*1000+campione, stato, nota, canale)
    std::set<int> chans;
};

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    FakeHead head;
    const double sr = 48000; const int bs = 480;
    std::vector<std::unique_ptr<AgoProcessor>> inst;
    for (int i = 0; i < 6; ++i) { inst.push_back(std::make_unique<AgoProcessor>()); inst.back()->setPlayHead(&head); inst.back()->prepareToPlay(sr, bs); }
    AgoProcessor& L = *inst[0];
    L.setParam("linkRole", 1);
    for (auto* id : { "padOn", "droneOn", "m1On", "m2On" }) L.setParam(id, 1);
    L.setParam("padRange", 5); L.setParam("droneTrig", 13); L.setParam("m1Var", 5); L.setParam("m1Acc", 1);
    L.setParam("seqArm", 1); L.setParam("m2Patt", ago::kPatternNZ);
    auto cfg = L.getConfig();
    cfg.mods[0] = { ago::SRC_MOD4, ago::D_PAD_POS, 40, 1, 0 };   // casuale: se i motori fossero separati divergerebbero
    cfg.mods[1] = { ago::SRC_LFO3, ago::D_M1_POS, 60, 1, 0 };
    L.setConfig(cfg);
    for (int k = 1; k <= 5; ++k) { inst[k]->setParam("linkRole", 2); inst[k]->setParam("linkPart", k - 1); }
    // un'istanza con i parametri diversi dal leader: non deve importare
    inst[1]->setParam("padRange", 22); inst[1]->setParam("key", 5);

    std::vector<Tally> t(6);
    juce::AudioBuffer<float> buf(2, bs);
    std::mt19937 rng(3);
    long block = 0;
    auto runBlocks = [&](int count, bool skipLeader = false) {
        for (int b = 0; b < count; ++b, ++block) {
            std::vector<int> order = { 0, 1, 2, 3, 4, 5 };
            std::shuffle(order.begin(), order.end(), rng);
            for (int i : order) {
                if (skipLeader && i == 0) continue;
                if (!inst[i]) continue;
                juce::MidiBuffer m;
                inst[i]->processBlock(buf, m);
                for (const auto meta : m) {
                    auto msg = meta.getMessage();
                    if (!msg.isNoteOnOrOff()) continue;
                    int key = msg.getChannel() * 128 + msg.getNoteNumber();
                    if (msg.isNoteOn()) { t[i].ons++; t[i].open[key]++; t[i].chans.insert(msg.getChannel()); }
                    else if (t[i].open[key] > 0) t[i].open[key]--;
                    t[i].log.emplace_back(block * 1000 + meta.samplePosition, msg.isNoteOn(), msg.getNoteNumber(), msg.getChannel());
                }
            }
            if (head.playing) head.ppq += bs / (sr * 60.0 / head.bpm);
        }
    };
    const int bar = (int) (4 * sr * 60.0 / head.bpm / bs);
    head.playing = false; runBlocks(2); head.playing = true;   // il leader si registra prima del Play (come in Live)
    runBlocks(bar * 6);
    head.ppq = 4.0 * 2;          // loop: torna alla battuta 3
    runBlocks(bar * 4);
    head.playing = false;
    runBlocks(3);

    const int chOf[4] = { 1, 2, 3, 4 };
    for (int k = 1; k <= 4; ++k) {
        CHECK(t[k].ons > 10, "follower %d: troppe poche note (%d)", k, t[k].ons);
        CHECK(t[k].chans.size() == 1 && *t[k].chans.begin() == chOf[k - 1], "follower %d: canali sbagliati", k);
        // stesse note e stessi campioni del leader per quel canale
        std::vector<std::tuple<long, int, int, int>> ref;
        for (auto& e : t[0].log) if (std::get<3>(e) == chOf[k - 1]) ref.push_back(e);
        CHECK(ref == t[k].log, "follower %d: eventi diversi dal leader (%zu vs %zu)", k, ref.size(), t[k].log.size());
        if (ref != t[k].log) {
            size_t i = 0; while (i < ref.size() && i < t[k].log.size() && ref[i] == t[k].log[i]) ++i;
            for (size_t j = i; j < i + 4 && j < ref.size(); ++j) std::printf("  L %ld %d %d | ", std::get<0>(ref[j]), std::get<1>(ref[j]), std::get<2>(ref[j]));
            std::printf("\n");
            for (size_t j = i; j < i + 4 && j < t[k].log.size(); ++j) std::printf("  F %ld %d %d | ", std::get<0>(t[k].log[j]), std::get<1>(t[k].log[j]), std::get<2>(t[k].log[j]));
            std::printf("\n");
        }
    }
    CHECK(t[5].log == t[0].log, "follower 'Tutte' diverso dal leader");
    for (int i = 0; i < 6; ++i) { int s = 0; for (auto& kv : t[i].open) s += kv.second; CHECK(s == 0, "istanza %d: %d note appese", i, s); }
    std::printf("leader %d note, follower pad %d drone %d m1 %d m2 %d tutte %d\n", t[0].ons, t[1].ons, t[2].ons, t[3].ons, t[4].ons, t[5].ons);

    // rimozione del leader a trasporto in corsa: i follower ricevono i note-off
    for (auto& x : t) { x.open.clear(); x.log.clear(); }
    head.playing = true;
    runBlocks(bar * 2);
    inst[0].reset();
    runBlocks(4, true);
    for (int i = 1; i < 6; ++i) { int s = 0; for (auto& kv : t[i].open) s += kv.second; CHECK(s == 0, "dopo rimozione leader, istanza %d: %d note appese", i, s); }
    CHECK(inst[1]->getLinkStatus() == 4, "il follower deve segnalare 'nessun leader'");

    std::printf(fails ? "%d test Link FALLITI\n" : "test Link OK\n", fails);
    return fails ? 1 : 0;
}
