#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""M-PYRE: the sound pack, the four user sample slots as M-PYRE ships them (from build/mpyre/sets.json).

  tools/mpyre_slots.py [OUT_DIR]       (default build/mpyre) -> slot1..4.hdr / .bin, soundpack.bin

  USR1  GUITAR   the DI guitar, A1 .. G3 (6 notes)    \\ one set, GUITAR (eng_sample.c SMP_GTR): the zones of
  USR2  GUITAR2  the DI guitar, B3 .. A5 (5 notes)    /  both slots, their key ranges split over all 11 notes
  USR3  PALM     the palm mutes, A1 .. D4 (8 notes)
  USR4  BASS     the bass, B0 .. C4 (8 notes)

A slot is eng_sample.c's smp_user_hdr_t (magic, version 1, zones as smp_zone_t: offsets into the slot's data) and IMA
ADPCM data. Unlike sampleio.user_slot, the zones keep their loops (the loop's decoder state in pred / idx) and the
flags byte says SMP_DEEMPH (0x80): the samples are stored pre-emphasised (tools/prep_mpyre.py).
soundpack.bin: the four slots back to back, each padded to SLOT_SIZE (what the installer writes).
"""
import base64
import json
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sampleio import SLOT_DATA_OFF, SLOT_HDR_LEN, SLOT_MAX_DATA, SLOT_RATE, SLOT_SIZE, ima_encode, key_split  # noqa: E402

SRC = Path(__file__).resolve().parents[1]
SMP_DEEMPH = 0x80


def slot(name, zones):
    """zones: dicts with pcm (int16 list), root, loop, deemph, lo, hi -> (header, data)"""
    data, zb = bytearray(), b""
    rate = int(round(SLOT_RATE / 44100 * 65536))
    for z in zones:
        s = z["pcm"]
        ls, le = z["loop"] if z["loop"] else (len(s) - 1, len(s) - 1)
        adp, st = ima_encode(s, ls)
        off = len(data)
        data += adp
        if len(data) & 1:
            data.append(0)
        flags = (1 if z["loop"] else 0) | (SMP_DEEMPH if z["deemph"] else 0)
        zb += struct.pack("<5I2h4B", off, len(s), ls, le, rate, z["root"] * 16, st[0], st[1], z["lo"], z["hi"], flags)
    if len(data) > SLOT_MAX_DATA:
        raise SystemExit(f"mpyre_slots: {name}: {len(data)} B of ADPCM, a slot holds {SLOT_MAX_DATA} B")
    nm = name.encode()[:8].ljust(8, b"\0")
    hdr = struct.pack("<IHBB8sII8x", 0x504D5346, 1, len(zones), 0, nm, len(data), zlib.crc32(data)) + zb
    return hdr.ljust(SLOT_HDR_LEN, b"\0"), bytes(data)


def load_set(js, name):
    out = []
    for z in js[name]:
        raw = base64.b64decode(z["pcm"])
        out.append(dict(z, pcm=list(struct.unpack(f"<{len(raw) // 2}h", raw))))
    for z, (lo, hi) in zip(out, key_split([z["root"] for z in out])):   # the whole set's ranges
        z["lo"], z["hi"] = lo, hi
    return out


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else SRC / "build" / "mpyre"
    js = json.loads((out / "sets.json").read_text())
    gtr = load_set(js, "GUITAR")
    slots = [("GUITAR", gtr[:6]), ("GUITAR2", gtr[6:]), ("PALM", load_set(js, "PALM")), ("BASS", load_set(js, "BASS"))]
    pack = bytearray()
    for k, (name, zones) in enumerate(slots, 1):
        hdr, data = slot(name, zones)
        (out / f"slot{k}.hdr").write_bytes(hdr)
        (out / f"slot{k}.bin").write_bytes(data)
        img = (hdr.ljust(SLOT_DATA_OFF, b"\0") + data).ljust(SLOT_SIZE, b"\xFF")
        pack += img
        print(f"mpyre slots: USR{k} {name:8} {len(zones):2} zones, {len(data):6d} B of {SLOT_MAX_DATA}")
    (out / "soundpack.bin").write_bytes(bytes(pack))


if __name__ == "__main__":
    main()
