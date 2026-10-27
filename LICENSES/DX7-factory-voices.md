# The factory DX7 voices (firmware/src/dsp/dx7_factory.inc)

`firmware/src/dsp/dx7_factory.inc` is `app/factory_dx7.inc` of M-VAVE's SMK37 OPEN V01
(<https://github.com/m-vave/smk37_open>, commit b2d6bcb), unchanged: the 128 factory voices of the M-VAVE FM-1
and SMK-37 (banks A-D), distributed by M-VAVE under the Apache License, Version 2.0 (full text:
`LICENSES/Apache-2.0-msfa.txt`). HYP-1 includes the file in its firmware as it is; its header keeps M-VAVE's
notice. The notices from M-VAVE's THIRD_PARTY.md, retained:

---

## Factory DX7 voice resource

`app/factory_dx7.inc` contains 128 DX7-format presets extracted from the [FM1 OPEN](https://github.com/m-vave/fm1_open) resource `FM-1_015.fwsc`. Its upstream resource notice is retained below. M-VAVE notices apply to contributions it owns or is authorized to license.

Modified by M-VAVE, 2026: retain all 128 names and positions; clamp twelve out-of-range values in eleven voices to standard DX7 limits. `app/factory_dx7_packed.inc` is a lossless packed form.

## FM model / Apache-2.0

Copyright 2012, 2013 Google Inc.

Copyright 2016-2025 Pascal Gauthier (Dexed msfa note-model changes).

Modified by M-VAVE, 2026: C port, fixed-point arithmetic, block processing, voice management and MIDI controls.
