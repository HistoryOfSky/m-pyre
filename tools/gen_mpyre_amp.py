#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""M-PYRE: the amps' fixed filters and the cabinets (impulse responses) -> build/gen/mpyre_amp.h

  tools/gen_mpyre_amp.py OUT.h

Biquads as Q28 coefficients {b0, b1, b2, a1, a2} (RBJ cookbook), for 44.1 kHz (AMP_FS) and for the amps' 2x
oversampled stages (88.2 kHz). The cabinets: History Of Sky's IRs (assets/mpyre-src/ir, CLEAN CRUNCH HIGAIN LEAD),
resampled to 22.05 kHz (the cabinet runs at half rate: a guitar speaker passes next to nothing above 8 kHz), cut to
CAB_TAPS (11.6 ms: the box; the rest is room) with a fade, normalised to unity gain at 1 kHz, Q15. Without the IRs
(a public build), a generic 4x12 made of filters stands in. A bass cabinet (no IR): an 8x10's curve, made here.
"""
import math
import subprocess
import sys
from pathlib import Path

import numpy as np
from scipy.signal import firwin2, resample_poly

SRC = Path(__file__).resolve().parents[1]
IRDIR = SRC / "assets" / "mpyre-src" / "ir"
FS = 44100
CAB_FS = 22050
CAB_TAPS = 256   # 11.6 ms at 22.05 kHz: within ~2 dB of the full IR (1/3-octave smoothed), half the cost of 512
Q30 = 1 << 28   # (named Q30 for history: Q28, coefficients up to +-8)


def biquad(kind, f0, fs, q=0.707, gain_db=0.0):
    A = 10 ** (gain_db / 40)
    w = 2 * math.pi * f0 / fs
    c, s = math.cos(w), math.sin(w)
    al = s / (2 * q)
    if kind == "lp":
        b, a = [(1 - c) / 2, 1 - c, (1 - c) / 2], [1 + al, -2 * c, 1 - al]
    elif kind == "hp":
        b, a = [(1 + c) / 2, -(1 + c), (1 + c) / 2], [1 + al, -2 * c, 1 - al]
    elif kind == "bp":                                  # constant 0 dB peak gain
        b, a = [al, 0, -al], [1 + al, -2 * c, 1 - al]
    elif kind == "peak":
        b, a = [1 + al * A, -2 * c, 1 - al * A], [1 + al / A, -2 * c, 1 - al / A]
    elif kind == "lowshelf":
        sq = 2 * math.sqrt(A) * al
        b = [A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq)]
        a = [(A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq]
    elif kind == "highshelf":
        sq = 2 * math.sqrt(A) * al
        b = [A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq)]
        a = [(A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq]
    else:
        raise ValueError(kind)
    return [round(v / a[0] * Q30) for v in (b[0], b[1], b[2], a[1], a[2])]


NONE = [Q30, 0, 0, 0, 0]                                # a pass-through biquad

# the amps (as tested in the A/B renders, tools: scripts amp.py / amp_ir.py of the design): per model
#   pre[3]   biquads at 44.1 kHz before the gain stages (hiss cut, tightening high-pass, mid boost)
#   g1, b1   stage 1: gain (x 1/16) and bias (Q15) into tanh, at 88.2 kHz
#   mid[2]   biquads between the stages, at 88.2 kHz
#   g2, g3   stage 2 and the power stage gains (x 1/16; 0: none)
#   post[3]  the fixed voicing after the stages, 44.1 kHz
#   cab      the cabinet (CAB_NAMES index)
AMPS = [
    dict(name="CLEAN", pre=[NONE, NONE, NONE], g1=24, b1=0, mid=[NONE, NONE], g2=0, g3=0,
         post=[biquad("lowshelf", 150, FS, 0.7, 2), biquad("peak", 500, FS, 0.8, -3), biquad("highshelf", 2500, FS, 0.7, 3)],
         cab=0),
    dict(name="CRUNCH", pre=[biquad("hp", 90, FS, 0.7), biquad("peak", 900, FS, 0.9, 5), NONE], g1=224, b1=3932,
         mid=[biquad("lp", 9000, 2 * FS), biquad("hp", 40, 2 * FS)], g2=40, g3=0,
         post=[biquad("lowshelf", 120, FS, 0.7, 3), biquad("peak", 1000, FS, 0.8, 2), biquad("highshelf", 2200, FS, 0.7, 5)],
         cab=1),
    dict(name="HI-GAIN", pre=[biquad("lp", 4500, FS, 0.7), biquad("hp", 140, FS, 0.7), biquad("peak", 800, FS, 0.8, 9)],
         g1=400, b1=6554, mid=[biquad("hp", 60, 2 * FS), biquad("lp", 9000, 2 * FS)], g2=96, g3=48,
         post=[biquad("lowshelf", 100, FS, 0.7, 7), biquad("peak", 500, FS, 0.9, -6), biquad("highshelf", 2500, FS, 0.7, 4)],
         cab=2),
    dict(name="LEAD", pre=[biquad("lp", 4500, FS, 0.7), biquad("hp", 110, FS, 0.7), biquad("peak", 800, FS, 0.8, 9)],
         g1=400, b1=6554, mid=[biquad("hp", 60, 2 * FS), biquad("lp", 7000, 2 * FS)], g2=96, g3=48,
         post=[biquad("lowshelf", 100, FS, 0.7, 4), biquad("peak", 700, FS, 0.8, 3), biquad("peak", 3500, FS, 1.2, -4)],
         cab=3),
]
# the knobs' bands (parallel: y = x + gain * band(x)): BASS, MID, TREBLE, PRESENCE
BANDS = [biquad("lp", 120, FS, 0.6), biquad("bp", 700, FS, 0.8), biquad("hp", 2200, FS, 0.6), biquad("bp", 4000, FS, 0.9)]
# the half-rate cabinet: its anti-alias low-pass before taking every other sample
CAB_AA = biquad("lp", 9500, FS, 0.6)
CAB_NAMES = ["CLEAN", "CRUNCH", "HIGAIN", "LEAD", "BASS"]
# the bass amps (amp.c track_bass): the low end split off clean, only the mids and highs driven (in parallel)
#   B-CLEAN  the bass through the knobs' bands only (+ the bass cabinet)
#   B-DRIVE  low: two low-passes at 220 Hz; drive: two high-passes at 250 Hz, +4 dB at 1.2 kHz, tanh (x30), -4 dB at
#            500 Hz, two low-passes at 4.5 / 5.5 kHz; low + BLEND x drive
#   DJENT    low: two low-passes at 180 Hz, squeezed (tanh x2.5); drive: two high-passes at 350 Hz, +8 dB at 1.5 kHz,
#            tanh (x120), high-pass 150 Hz, tanh (x4), +4 dB at 800 Hz, -5 dB at 450 Hz, +5 dB at 2.5 kHz, low-passes
#            at 6 / 7 kHz
BASS_BQ = {
    "BD_LO": biquad("lp", 220, FS, 0.7), "BD_HI": biquad("hp", 250, FS, 0.7), "BD_PK1": biquad("peak", 1200, FS, 0.8, 4),
    "BD_PK2": biquad("peak", 500, FS, 0.9, -4), "BD_LP1": biquad("lp", 4500, FS, 0.7), "BD_LP2": biquad("lp", 5500, FS, 0.6),
    "DJ_LO": biquad("lp", 180, FS, 0.7), "DJ_HI": biquad("hp", 350, FS, 0.7), "DJ_PK1": biquad("peak", 1500, FS, 0.8, 8),
    "DJ_HP": biquad("hp", 150, 2 * FS, 0.7), "DJ_PK2": biquad("peak", 800, FS, 0.8, 4),
    "DJ_PK3": biquad("peak", 450, FS, 1.0, -5), "DJ_PK4": biquad("peak", 2500, FS, 1.0, 5),
    "DJ_LP1": biquad("lp", 6000, FS, 0.7), "DJ_LP2": biquad("lp", 7000, FS, 0.6), "B_HP30": biquad("hp", 30, FS, 0.7),
}
# in front of the amp (amp.c track_pre): TONE (left: bite at 1.8 kHz, right: treble from 3 kHz) and the pedals
#   BOOST  clean, + the highs from 700 Hz       OD    a tube screamer: the band over 720 Hz clipped softly, added
#   DIST   high-pass 60 Hz, hard clip, 3.5 kHz  FUZZ  two stages with a bias, the mids scooped (1 kHz)
PRE_BQ = {
    "TN_BITE": biquad("bp", 1800, FS, 1.0), "TN_TREB": biquad("hp", 3000, FS, 0.6),
    "PD_BOOST": biquad("hp", 700, FS, 0.6), "PD_ODHP": biquad("hp", 720, FS, 0.6), "PD_ODLP": biquad("lp", 4000, FS, 0.7),
    "PD_DSHP": biquad("hp", 60, FS, 0.7), "PD_DSLP": biquad("lp", 3500, FS, 0.7),
    "PD_FZLP": biquad("lp", 1000, FS, 0.5), "PD_FZHP": biquad("hp", 1000, FS, 0.5), "PD_FZAA": biquad("lp", 6000, FS, 0.7),
}
CAB_FILES = ["Clean_Amp_IR.wav", "Crunch_Amp_IR.wav", "High_Gain_Amp_IR.wav", "Lead_Amp_IR.wav"]


def load_ir(path):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(path), "-ac", "1", "-ar", "48000", "-f", "f32le", "-"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype="<f4").astype(np.float64)


def shape_ir(h):
    """22.05 kHz, CAB_TAPS with a fade over the last quarter, unity gain at 1 kHz -> Q15"""
    h = resample_poly(h, 147, 320)[:CAB_TAPS].copy()
    h = np.pad(h, (0, max(0, CAB_TAPS - len(h))))
    q = CAB_TAPS // 4
    h[-q:] *= np.linspace(1, 0, q) ** 2
    w = np.exp(-2j * np.pi * 1000 / CAB_FS * np.arange(CAB_TAPS))
    h /= abs(np.dot(h, w))
    if np.abs(h).max() > 0.999:                         # (a tall first tap: the whole IR down, the level comes back
        h *= 0.999 / np.abs(h).max()                    # in the make-up gain)
    return [int(round(v * 32767)) for v in h]


def generic_cab(points):
    """an IR from a magnitude curve [(Hz, dB)] (a minimum-phase-ish FIR; no recording needed)"""
    f = [0] + [p[0] for p in points] + [CAB_FS / 2]
    g = [10 ** (points[0][1] / 20)] + [10 ** (p[1] / 20) for p in points] + [10 ** (points[-1][1] / 20)]
    h = firwin2(127, f, g, fs=CAB_FS)
    spec = np.fft.rfft(h, 4096)                          # minimum phase (real cepstrum): the energy up front
    cep = np.fft.irfft(np.log(np.abs(spec) + 1e-9))
    cep[1:2048] *= 2
    cep[2049:] = 0
    hm = np.real(np.fft.irfft(np.exp(np.fft.rfft(cep))))[:CAB_TAPS]
    return shape_ir(resample_poly(np.pad(hm, (0, 0)), 320, 147))


GENERIC_4X12 = [(60, -18), (90, -3), (110, 2), (200, 0), (500, -1), (1000, 0), (2500, 3), (4000, 1), (5500, -6),
                (7000, -20), (9000, -36)]
BASS_8X10 = [(30, -12), (45, -2), (70, 2), (150, 0), (400, -2), (800, 0), (1500, 1), (3000, -3), (4500, -12),
             (6500, -30)]


def main(out):
    have = IRDIR.exists() and all((IRDIR / f).exists() for f in CAB_FILES)
    cabs = [shape_ir(load_ir(IRDIR / f)) if have else generic_cab(GENERIC_4X12) for f in CAB_FILES]
    cabs.append(generic_cab(BASS_8X10))
    L = ["/* generated by tools/gen_mpyre_amp.py: M-PYRE amps and cabinets */", "#pragma once", "#include <stdint.h>",
         f"#define AMP_NMODELS {len(AMPS)}", f"#define CAB_TAPS {CAB_TAPS}", f"#define CAB_N {len(cabs)}",
         "typedef struct { int32_t c[5]; } bq_coef_t;",
         "typedef struct { bq_coef_t pre[3], mid[2], post[3]; int16_t g1, b1, g2, g3; uint8_t cab; } amp_model_t;",
         "static const amp_model_t AMP_MODELS[AMP_NMODELS] = {"]

    def bq(c):
        return "{{" + ", ".join(map(str, c)) + "}}"
    for m in AMPS:
        L.append(f"    {{ /* {m['name']} */ {{{', '.join(bq(c) for c in m['pre'])}}}, {{{', '.join(bq(c) for c in m['mid'])}}}, "
                 f"{{{', '.join(bq(c) for c in m['post'])}}}, {m['g1']}, {m['b1']}, {m['g2']}, {m['g3']}, {m['cab']}}},")
    L.append("};")
    L.append("static const bq_coef_t AMP_BANDS[4] = {" + ", ".join(bq(c) for c in BANDS) + "};")
    L.append(f"static const bq_coef_t CAB_AA = {bq(CAB_AA)};")
    for k, c in list(BASS_BQ.items()) + list(PRE_BQ.items()):
        L.append(f"static const bq_coef_t {k} = {bq(c)};")
    L.append(f"static const int16_t CAB_IR[CAB_N][CAB_TAPS] = {{   /* {'History Of Sky IRs' if have else 'generic'} */")
    for c in cabs:
        L.append("    {" + ",".join(map(str, c)) + "},")
    L.append("};")
    L.append("static const char *const CAB_NAMES[CAB_N] = {" + ", ".join(f'"{n}"' for n in CAB_NAMES) + "};")
    # tanh over 0 .. 8 (Q15 in, x / 32768), 1024 + 1 points: the stages run far past tanh(2) (dsp.c softclip)
    L.append("static const int16_t AMP_TANH[1025] = {" +
             ",".join(str(int(round(32767 * math.tanh(i / 128)))) for i in range(1025)) + "};")
    Path(out).write_text("\n".join(L) + "\n")
    print(f"mpyre amp: {len(AMPS)} amps, {len(cabs)} cabinets ({'IRs' if have else 'generic'}), {CAB_TAPS} taps")


if __name__ == "__main__":
    main(sys.argv[1])
