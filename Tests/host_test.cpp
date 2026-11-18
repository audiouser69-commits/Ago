// Host di prova (Linux): carica il processore, simula il trasporto dell'host, conta il MIDI in uscita
// e salva uno screenshot dell'interfaccia mentre suona. Build: cmake -DAGO_HOST_TEST=ON
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <map>

struct FakeHead : juce::AudioPlayHead {
    double ppq = 0, bpm = 120; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo p; p.setPpqPosition(ppq); p.setBpm(bpm); p.setIsPlaying(playing); return p;
    }
};

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String shot = argc > 1 ? argv[1] : "/tmp/ago_ui.png";
    auto proc = std::make_unique<AgoProcessor>();
    FakeHead head;
    proc->setPlayHead(&head);
    const double sr = 48000; const int bs = 512;
    proc->prepareToPlay(sr, bs);
    for (auto* id : { "padOn", "droneOn", "m1On", "m2On" }) proc->setParam(id, 1);
    proc->setParam("padStrum", 3); proc->setParam("padRange", 7); proc->setParam("polyChain", 2);
    proc->setParam("droneTrig", 10); proc->setParam("m2Patt", ago::kPatternNZ); proc->setParam("seqArm", 1);
    auto cfg = proc->getConfig();
    cfg.mods[0] = { ago::SRC_LFO1, ago::D_PAD_POS, 30, 1, 0 };
    cfg.mods[1] = { ago::SRC_LFO2, ago::D_PN_X, 60, 1, 0 };
    cfg.man[0].on = 1;
    proc->setConfig(cfg);

    if (argc > 2 && juce::String(argv[2]) == "follower") { proc->setParam("linkRole", 2); proc->setParam("linkPart", 2); }
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc->createEditor());
    ed->setVisible(true);

    juce::AudioBuffer<float> buf(2, bs);
    juce::MidiBuffer midi;
    std::map<int, int> open;
    int ons = 0, offs = 0, bad = 0;
    const double spb = sr * 60.0 / head.bpm;
    auto run = [&](int blocks) {
        for (int b = 0; b < blocks; ++b) {
            midi.clear();
            proc->processBlock(buf, midi);
            int last = -1;
            for (const auto m : midi) {
                if (m.samplePosition < last || m.samplePosition < 0 || m.samplePosition >= bs) ++bad;
                last = m.samplePosition;
                auto msg = m.getMessage();
                const int key = msg.getChannel() * 128 + msg.getNoteNumber();
                if (msg.isNoteOn()) { ++ons; open[key]++; }
                else if (msg.isNoteOff()) { ++offs; if (open[key] > 0) open[key]--; }
            }
            if (head.playing) head.ppq += bs / spb;
        }
    };
    const int blocksPerBar = (int) (4 * spb / bs);
    run(blocksPerBar * 8);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(300);
    auto img = ed->createComponentSnapshot(ed->getLocalBounds());
    juce::File(shot).deleteFile();
    juce::FileOutputStream fos { juce::File(shot) };
    juce::PNGImageFormat().writeImageToStream(img, fos);

    for (int tab = 1; tab < 9; ++tab) {
        if (auto* ae = dynamic_cast<AgoEditor*>(ed.get())) ae->showTab(tab);
        run(blocksPerBar);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
        auto im = ed->createComponentSnapshot(ed->getLocalBounds());
        juce::File f(shot.replace(".png", "_tab" + juce::String(tab) + ".png"));
        f.deleteFile();
        juce::FileOutputStream os { f };
        juce::PNGImageFormat().writeImageToStream(im, os);
    }
    head.playing = false;
    run(2);
    int stuck = 0; for (auto& kv : open) stuck += kv.second;
    std::printf("note on %d, off %d, appese %d, timestamp fuori blocco %d\n", ons, offs, stuck, bad);
    ed.reset();
    const bool ok = ons > 200 && stuck == 0 && bad == 0;
    std::printf(ok ? "host test OK\n" : "host test FALLITO\n");
    return ok ? 0 : 1;
}
