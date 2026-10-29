#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check a full flash backup of the M-Vave SMK-37 (fm1-to-smk37 docs/02, 3.1 step 6): two reads of the 1 MiB
flash, the same, with the unit's calibration record ("trimdat" at 0xC1224).

  tools/smk_dump_check.py smk37-dump-1.bin smk37-dump-2.bin
"""
import hashlib
import sys


def main(a):
    if len(a) != 2:
        print(__doc__)
        return 2
    d = [open(f, "rb").read() for f in a]
    bad = []
    for f, x in zip(a, d):
        if len(x) != 1048576:
            bad.append(f"{f}: {len(x)} bytes, not 1048576")
    h = [hashlib.sha256(x).hexdigest() for x in d]
    if h[0] != h[1]:
        bad.append("the two reads differ: read again")
    if d[0][0xC1224:0xC1224 + 7] != b"trimdat":
        bad.append("no 'trimdat' at 0xC1224: this is not a good dump of an SMK-37, read again")
    if bad:
        print("NOT A GOOD BACKUP:\n  " + "\n  ".join(bad))
        return 1
    print(f"good backup: 1 MiB, both reads the same (sha256 {h[0][:16]}...), calibration record at 0xC1224")
    print("keep the two files in two places (a USB stick, the cloud)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
