# HYP-1 licensing

HYP-1 (a RedtailWorks fork of Charles Vestal's AMB-1) is free software under the GNU General Public
License, version 3 only
(`GPL-3.0-only`, full text in `LICENSE`). It is built on Felucca's platform layer (through X0X and FoMni)
and keeps Felucca's licence. If you distribute HYP-1, or firmware derived from it, you must give
your recipients its complete corresponding source under the same licence.

## Where the code comes from

| What | Origin | Licence |
| --- | --- | --- |
| Platform: `firmware/hal/`, `firmware/loader/`, `firmware/src/{libc,lcd,gfx,usb,storage,ota,midi_uart}.c`, `tools/` (build, package, install, rescue), `web/fm1*.js` | [Felucca](https://github.com/hugelton/Felucca), Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments, as changed for [X0X](https://github.com/charlesvestal/fm1-x0x) and FoMni, then AMB-1 | GPL-3.0-only |
| `firmware/src/app/{panel,plat_fm1,main_fm1}.c`, `host/amb_host.c`, `web/emu/`, `tools/{make_pages,manual_html}.py`, `tools/publish_pages.sh` | adapted from X0X (itself from Felucca's `panel.c`, `main.c`, `audio.c`) | GPL-3.0-only |
| Dattorro's plate reverb in `firmware/src/dsp/amb.c` | from [FoMni](https://github.com/charlesvestal/fm1-fomni) | GPL-3.0-only |
| The idea of a browser emulator for FM-1 firmware (`web/emu/`) | [Groove OS](https://www.groove-os.com/) by Peter Gombos, whose site runs its firmware in the browser; no code from it | credit only |
| The FM voice's glue (`firmware/src/dsp/fm.c`, the DX7 bank tools `tools/dx7_bank.py`) and the SMK-37 UI (`firmware/src/app/ui_smk.c`) | the SMK-37 port of HYP-1 (`fm_unpack`/`fm_max` from Felucca's `eng_fm6.c`) | GPL-3.0-only |
| Everything else in `firmware/src/{app,dsp}`, `tests/` | AMB-1 (Charles Vestal), HYP-1 changes (RedtailWorks) | GPL-3.0-only |

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Chakra Petch (Cadson Demak), the screen's face and the site's headings | SIL OFL 1.1 | `assets/fonts/ChakraPetch-*.ttf`, `assets/fonts/ChakraPetch-OFL.txt` |
| Barlow Semi Condensed (The Barlow Project Authors), the site's text and labels | SIL OFL 1.1 | `assets/fonts/BarlowSemiCondensed-*.ttf`, `assets/fonts/Barlow-OFL.txt` |
| Terminus (Dimitar Toshkov Zhekov), an alternative font set | SIL OFL 1.1 | `assets/fonts/ter-u*.bdf`, `assets/fonts/Terminus-LICENSE.txt` |
| msfa by Google Inc. and Pascal Gauthier, from Dexed (<https://github.com/asb2m10/dexed>), as Felucca ported it to integer C (Leo Kuroshita, Hügelton Instruments): the FM voice's synthesis (the SMK-37 build) | Apache-2.0 | `firmware/src/dsp/fm6_core.c` (unchanged from Felucca), `LICENSES/Apache-2.0-msfa.txt`; its tables from `tools/gen_fm6_tables.py` (Felucca's generator, GPL-3.0-only) |
| The 128 factory DX7 voices of the M-VAVE FM-1 / SMK-37 (banks A-D), from M-VAVE's SMK37 OPEN (<https://github.com/m-vave/smk37_open>) | Apache-2.0 | `firmware/src/dsp/dx7_factory.inc` (unchanged), `LICENSES/DX7-factory-voices.md` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments.
"M-VAVE" and "FM-1" are trademarks of their respective owners. HYP-1 is independent firmware; it is
not affiliated with, endorsed by or supported by any of them.

## Radio

HYP-1 never enables the Bluetooth / Wi-Fi radio of the hardware.
