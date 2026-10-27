#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""DX7 32-voice banks for HYP-1's FM voice (the SMK-37 build): check, make and send them.

  tools/dx7_bank.py info BANK.syx                 the 32 names, and whether the dump is a good DX7 bank
  tools/dx7_bank.py factory A|B|C|D OUT.syx       one of the factory banks (firmware/src/dsp/dx7_factory.inc) as a dump
  tools/dx7_bank.py send BANK.syx [PORT]          send it to the keyboard over USB MIDI (pip3 install mido python-rtmidi)

HYP-1 takes the bank as U01..U32 (FM page, Patch; and the pad's Sound past the factory ones) and stores it at
once. A DX7 32-voice bulk dump is F0 43 0n 09 20 00, 4096 bytes (32 packed voices of 128), a checksum, F7.
"""
import re
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]


def checksum(data):
    return (-sum(data)) & 0x7F


def make(voices):                    # 4096 bytes -> the dump
    assert len(voices) == 4096 and all(b < 128 for b in voices)
    return bytes([0xF0, 0x43, 0x00, 0x09, 0x20, 0x00]) + bytes(voices) + bytes([checksum(voices), 0xF7])


def parse(sx):                       # the dump -> 4096 bytes, or an error
    if len(sx) != 4104:
        raise ValueError(f"{len(sx)} bytes: a DX7 32-voice bank is 4104")
    if sx[:2] != b"\xF0\x43" or sx[2] & 0xF0 or sx[3:6] != b"\x09\x20\x00" or sx[-1] != 0xF7:
        raise ValueError("not a DX7 32-voice bulk dump (F0 43 0n 09 20 00 .. F7)")
    data = sx[6:4102]
    if any(b & 0x80 for b in data):
        raise ValueError("a data byte above 127")
    if checksum(data) != sx[4102]:
        raise ValueError("checksum wrong")
    return data


def names(data):
    return [bytes(c if 32 <= c <= 126 else 32 for c in data[i * 128 + 118:i * 128 + 128]).decode().rstrip()
            for i in range(32)]


def factory(bank):
    text = (HERE / "firmware/src/dsp/dx7_factory.inc").read_text()
    vals = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", text))
    k = "ABCD".index(bank.upper())
    return vals[k * 4096:(k + 1) * 4096]


def send(sx, port=None):
    import mido
    outs = mido.get_output_names()
    name = port or next((o for o in outs if re.search(r"hyp-1|smk", o, re.I)), None)
    if not name:
        raise SystemExit(f"no HYP-1 / SMK port among {outs}")
    with mido.open_output(name) as out:
        out.send(mido.Message("sysex", data=list(sx[1:-1])))
        time.sleep(0.5)
    print(f"sent {len(sx)} bytes to {name}")


def main(a):
    if len(a) >= 2 and a[0] == "info":
        data = parse(Path(a[1]).read_bytes())
        for i, n in enumerate(names(data)):
            print(f"U{i + 1:02d} {n}")
        return 0
    if len(a) == 3 and a[0] == "factory":
        Path(a[2]).write_bytes(make(factory(a[1])))
        print(f"{a[2]}: factory bank {a[1].upper()}")
        return 0
    if len(a) >= 2 and a[0] == "send":
        sx = Path(a[1]).read_bytes()
        parse(sx)
        send(sx, a[2] if len(a) > 2 else None)
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
