# HYP-1 on the M-Vave SMK-37

HYP-1 also builds for the M-Vave SMK-37 keyboards (SMK-37 Elite, SMK-37 Pro, MKE-P37): the same chip as the
FM-1 (JieLi AC791N), a different panel. It is the same HYP-1: the same pieces, pages, sounds, screen and
[field manual](docs/HYP-1_Manual.pdf). The SMK-37's extra controls go to what the FM-1 makes you hold a
button for: the drum steps are always on the pads, the acid is on the faders, and the wheels bend and open
the filters. And it has a sixth voice the FM-1 build has not: a DX7-style FM synth with the 128 factory
patches of the SMK-37, played on the 37 keys over the groove; the pad can sing those patches too. The FM-1 build is unchanged (byte for byte); everything for the SMK-37 is behind `BOARD_SMK37`.

## Models and packages

| Keyboard | Package | Identity |
| --- | --- | --- |
| SMK-37 Elite | `hyp1-smk37-elite.fwsc` | `SMK-37 Elite_7XXYYZZ` |
| SMK-37 Pro (also Starrykey 37 Play) | `hyp1-smk37.fwsc` | `SMK-37 Pro_7XXYYZZ` |
| MKE-P37 | `hyp1-smk37-mke.fwsc` | `MKE-P37_7XXYYZZ` |

All three are the same hardware. Each package carries its model's name, so M-Vave's updater still takes that
unit's factory firmware afterwards. Install the package for your model.

## Controls

The numbers 1-25 are the field manual's: each FM-1 control keeps its HYP-1 job on the SMK-37 control printed
with the same meaning, so the manual applies as it is. 26 and up are the SMK-37's own. On screen, the help
cards and hints use the SMK-37's printed names (for example **BT (SAVE)**, **SCALE + CHORD**).

| # | SMK-37 | FM-1 / manual | HYP-1 |
| --- | --- | --- | --- |
| 1 | K1 | MASTER | volume (an endless knob: at each power-on it starts at about 70 % of its travel, half gain) |
| 2 | K2 | SELECT | the pages; with PATCH held, the tempo |
| 3 | K4 | PRESETS | the piece, 01-10 (with SCALE held: on the next bar); the picker shows for a moment |
| 4 | K3 | ALGORITHM | the mode; with SCALE held, the sound of the voice on screen (drums: the kit) |
| 5-8 | K5 K6 K7 K8 | KNOB 1-4 | the page's four values, a b c d, left to right as on the screen |
| 10 / 11 | OCTAVE- / OCTAVE+ | OCT- / OCT+ | the octave of the page's voice, or the song's; in the sequencer, the drum |
| 12 | FX | FX | the effects menu (the keys are effects while it is on) |
| 13 | SCALE | SEL | shift, held |
| 14 | CHORD | ENV | the pad's pages; SCALE + CHORD on a voice's page: that voice by hand (on the FM pages: the keys play FM on every page) |
| 15 | NOTE REPEAT | LFO | the drone's pages |
| 16 | PARA | EDIT | the next chord, on the bar |
| 17 | GLOBE | GLO | Global (root, theme, tune, MIDI out); here BT stores the piece |
| 18 | PATCH | HOME | the Tempo page; again: the FM voice, again: its keys page; held + K2: the tempo |
| 19 | BT | SAVE | breakdown, build, drop; on Global: store (held 1.5 s: factory); in FX, held: lock |
| 20 | ARP | ARP | the acid's pages |
| 21 | SEQ | SEQ | drums, links, kit, drum drift; held: the sequencer page |
| 22 | SEQ PLAY | PLAY | play / stop |
| 23 | SEQ REC | REC | freeze |
| 24 / 25 | keys F3..G5 (the middle 27) | white / black keys | the scale; a voice by hand; the steps on the sequencer page; the effects in FX |
| 26 | keys C3..E3, G#5..C6 | | the scale too; on a hand-played voice's pages, that voice below F3 and above G5 |
| 27 | 16 pads | | the steps of the pads' lane (below), always: a pad turns its hit on or off |
| 28 | < / > | | the pads' lane: kick, snare, clap, hat, open hat, perc |
| 29 | KNOB BANK | | the picker (below); lit while it is open |
| 30 | faders 1-4 | | the current bank's four values (below) |
| 31 | FADER BANK | | the fader bank, A B C; lit off A |
| 32 | pitch wheel | | bend, +-2 semitones, on the pad, the drone and the acid |
| 33 | mod wheel | | a filter macro: the acid's and the drone's cutoff open together (up to 3 octaves) |
| | OCTAVE- + OCTAVE+ held 5 s | OCT- + OCT+ | update mode |
| | OCTAVE- + OCTAVE+ held at power-on | | hardware calibration: teach the buttons and knobs again |

### The pads: the steps

The pads are the sixteen steps of one drum lane, in reading order: the top row steps 1-8, the bottom row
9-16. A pad turns its step's hit on or off, on any page, in the pattern the sequencer edits (Drums page, d).
Lit: the hits in the drums' colour, the playhead bright (orange on a hit), beats 1, 5, 9 and 13 faint; in a
breakdown the hits dim. `<` and `>` choose the lane; the main page marks it on the drum pattern (an orange
tick, and `PADS KCK` under the pattern number).

### KNOB BANK: the picker

The screen and the pads show the same two rows: the top row pieces 01-08, the bottom row the six lanes
(KCK SNR CLP HAT OPN PRC) and then pieces 09 and 10. A piece pad plays that piece (SCALE held while playing:
on the next bar, its pad blinking until then); a lane pad picks that lane and the pads go back to its steps.
KNOB BANK again closes it. Turning K4 (PRESETS) shows it for a moment.

### The faders

| Bank | Fader 1 | Fader 2 | Fader 3 | Fader 4 |
| --- | --- | --- | --- | --- |
| **A** levels | Pad | Drone | Acid | Drums |
| **B** acid | Cutoff | Reso | Env mod | Decay |
| **C** space | Echo | Reverb | Noise | Drone cutoff |

A fader takes over only once it moves, also after a bank change, so nothing jumps. The fader card (the
banks as tabs, four faders, the one moved in orange) shows while you move them. One card at a time:
KNOB BANK closes the fader card, FADER BANK the picker. The values are the pages' own: stored with the piece.

### The FM voice (DX7)

A 6-operator FM synth after the DX7: msfa, the core of Dexed, as Felucca ported it to integers. Its patches are
the 128 factory voices of the SMK-37 and the FM-1 (banks A-D: pianos, organs, leads, pads, guitars, basses, brass,
strings, voices, bells and percussion), and a bank of 32 of your own (U01-U32, below).

| Page | Reach it | a (K5) | b (K6) | c (K7) | d (K8) |
| --- | --- | --- | --- | --- | --- |
| FM | PATCH x2 | Level | Patch (A01-D32, U01-U32; the name on the line above) | Bright (the modulators: duller, brighter) | Space (echo and reverb) |
| FM keys | PATCH x3 | Snap (Off / Scale: each key to the scale's nearest note) | Mono (Poly, 6 notes / Mono, legato) | Glide | Velocity (how much it counts) |

- **Play it:** on an FM page, SCALE + CHORD. From then on the keys (all 37, with velocity) play the FM voice on
  every page, over the groove; the header says **KEYS:FM**. SCALE + CHORD on an FM page again: the keys edit the
  scale again. On a page of a voice played by hand (pad, drone, acid), the keys play that voice instead.
- **OCTAVE- / +** on the FM pages: its octave. **SCALE + K3**: the next patch. The wheels bend it.
- **MIDI:** channel 2 in plays it (from a DAW or another keyboard); what you play goes out on channel 2.
- **The meter** F, between A and K on the main page.
- **The pad in FM:** on the Pad page turn Sound (K7) past Sine: A01 ... D32, U01 ... U32. The pad's four voices
  sing the patch: each breath (or stab, or key) is a note, so the chords still breathe; Bright moves the patch's
  modulators. Strings, voices and pads (bank C) breathe best; the pianos and bells strike at each breath.

**Your own bank (U01-U32).** Send a DX7 32-voice bank (a `.syx` file: `F0 43 0n 09 20 00`, 4104 bytes) to the
keyboard over USB: HYP-1 checks it, says **DX7 BANK IN** with its first voice's name, and stores it at once (the
music pauses a moment while it is written, as a piece's store does). It stays after a power cycle; the next bank
replaces it. A file that is not a good DX7 bank is refused (**NOT A DX7 BANK**) and nothing is written.

```sh
python3 tools/dx7_bank.py info MYBANK.syx          # the 32 names; is it a good bank?
python3 tools/dx7_bank.py send MYBANK.syx          # to the keyboard (pip3 install mido python-rtmidi)
python3 tools/dx7_bank.py factory C strings.syx    # a factory bank as a file, to edit elsewhere
```

Any SysEx sender works too (SysEx Librarian, MIDI-OX, a DAW). Yamaha's own DX7 cartridges are not in the
firmware; if you have them as `.syx`, send one as your bank.

**Load.** Six FM notes held with the pad in FM is the heaviest HYP-1 gets (on a computer, about a fifth more than
HYP-1 alone). On the keyboard, hold GLOBE for its card: it shows the CPU load and the xruns. If they climb, use
Mono or fewer notes.

### The wheels

The wheels never change what is stored: back at rest, the music is as it was. The pitch wheel bends the FM
voice too. While one is off rest the
header says **BEND** or **FLT**.

## What the SMK-37 cannot do

- No key lights: the FM-1's lit keys (the scale, the steps, the effects) are not there. The pads show the
  steps, the screen the rest (the FX menu lists every key's effect).
- No TRS MIDI IN (on the SMK-37 the jack is a MIDI output, unused here). MIDI in and clock come over USB.
- Remote (SCALE + GLOBE) still plays the FM-1 emulator; the SMK-37's extra controls are not sent.

## Build

See [BUILDING.md](BUILDING.md) for the toolchain.

```sh
BOARD=smk37 MODEL=elite ./build.sh     # build/hyp1-smk37-elite.fwsc
BOARD=smk37 ./build.sh                 # build/hyp1-smk37.fwsc (SMK-37 Pro, Starrykey 37 Play)
BOARD=smk37 MODEL=mke ./build.sh       # build/hyp1-smk37-mke.fwsc
./build.sh                             # the FM-1 build again before tests/run_tests.sh (the tests use its files)
host/build_host.sh smk37               # the SMK-37 UI in the simulator: tests/scenarios/smk37/*.amb
```

`.github/workflows/smk37.yml` builds the three packages on every push to the `smk37` branch, checks that the
FM-1 build is byte for byte the base's, runs the tests, and keeps the packages as the run's artifacts
(they expire). A tag `smk37-vX.Y.Z` also publishes them as a GitHub Release, the permanent downloads.

## Install

**First, a full flash backup.** The SMK-37 keeps data unique to your unit in its flash (the factory key, pad
and wheel calibration at `0xC1000`, the Bluetooth pairing, the chip's key data). HYP-1 never writes there (an
allow-list and a second guard in the flash driver), but back it up anyway, before anything is installed (the
kit's method A, [fm1-to-smk37 docs/02, section 3](https://github.com/genkigenki/fm1-to-smk37/blob/main/docs/02-update-and-recovery.md)):

```sh
python3 tools/fm1_install.py --backup hyp1-smk37-elite.fwsc   # the check only: the update loader starts, nothing
                                                               # is written; then the ROM bootloader (a USB disk)
# then, as it prints: jl-uboot-tool reads the 1 MiB flash twice
python3 tools/smk_dump_check.py smk37-dump-1.bin smk37-dump-2.bin   # same twice, "trimdat" at 0xC1224
```

Switch the keyboard off and on afterwards: it comes back in update mode, and the install below finishes it.

Then, with the keyboard connected by USB:

```sh
pip3 install mido python-rtmidi
python3 tools/fm1_install.py --info                       # what is running now
python3 tools/fm1_install.py hyp1-smk37-elite.fwsc        # the package for your model
```

Or the browser installer for SMK-37 ports: <https://genkigenki.github.io/smk37/>.

The first start clears the stock firmware's settings area (its presets, not the calibration). Under HYP-1 the
keyboard shows up over USB as **HYP-1 SMK-37**.

## Going back to the stock firmware

Install M-Vave's factory package for your model (`SMK-37 Elite` V016, from <https://www.m-vave.com/download>)
with M-Vave's updater, M-UPGRADE. It works directly, because the installed package keeps the keyboard's model
name, and the calibration was never touched.

## Credits

The SMK-37 board layer (input, display, pad LEDs, flash map, update path, package layout, installers) is the
reference port's by M. Geisler (genkigenki), as carried into AMB-1's SMK-37 build
([genkigenki/fm1-amb](https://github.com/genkigenki/fm1-amb), branch `smk37`), with the hardware facts of the
[fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37) kit; Felucca by Leo Kuroshita (Hügelton Instruments)
underneath, AMB-1 by Charles Vestal, HYP-1 by RedtailWorks. "M-Vave", "SMK-37" and "MKE-P37" belong to their
owners; HYP-1 is not made or endorsed by them. Installing is at your own risk.
