#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 History Of Sky (M-PYRE)
"""M-PYRE demo songs, rock to metal: each a 4-bar loop on the four tracks (rhythm guitar, a second part, bass,
drums), written here as text and turned into build/demo/songs.h for tests/demo_songs.c (which builds them as
projects - .mpyre files the emulator's SONG LADEN and the FM-1 open - and renders a WAV of each).

  python3 tools/demo_songs.py && cc ... tests/demo_songs.c && build/host/demo_songs build/demo

A track's steps: one token a step, separated by spaces, '|' ignored (bars): 'E2' a note, 'E2+B2' a chord (POLY),
'-' the note goes on (a tie), '.' a rest; 'p' in front a palm mute (the step's PALM lock), '!' after it an
accent. Drums: a string a lane, one character a step: 'x' a hit, 'X' hard, 'g' ghost, '.' none."""
import re
import sys
from pathlib import Path

NOTE = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}
LANE = {"K": 0, "S": 1, "G": 2, "H": 3, "O": 4, "P": 5, "R": 6, "B": 7, "T1": 8, "T2": 9, "T3": 10, "C": 11,
        "C2": 12, "CH": 13}


def midi(n):
    m = re.fullmatch(r"([A-G]#?)(-?\d)", n)
    return 12 * (int(m.group(2)) + 1) + NOTE[m.group(1)]


def bars(*b):
    return " ".join(b)


def rep(s, k):
    return " ".join([s] * k)


SONGS = [
    dict(name="HIGHWAY", style="Hard Rock", bpm=126, kit=0, dly=None, rev=(2, 60, 70),
         tracks=[
             dict(preset="CRUNCH GTR", set={"P_CHORD": 2, "P_SGATE": 112, "P_REV": 20},
                  steps=bars("A2 - - - . . A2 - G2 - - - D3 - - -", "A2 - - - . . A2 - G2 - D3 - - - . .",
                             "A2 - - - . . A2 - G2 - - - D3 - - -", "E2 - - - - - . . E2 - E2 - E2 - E2 -")),
             dict(preset="LEAD GTR", set={"P_REV": 30},
                  steps=bars(rep(".", 16), rep(".", 16), rep(".", 16),
                             "A4 - C5 - D5 - - - E5 - D5 - C5 - A4 -")),
             dict(preset="PICK BASS", set={"P_SGATE": 110},
                  steps=bars("A1 - A1 - A1 - A1 - G1 - G1 - D2 - D2 -", "A1 - A1 - A1 - A1 - G1 - D2 - D2 - D2 -",
                             "A1 - A1 - A1 - A1 - G1 - G1 - D2 - D2 -", "E1 - E1 - E1 - E1 - E1 - E1 - E1 - E1 -")),
         ],
         drums={"K": "x.......x.x....." * 3 + "x.......x.x.x...",
                "S": "....x.......x..." * 3 + "....x.......xxxx",
                "H": ".." + "x.x.x.x.x.x.x." + ("x.x.x.x.x.x.x.x." * 2) + "x.x.x.x.x.......",
                "C": "x..............." + "." * 48}),
    dict(name="BURN IT DOWN", style="Punk Rock", bpm=178, kit=0, dly=None, rev=(1, 40, 70), div=1, slen=32,
         tracks=[
             dict(preset="HI-GAIN", set={"P_CHORD": 1, "P_SGATE": 100},
                  steps=bars(rep("E2", 8), rep("C3", 8), rep("G2", 8), "D3 D3 D3 D3 D3 D3 D3! -")),
             dict(preset="LEAD GTR", set={"P_REV": 20},
                  steps=bars(rep(".", 16), "B4 - - - G4 - A4 - B4 - - - D5 - - -")),
             dict(preset="DRIVE BASS", set={"P_SGATE": 100},
                  steps=bars(rep("E1", 8), rep("C2", 8), rep("G1", 8), "D2 D2 D2 D2 D2 D2 D2 -")),
         ],
         drums={"K": "x.x.x.x." * 4, "S": ".x.x.x.x" * 3 + ".x.xxxxX", "H": "x.x.x.x." * 4,
                "C": "x......." * 4}),
    dict(name="DESERT SUN", style="Stoner Rock", bpm=84, kit=0, dly=None, rev=(3, 70, 50),
         tracks=[
             dict(preset="CRUNCH GTR", set={"P_CHORD": 1, "P_PEDAL": 4, "P_PDRV": 96, "P_TONE": 50, "P_REV": 25},
                  steps=bars("D2 - - - D2 - F2 - G2 - - - F2 - D2 -", "C2 - - - C2 - D#2 - D2 - - - - - . .",
                             "D2 - - - D2 - F2 - G2 - - - F2 - D2 -", "A#1 - - - A#1 - C2 - D2 - - - - - - -")),
             dict(preset="LEAD GTR", set={"P_REV": 40, "P_VIB": 50},
                  steps=bars(rep(".", 16), rep(".", 16), rep(".", 16), "D5 - - - F5 - - - G5 - - - - - - -")),
             dict(preset="HEAVY BASS", set={"P_SGATE": 120},
                  steps=bars("D1 - - - D1 - F1 - G1 - - - F1 - D1 -", "C1 - - - C1 - D#1 - D1 - - - - - . .",
                             "D1 - - - D1 - F1 - G1 - - - F1 - D1 -", "A#0 - - - A#0 - C1 - D1 - - - - - - -")),
         ],
         drums={"K": "x.....x.x......." * 4, "S": "........x......." * 3 + "........x...x.xx",
                "R": "x.x.x.x.x.x.x.x." * 4, "C": "x..............." + "." * 32 + "x..............."}),
    dict(name="IRON STEED", style="Heavy Metal", bpm=168, kit=0, dly=(2, 40, 60), rev=(2, 60, 70),
         tracks=[
             dict(preset="HI-GAIN", set={"P_CHORD": 1, "P_SGATE": 100},
                  steps=bars(rep("pE2 - pE2 pE2", 4), rep("pE2 - pE2 pE2", 4),
                             rep("pC3 - pC3 pC3", 2) + " " + rep("pD3 - pD3 pD3", 2),
                             rep("pE2 - pE2 pE2", 2) + " E2! - - - - - - -")),
             dict(preset="LEAD GTR", set={"P_DLY": 34, "P_REV": 26},
                  steps=bars("B4 - - - E5 - F#5 - G5 - - - F#5 - E5 -", "D5 - - - E5 - - - B4 - - - - - - -",
                             "C5 - - - E5 - G5 - D5 - - - F#5 - A5 -", "G5 - F#5 - E5 - - - - - - - - - - -")),
             dict(preset="DRIVE BASS", set={"P_SGATE": 100},
                  steps=bars(rep("E1 - E1 E1", 4), rep("E1 - E1 E1", 4), rep("C2 - C2 C2", 2) + " " + rep("D2 - D2 D2", 2),
                             rep("E1 - E1 E1", 2) + " E1 - - - - - - -")),
         ],
         drums={"K": "x.xxx.xxx.xxx.xx" * 3 + "x.xxx.xxx.......",
                "S": "....x.......x..." * 3 + "....x.......X...",
                "R": "x...x...x...x..." * 4, "C": "x..............." * 2 + "x..............." + "........x......."}),
    dict(name="SKULL THRASH", style="Thrash Metal", bpm=196, kit=0, dly=None, rev=(1, 40, 70),
         tracks=[
             dict(preset="HI-GAIN", set={"P_CHORD": 1, "P_SGATE": 100},
                  steps=bars("pE2 pE2 pE2 pE2 pE2 pE2 F2! - pE2 pE2 pE2 pE2 pE2 pE2 G2! -",
                             "pE2 pE2 pE2 pE2 pE2 pE2 F#2! - pE2 pE2 pE2 pE2 A#2! - A2! -",
                             "pE2 pE2 pE2 pE2 pE2 pE2 F2! - pE2 pE2 pE2 pE2 pE2 pE2 G2! -",
                             "pE2 pE2 pE2 pE2 G2! - F#2! - F2! - E2! - - - - -")),
             dict(preset="LEAD GTR", set={"P_REV": 20},
                  steps=bars(rep(".", 16), rep(".", 16), rep(".", 16), rep("E5", 12) + " - - - -")),
             dict(preset="DRIVE BASS", set={"P_SGATE": 100},
                  steps=bars("E1 E1 E1 E1 E1 E1 F1 - E1 E1 E1 E1 E1 E1 G1 -",
                             "E1 E1 E1 E1 E1 E1 F#1 - E1 E1 E1 E1 A#1 - A1 -",
                             "E1 E1 E1 E1 E1 E1 F1 - E1 E1 E1 E1 E1 E1 G1 -",
                             "E1 E1 E1 E1 G1 - F#1 - F1 - E1 - - - - -")),
         ],
         drums={"K": "x...x...x...x..." * 4, "S": "..x...x...x...x." * 3 + "..x...x...xxxxxx",
                "H": "x...x...x...x..." * 4, "C": "x..............." * 4}),
    dict(name="FRACTAL", style="Djent / Metalcore", bpm=132, kit=0, dly=(7, 50, 50), rev=(5, 90, 60),
         tracks=[
             dict(preset="DJENT GTR", set={"P_CHORD": 1, "P_SGATE": 90},
                  steps=bars("pA1 . . pA1 . . pA1 . pA1 pA1 . . pA1 . . .", "pA1 . pA1 . . pA1 . . C2! - - - A#1! - - -",
                             "pA1 . . pA1 . . pA1 . pA1 pA1 . . pA1 . . .", "pA1 pA1 . pA1 pA1 . pA1 pA1 D2! - - - C2! - - -")),
             dict(preset="CLEAN GTR", set={"P_ITYPE": 3, "P_IMIX": 70, "P_DLY": 40, "P_REV": 50, "P_TRANS": 0, "P_LEVEL": 90},
                  steps=bars("A4 . E5 . C5 . G5 . A4 . E5 . C5 . G5 .", "F4 . C5 . A4 . E5 . F4 . C5 . A4 . E5 .",
                             "A4 . E5 . C5 . G5 . A4 . E5 . C5 . G5 .", "D4 . A4 . F4 . C5 . E4 . B4 . G4 . D5 .")),
             dict(preset="HEAVY BASS", set={"P_SGATE": 90},
                  steps=bars("A0 . . A0 . . A0 . A0 A0 . . A0 . . .", "A0 . A0 . . A0 . . C1 - - - A#0 - - -",
                             "A0 . . A0 . . A0 . A0 A0 . . A0 . . .", "A0 A0 . A0 A0 . A0 A0 D1 - - - C1 - - -")),
         ],
         drums={"K": "x..x..x.xx..x..." + "x.x..x..x...x..." + "x..x..x.xx..x..." + "xx.xx.xxx...x...",
                "S": "........x......." * 4, "CH": "x...x...x...x..." * 4, "C": "x..............." + "." * 48}),
    dict(name="LAST LIGHT", style="Doom", bpm=64, kit=0, dly=None, rev=(5, 110, 50),
         tracks=[
             dict(preset="HI-GAIN", set={"P_CHORD": 2, "P_SGATE": 127, "P_REV": 30},
                  steps=bars("D2 - - - - - - - - - - - F2 - - -", "E2 - - - - - - - C2 - - - - - - -",
                             "D2 - - - - - - - - - - - F2 - - -", "G2 - - - - - - - A2 - - - - - - -")),
             dict(preset="STRINGS", set={"P_REV": 60, "P_LEVEL": 92},
                  steps=bars("D4+F4+A4 " + rep("-", 15), "C4+E4+G4 " + rep("-", 15), "D4+F4+A4 " + rep("-", 15),
                             "A#3+D4+G4 " + rep("-", 7) + " A3+C#4+E4 " + rep("-", 7))),
             dict(preset="HEAVY BASS", set={"P_SGATE": 127},
                  steps=bars("D1 - - - - - - - - - - - F1 - - -", "E1 - - - - - - - C1 - - - - - - -",
                             "D1 - - - - - - - - - - - F1 - - -", "G1 - - - - - - - A1 - - - - - - -")),
         ],
         drums={"K": "x.......x.x....." * 3 + "x.......x.......",
                "S": "........x......." * 3 + "........x.......",
                "R": "x...x...x...x..." * 3 + "x...x...........",
                "T1": "." * 60 + "x...", "T2": "." * 52 + "....x.x.....", "T3": "." * 56 + "..x.x...",
                "C": "x..............." + "." * 48}),
    dict(name="AFTERGLOW", style="Rock Ballad", bpm=76, kit=0, dly=(7, 30, 50), rev=(2, 80, 70),
         tracks=[
             dict(preset="CLEAN GTR", set={"P_ITYPE": 3, "P_IMIX": 60, "P_DLY": 25, "P_REV": 40},
                  steps=bars("E3 . B3 . E4 . G4 . B3 . E4 . G4 . B3 .", "C3 . G3 . C4 . E4 . G3 . C4 . E4 . G3 .",
                             "G2 . D3 . G3 . B3 . D3 . G3 . B3 . D3 .", "D3 . A3 . D4 . F#4 . A3 . D4 . F#4 . A3 .")),
             dict(preset="STRINGS", set={"P_REV": 50, "P_LEVEL": 92},
                  steps=bars("E4+G4+B4 " + rep("-", 15), "E4+G4+C5 " + rep("-", 15), "D4+G4+B4 " + rep("-", 15),
                             "D4+F#4+A4 " + rep("-", 15))),
             dict(preset="PICK BASS", set={"P_SGATE": 120},
                  steps=bars("E1 - - - - - - - E1 - - - B1 - - -", "C2 - - - - - - - C2 - - - G1 - - -",
                             "G1 - - - - - - - G1 - - - D2 - - -", "D2 - - - - - - - D2 - - - A1 - - -")),
         ],
         drums={"K": "x.......x.x....." * 4, "S": "....x.......x..." * 4, "G": "..........g....." * 4,
                "R": "x.x.x.x.x.x.x.x." * 4}),
]


def track_steps(spec, slen):
    toks = [t for t in spec.split() if t != "|"]
    assert len(toks) == slen, (len(toks), slen, spec[:40])
    out = []
    for t in toks:
        if t == "-":
            out.append((1, [], 0, 0))
        elif t == ".":
            out.append((2, [], 0, 0))
        else:
            palm = t.startswith("p")
            acc = t.endswith("!")
            t = t.lstrip("p").rstrip("!")
            out.append((0, [midi(n) for n in t.split("+")], int(acc), int(palm)))
    return out


def main(out):
    L = ["/* generated by tools/demo_songs.py */", "typedef struct { uint8_t time, n, note[4], acc, palm; } dm_step_t;",
         "typedef struct { const char *preset; uint8_t npar; int16_t par[8][2]; dm_step_t st[64]; } dm_track_t;",
         "typedef struct { const char *name, *style; uint8_t bpm, kit, slen, div; int8_t dly[3], rev[3]; dm_track_t t[3];"
         " uint16_t drum[64], hard[64], ghost[64]; } dm_song_t;", f"#define DM_N {len(SONGS)}",
         "static const dm_song_t DM_SONGS[DM_N] = {"]
    for s in SONGS:
        slen = s.get("slen", 64)
        dl = s["dly"] or (0, 0, 0)
        rv = s["rev"] or (0, 0, 0)
        ts = []
        for t in s["tracks"]:
            par = ", ".join(f"{{{k}, {v}}}" for k, v in t["set"].items())
            st = ", ".join(f"{{{a}, {len(n)}, {{{', '.join(map(str, n + [0] * (4 - len(n))))}}}, {c}, {p}}}"
                           for a, n, c, p in track_steps(t["steps"], slen))
            ts.append(f"{{\"{t['preset']}\", {len(t['set'])}, {{{par}}}, {{{st}}}}}")
        dr, hd, gh = [0] * slen, [0] * slen, [0] * slen
        for lane, pat in s["drums"].items():
            assert len(pat) == slen, (s["name"], lane, len(pat))
            for i, ch in enumerate(pat):
                if ch in "xXg":
                    dr[i] |= 1 << LANE[lane]
                if ch == "X":
                    hd[i] |= 1 << LANE[lane]
                if ch == "g":
                    gh[i] |= 1 << LANE[lane]
        L.append(f"    {{\"{s['name']}\", \"{s['style']}\", {s['bpm']}, {s['kit']}, {slen}, {s.get('div', 2)}, "
                 f"{{{dl[0]}, {dl[1]}, {dl[2]}}}, {{{rv[0]}, {rv[1]}, {rv[2]}}}, {{{', '.join(ts)}}}, "
                 f"{{{', '.join(map(str, dr))}}}, {{{', '.join(map(str, hd))}}}, {{{', '.join(map(str, gh))}}}}},")
    L.append("};")
    Path(out).parent.mkdir(parents=True, exist_ok=True)
    Path(out).write_text("\n".join(L) + "\n")
    print(f"demo songs: {len(SONGS)} -> {out}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "build/demo/songs.h")
