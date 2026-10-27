#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# HYP-1's SMK-37 build (the FM voice too) in the browser, with plain clang (wasm32) and wasm-ld, as
# web/emu/build_clang.sh builds the FM-1's:   sh web/emu/smk37/build.sh  ->  build/emu-smk37/{index.html, worklet.js, amb.wasm}
set -e
cd "$(dirname "$0")/../../.."
mkdir -p build/gen build/emu-smk37
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
clang --target=wasm32 -O2 -ffp-contract=off -std=gnu99 -nostdlib -ffreestanding -w \
    -DEN_HOST -DEN_WEB -DBOARD_SMK37=1 "-DEN_VERSION=\"$(printf %s "${EN_VERSION:-DEV}" | tr a-z A-Z)\"" \
    -Iweb/emu/shim -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -Wl,--no-entry -Wl,--export-dynamic -Wl,--initial-memory=33554432 -Wl,-z,stack-size=1048576 \
    -o build/emu-smk37/amb.wasm web/emu/amb_web.c firmware/src/dsp/amb.c web/emu/shim/libc.c
cp web/emu/smk37/index.html web/emu/smk37/worklet.js build/emu-smk37/ 2>/dev/null || true
echo "emu: build/emu-smk37 ($(wc -c < build/emu-smk37/amb.wasm) B wasm)"
