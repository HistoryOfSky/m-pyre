#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 History Of Sky (M-PYRE)
"""M-PYRE: the amps' fixed filters and the cabinets (impulse responses) -> build/gen/mpyre_amp.h

  tools/gen_mpyre_amp.py OUT.h

Biquads as Q28 coefficients {b0, b1, b2, a1, a2} (RBJ cookbook), for 44.1 kHz (AMP_FS) and for the amps' 2x
oversampled stages (88.2 kHz). The cabinets: History Of Sky's IRs (assets/mpyre-src/ir, CLEAN CRUNCH HIGAIN LEAD),
resampled to 22.05 kHz (the cabinet runs at half rate: a guitar speaker passes next to nothing above 8 kHz), cut to
CAB_TAPS (11.6 ms: the box; the rest is room) with a fade, normalised to unity power gain over the guitar band (pink), Q15. Without the IRs
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
CABF_TAPS = 64   # M-PYRE 1.0.1: the full-rate cabinet (HI-GAIN, LEAD), 1.45 ms: a quarter of the cost of 256, the eq refitted
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
#   mid[3]   biquads between the stages, at 88.2 kHz (the third: before the power stage, as the reference)
#   g2, g3   stage 2 and the power stage gains (x 1/16; 0: none)
#   post[3]  the fixed voicing after the stages, 44.1 kHz
#   cab      the cabinet (CAB_NAMES index)
AMPS = [
    dict(name="CLEAN", pre=[NONE, NONE, NONE], g1=24, b1=0, mid=[NONE, NONE, NONE], g2=0, g3=0,
         post=[biquad("lowshelf", 150, FS, 0.7, 2), biquad("peak", 500, FS, 0.8, -3), biquad("highshelf", 2500, FS, 0.7, 3)],
         cab=0),
    dict(name="CRUNCH", pre=[biquad("hp", 90, FS, 0.7), biquad("peak", 900, FS, 0.9, 5), biquad("lp", 6000, FS, 0.7)], g1=224, b1=3932,
         mid=[biquad("lp", 9000, 2 * FS), biquad("hp", 40, 2 * FS), NONE], g2=40, g3=0,
         post=[biquad("lowshelf", 120, FS, 0.7, 3), biquad("peak", 1000, FS, 0.8, 2), biquad("highshelf", 2200, FS, 0.7, 5)],
         cab=1),
    # M-PYRE 1.0: HI-GAIN and LEAD fitted to History Of Sky's own guitar (Soundcheck_Riff / _Lead: the band's DI and the
    # same DI through their amp and IR 3 / IR 4) - the DI through these stages, frame by frame, as the recording: a hotter
    # first stage, a tighter low cut (~200 / 245 Hz), the mids pushed at ~1.3 kHz before the clipping, the highs kept
    # into it (was a 4.5 kHz low-pass); after the stages a full-rate cabinet (CABF_IR) and the voicing (post + eq)
    dict(name="HI-GAIN", pre=[biquad("lp", 12668, FS, 0.7), biquad("hp", 198, FS, 0.7), biquad("peak", 1270, FS, 0.8, 13.88)],
         g1=1578, b1=2367, mid=[biquad("hp", 60, 2 * FS), biquad("lp", 17392, 2 * FS), biquad("lp", 19131, 2 * FS)], g2=33, g3=127,
         post=[biquad("hp", 95, FS, 0.7), biquad("peak", 80, FS, 0.9, -0.41), biquad("peak", 200, FS, 0.9, -5.43)],
         # (1.0.1: the cabinet cut to CABF_TAPS; the eq fitted again so that the two together match the 256-tap cabinet
         # with the first fit's eq, 70 Hz .. 12 kHz, 1/6 octave: 0.5 dB rms, the low end the short IR loses at ~160 Hz)
         eq=[biquad("peak", 164.76, FS, 0.8, 9.18), biquad("peak", 548.72, FS, 3.0, -6.33), biquad("peak", 1324.05, FS, 0.75, -2.65),
             biquad("peak", 5385.34, FS, 0.73, 2.43), biquad("highshelf", 9945.42, FS, 1.48, -0.42)],
         cab=2, full=0),
    dict(name="LEAD", pre=[biquad("lp", 13203, FS, 0.7), biquad("hp", 244, FS, 0.7), biquad("peak", 1396, FS, 0.8, 12.92)],
         g1=815, b1=3764, mid=[biquad("hp", 60, 2 * FS), biquad("lp", 5804, 2 * FS), biquad("lp", 6384, 2 * FS)], g2=55, g3=59,
         post=[biquad("hp", 70, FS, 0.7), biquad("peak", 80, FS, 0.9, 6.96), biquad("peak", 200, FS, 0.9, -2.44)],
         eq=[biquad("peak", 158.59, FS, 0.84, 8.74), biquad("peak", 540.48, FS, 3.0, -7.16), biquad("peak", 1402.09, FS, 0.53, -4.61),
             biquad("peak", 4362.85, FS, 1.17, 1.28), biquad("highshelf", 8218.14, FS, 0.57, 9.49)],
         cab=3, full=1),
]
# the knobs' bands (parallel: y = x + gain * band(x)): BASS, MID, TREBLE, PRESENCE
BANDS = [biquad("lp", 120, FS, 0.6), biquad("bp", 700, FS, 0.8), biquad("hp", 2200, FS, 0.6), biquad("bp", 4000, FS, 0.9)]
# the half-rate cabinet: its anti-alias low-pass before taking every other sample
CAB_AA = biquad("lp", 9500, FS, 0.6)
# M-PYRE: after the cabinet, what the half rate took (the interpolation back to 44.1 kHz and the two image low-passes:
# ~-8 dB at 8 kHz) given back, and the low mids a touch fuller - the guitar's long-term spectrum set against the
# band's own recording of it through their amp (Samples_Distortion_Guitar: our render 9 .. 14 dB short over 6 kHz,
# ~4 dB at 250 Hz)
CAB_HS = biquad("highshelf", 5000, FS, 0.7, 10)
CAB_LM = biquad("peak", 260, FS, 0.9, 3)
# 88.2 -> 44.1 kHz after the stages: a 4th-order Butterworth at 10.5 kHz (two biquads; the reference resampled to 22.05)
AMP_DEC = [biquad("lp", 10500, 2 * FS, 0.5412), biquad("lp", 10500, 2 * FS, 1.3066)]
# (M-PYRE 1.0: HI-GAIN and LEAD keep more: 14 kHz)
AMP_DECF = [biquad("lp", 14000, 2 * FS, 0.5412), biquad("lp", 14000, 2 * FS, 1.3066)]
CAB_NAMES = ["CLEAN", "CRUNCH", "HIGAIN", "LEAD", "BASS"]
# the bass amp (amp.c track_bass): the low end split off clean, only the mids and highs driven (in parallel):
#   low: two low-passes at 220 Hz; drive: two high-passes at 250 Hz, +4 dB at 1.2 kHz, tanh (x GAIN), -4 dB at
#   500 Hz, two low-passes at 4.5 / 5.5 kHz; low + BLEND x drive
BASS_BQ = {
    "BD_LO": biquad("lp", 220, FS, 0.7), "BD_HI": biquad("hp", 250, FS, 0.7), "BD_PK1": biquad("peak", 1200, FS, 0.8, 4),
    "BD_PK2": biquad("peak", 500, FS, 0.9, -4), "BD_LP1": biquad("lp", 4500, FS, 0.7), "BD_LP2": biquad("lp", 5500, FS, 0.6),
    "B_HP30": biquad("hp", 42, FS, 0.7),       # (M-PYRE: 42 Hz, was 30: the sub out from under the kick)
    "B_LS": biquad("lowshelf", 110, FS, 0.7, -3),   # (and the boom a little down: room for the kick)
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
    """22.05 kHz, CAB_TAPS with a fade over the last quarter, unity power gain over 100 Hz .. 5 kHz (pink) -> Q15"""
    h = resample_poly(h, 147, 320)[:CAB_TAPS].copy()
    h = np.pad(h, (0, max(0, CAB_TAPS - len(h))))
    q = CAB_TAPS // 4
    h[-q:] *= np.linspace(1, 0, q) ** 2
    H = np.abs(np.fft.rfft(h, 4096)); fr = np.fft.rfftfreq(4096, 1 / CAB_FS)
    band = (fr >= 100) & (fr <= 5000)                   # (the power gain over a guitar's band, each octave alike:
    h /= np.sqrt(np.mean(H[band] ** 2 / fr[band]) / np.mean(1 / fr[band]))   # every cabinet as loud as the others)
    if np.abs(h).max() > 0.999:                         # (a tall first tap: the whole IR down, the level comes back
        h *= 0.999 / np.abs(h).max()                    # in the make-up gain)
    return [int(round(v * 32767)) for v in h]


def full_irs():
    """M-PYRE 1.0: the HI-GAIN and LEAD cabinets at 44.1 kHz, 256 taps (5.8 ms, the last eighth faded), one scale for
    both (the loudest tap at full scale; the make-up gain, amp.c MK, sets the level) -> Q15; generic without the IRs"""
    hs = []
    for f in ("High_Gain_Amp_IR.wav", "Lead_Amp_IR.wav"):
        if (IRDIR / f).exists():
            h = resample_poly(load_ir(IRDIR / f), 147, 160)[:256].copy()
            h = np.pad(h, (0, max(0, 256 - len(h))))
        else:
            h = np.zeros(256)
            h[:CAB_TAPS // 2] = resample_poly(np.array(generic_cab(GENERIC_4X12), float) / 32767, 2, 1)[:CAB_TAPS // 2]
        h *= np.r_[np.ones(224), np.hanning(64)[32:]]
        hs.append(h[:CABF_TAPS] * np.r_[np.ones(CABF_TAPS - 12), np.hanning(24)[12:]])   # (1.0.1: the first CABF_TAPS)
    mx = max(np.abs(h).max() for h in hs)
    return [[int(round(v / mx * 32767)) for v in h] for h in hs]


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
    bass_ir = IRDIR / "Bass_Amp_IR.wav"
    cabs.append(shape_ir(load_ir(bass_ir)) if bass_ir.exists() else generic_cab(BASS_8X10))   # (the band's own bass box)
    L = ["/* generated by tools/gen_mpyre_amp.py: M-PYRE amps and cabinets */", "#pragma once", "#include <stdint.h>",
         f"#define AMP_NMODELS {len(AMPS)}", f"#define CAB_TAPS {CAB_TAPS}", f"#define CABF_TAPS {CABF_TAPS}", f"#define CAB_N {len(cabs)}",
         "typedef struct { int32_t c[5]; } bq_coef_t;",
         "typedef struct { bq_coef_t pre[3], mid[3], post[3], eq[5]; int16_t g1, b1, g2, g3; uint8_t cab, full; } amp_model_t;",
         "static const amp_model_t AMP_MODELS[AMP_NMODELS] = {"]

    def bq(c):
        return "{{" + ", ".join(map(str, c)) + "}}"
    for m in AMPS:
        L.append(f"    {{ /* {m['name']} */ {{{', '.join(bq(c) for c in m['pre'])}}}, {{{', '.join(bq(c) for c in m['mid'])}}}, "
                 f"{{{', '.join(bq(c) for c in m['post'])}}}, {{{', '.join(bq(c) for c in m.get('eq', [NONE] * 5))}}}, "
                 f"{m['g1']}, {m['b1']}, {m['g2']}, {m['g3']}, {m['cab']}, {m.get('full', 255)}}},")
    L.append("};")
    L.append("static const bq_coef_t AMP_BANDS[4] = {" + ", ".join(bq(c) for c in BANDS) + "};")
    L.append(f"static const bq_coef_t CAB_AA = {bq(CAB_AA)};")
    L.append(f"static const bq_coef_t CAB_HS = {bq(CAB_HS)};")
    L.append(f"static const bq_coef_t CAB_LM = {bq(CAB_LM)};")
    L.append("static const bq_coef_t AMP_DEC[2] = {" + ", ".join(bq(c) for c in AMP_DEC) + "};")
    L.append("static const bq_coef_t AMP_DECF[2] = {" + ", ".join(bq(c) for c in AMP_DECF) + "};")
    L.append("static const int16_t CABF_IR[2][CABF_TAPS] = {" + ", ".join("{" + ",".join(map(str, h)) + "}" for h in full_irs()) + "};")
    # the wah, as a Cry Baby: a resonant band-pass from ~450 Hz (heel) to ~1.8 kHz (toe), its Q from ~6.5 at the heel
    # to ~4 at the toe; the rocker's audio-taper pot: the sweep slow at the heel, quick toward the toe (t^1.35)
    L.append("static const bq_coef_t WAH_BQ[64] = {" + ", ".join(
        bq(biquad("bp", 450 * (1800 / 450) ** ((i / 63) ** 1.35), FS, 6.5 - 2.5 * i / 63)) for i in range(64)) + "};")
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
