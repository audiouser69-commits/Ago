# AGO — harmonic conductor (VST3 per Windows)

AGO è un clone software del Conductive Labs **NDLR** (firmware 3.3). È un generatore MIDI con quattro parti
che suonano sempre nella stessa tonalità, nello stesso modo e sullo stesso accordo, a tempo con l'host:

| Parte | Cosa fa | Synth consigliato |
|---|---|---|
| **PAD** | tutte le note dell'accordo in un range (1-22 note), con voicing, strum e poly chain | DeepMind 12 (polifonico) |
| **DRONE** | radice (+5a, +ottava) tenuta o ritriggerata su cadenze 4/4, 3/4, 5/4 | pad lunghi, anche una cassa |
| **MOTIF 1 / 2** | arpeggi: pattern di note × ritmo, con variazioni, clock divider, gate, accenti | Pro-800, FM-1, lead/bassi mono |

Cambi accordo con i 7 tasti I–VII e i 7 tipi (Triad, 7th, sus2, alt2, alt1, sus4, 6th). Tutte le parti si adeguano.

## Installazione

1. Scarica `AGO-Windows.zip` (Actions → ultima build → artefatto, oppure ramo `pacchetti`).
2. Copia la cartella `AGO.vst3` in `C:\Program Files\Common Files\VST3` oppure nella cartella VST3 personalizzata di Live.
3. In Live: Preferenze → Plug-in → *Rescan*. AGO compare tra gli strumenti.
4. `AGO.exe` è la versione standalone: clock interno o MIDI Clock, uscita sulle porte A/B.

## Uso in Ableton Live (importante)

Live fa passare il MIDI in uscita da un plugin verso un'altra traccia (MIDI From = traccia del plugin). Però **unisce tutto su un solo canale**, quindi da una sola istanza le parti non si separano. Ci sono due soluzioni.

### 1. LINK: un AGO per parte, tutto dentro Live (consigliato)

- Traccia **AGO leader**: tab *MIDI / PORTE* → Link = **Leader**. Qui si suona: accordi, parti, pattern, modulazioni, sequencer.
- Una traccia per parte con un altro AGO: Link = **Follower**, Parte = Pad / Drone / Motif 1 / Motif 2 (o Tutte), stesso Gruppo del leader.
- Sulla traccia di ogni synth (DeepMind, Pro-800, emulatore, FM-1...): **MIDI From** = traccia del follower, sotto scegli "AGO". Poi Monitor **In**, oppure arma la traccia per registrare le note in clip.

Tutte le istanze di un gruppo usano **lo stesso motore**: i follower ricevono le note esatte del leader, sullo stesso campione, anche con le parti casuali (Random, probabilità degli LFO, Humanize). Ci sono 4 gruppi per avere più AGO indipendenti. I follower mostrano accordo e tastiera del leader. Se il leader viene rimosso, i follower spengono le note.

### 2. Porte A/B: dispositivi MIDI diretti

Ogni parte può uscire anche su:

- **Host**: uscita MIDI del plugin (Bitwig, Reaper, FL Studio, standalone, oppure il leader in Live).
- **Port A / Port B**: un dispositivo MIDI di Windows aperto direttamente dal plugin, con i canali separati. Si sceglie nella tab *MIDI / PORTE*.

Setup tipico con le porte:

- Metti AGO su una traccia MIDI (anche vuota) e premi Play in Live. AGO segue il trasporto e il tempo di Live.
- **Port A** = USB del DeepMind 12, Pad su canale 1. **Port B** = Pro-800, Motif 1 su canale 1.
- Per registrare le note in Live: crea una porta virtuale con **loopMIDI** e mandaci AGO (Port A). Poi usa la porta loopMIDI come ingresso delle tracce che suonano i synth.
- Se una porta risulta "impossibile aprirla", disattivala come uscita (Track/Sync) nelle preferenze MIDI di Live: Windows a volte non la condivide tra due programmi.
- Se le note esterne arrivano in anticipo rispetto all'audio, alza **Ritardo porte A/B** (tab MIDI).

Controllo da tastiera (traccia di AGO armata o in monitor):

- **Canale controllo (default 15)**: tasti bianchi = gradi I–VII. Tasti neri C#3–D#4 = tipi di accordo (alt1, alt2, triad, 7th, 6th, sus2, sus4). Le CC seguono la tabella dell'NDLR (26 grado, 27 tipo, 73 tonalità, 74 modo, 28-50 parametri delle parti, 102-107 Perlin...).
- **Canale trasposizione (default 14)**: le note dei tasti bianchi di C maggiore vengono trasposte nella tonalità/modo correnti. Su questo canale lavorano anche Rotator e Interval.

## Funzioni

- **Tonalità** nel circolo delle quinte. **15 modi**: Major, Dorian, Phrygian, Lydian, Mixolydian, Minor, Locrian, Gypsy Min, Harmonic Minor, Minor Pentatonic, Whole Tone, Tonic 2nds/3rds/4ths/6ths.
- **Pad**: Position, Range, Spread (6 voicing), Strum (7), Velocity, Quantize (1/4, 1/8, off), Poly Chain (1-4 canali consecutivi), Chord Invert (voice leading minimo).
- **Drone**: Position (5 ottave), Notes (4 combinazioni), Trigger (8 cadenze sulla tonica + 11 sull'accordo).
- **Motif**: Position, Patt Len 1-16, Variation (Forward, Backward, Ping-Pong, Ping-Pong rip., Odd-Even, Random), Pattern 1-40 + NZ, Clock Div (1/1, 1/2, 1/4, 1/8, 1/4T, 1/8T, 1/16), Rhy Len 4-32, Accent (10), Rhythm 1-40, Gate, Velocity. Offset del Motif 2 (0-3/16).
- **Pattern editor**: CHORD (segue l'accordo), SCALE e CHROMATIC (fissi sulla tonica). 20 pattern di fabbrica + 20 utente, tutti modificabili.
- **Rhythm editor**: velocity, REST, TIE, modalità euclidea (colpi + rotazione).
- **Matrice di modulazione** a 8 slot. Sorgenti: LFO 1-3 (sec o beat, 8 forme compreso PATT, probabilità), MOD 1-5 preset, Velocity/Pitch Bend/Mod Wheel/Aftertouch. Destinazioni: tutte quelle dell'NDLR, step 1-16 dei motif e CC 1-127 (con canale e porta). On/off globale e per parte.
- **Perlin noise (NZ)**: X, Y, Zoom, Spacing, Ruff, Persistence, note pool per motif.
- **Chord sequencer / SONG** (tab CHORD SEQ / SONG): 5 sequenze, ognuna in una di due modalità. Si attiva con ARM e parte al prossimo downbeat.
  - **SONG a righe** (default): fino a 64 righe. Ogni riga ha grado, tipo, tonalità e modo (globali o propri), durata (da 1/4 di battuta a 8 battute) e ripetizioni x1–x16. Con **+ AGGIUNGI RIGA** si aggiunge una riga che copia l'ultima; ^ v spostano la riga, X la elimina. Esempio in F maggiore: I ×3, VII ×1, IV ×2, V ×2. La riga in riproduzione si illumina e mostra la ripetizione corrente. Con tonalità e modo globali, cambiando la tonalità in alto si traspone tutta la song.
  - **Sezioni A/B/C** (come l'NDLR): sezioni da 6 step e song di 8 slot con ripetizioni.
- **Rotator** e **Interval mode** come nel firmware 3.
- **Clock**: Host (default), Interno con PLAY, oppure MIDI Clock in ingresso (Start/Stop/Continue).
- PLAY ALL, RESYNC (riallinea i motif al prossimo downbeat), PANIC.

Tutto, compresi pattern, ritmi, matrice e sequenze, è salvato nel progetto di Live. I preset di Live sostituiscono i preset globali dell'NDLR.

## Non incluso (rispetto all'hardware)

Clock CV in/out, MIDI Clock in uscita, la schermata "MIDI Controller" (mixer volume/pan via CC: si può fare con gli slot CC della matrice), la CC 41 per la Volca FM, l'inserimento dei pattern dalla tastiera.

## Build

GitHub Actions compila a ogni push su `main`: prima i test del motore, poi Windows (MinGW, VST3 + standalone) e macOS.
In locale: `cmake -S . -B build && cmake --build build`.
`Tests/engine_test.cpp` verifica teoria, tabella note del manuale, variazioni, euclideo, pad, sequencer e assenza di note appese.
`-DAGO_HOST_TEST=ON` compila un host di prova, che simula il trasporto e salva screenshot della UI, e `ago_link_test`. Quest'ultimo carica 1 leader e 5 follower processati in ordine casuale, con loop e stop, poi verifica che ogni follower emetta esattamente le note del leader per la sua parte.
