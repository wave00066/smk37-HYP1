#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""HYP-1's SMK-37 emulator as one page: web/emu/smk37/index.html with build/emu-smk37/amb.wasm in it as base64
(nothing to fetch but worklet.js beside it), and a plain-site copy with a document wrapper.

  sh web/emu/smk37/build.sh && python3 web/emu/smk37/make_artifact.py
  -> build/artifact-smk37/{index.html, worklet.js}   (a claude.ai Artifact: the host adds the document wrapper)
  -> build/emu-smk37/index.html                       (a static site: fetches amb.wasm)
"""
import base64
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SRC, EMU, OUT = ROOT / "web/emu/smk37", ROOT / "build/emu-smk37", ROOT / "build/artifact-smk37"
OUT.mkdir(parents=True, exist_ok=True)
page = (SRC / "index.html").read_text()
wasm = base64.b64encode((EMU / "amb.wasm").read_bytes()).decode()
art = page.replace('<script>\n"use strict";', f'<script>\n"use strict";\nconst HYP1_WASM = "{wasm}";', 1)
assert art != page
(OUT / "index.html").write_text(art)
(OUT / "worklet.js").write_text((SRC / "worklet.js").read_text())
site = ('<!doctype html>\n<html lang="it">\n<head>\n<meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n'
        + page.replace("<style>", "<style>\n:root{padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px)}body{margin:0}[hidden]{display:none!important}", 1)
        .replace("\n<div class=\"wrap\">", "\n</head>\n<body>\n<div class=\"wrap\">", 1) + "\n</body>\n</html>\n")
(EMU / "index.html").write_text(site)
(EMU / "worklet.js").write_text((SRC / "worklet.js").read_text())
print(f"artifact: {OUT / 'index.html'} ({len(art)} B) + worklet.js; site: {EMU / 'index.html'}")
