#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 History Of Sky (M-PYRE)
"""M-PYRE: prepare the riffbox's own samples (History Of Sky) for the FM-1.

  tools/prep_mpyre.py [OUT_DIR]        (default build/mpyre; needs numpy, scipy and ffmpeg)

Sources (assets/mpyre-src, not in the public tree):
  drums/*.aif            one-shots, 14 sounds        -> the built-in kit (set PERC, the drum track's lanes)
  bass.aif               8 notes, B0..C4, 8 s each   -> the user slot USR4 (set BASS)
  guitar_di.aif          11 notes, A1..A5, DI        -> the user slots USR1 + USR2 (set GUITAR)
  palm_di.aif            8 palm mutes, A1..D4, DI    -> the user slot USR3 (set PALM)

Every melodic note is cut 2 ms before its attack, tuned to A440 (the pitch measured 0.2..1.0 s in), its pitch drift
after 0.15 s flattened (a string sinks as it rings: two notes of a chord from different samples would beat), cut to
KEEP seconds with a loop of LOOP seconds at the end, crossfaded linearly (the material on both sides is alike: an
equal-power fade would swell at every pass), loudness-matched inside its set and stored pre-emphasised
(x - 0.85 x[-1]) / 0.15: eng_sample.c's de-emphasis (SMP_DEEMPH) undoes it after the ADPCM decoder, and takes the
decoder's noise down ~10 dB with it.

Writes OUT_DIR/sets.json: {set: [{name, root, lo, hi, loop: [ls, le] | null, deemph, pcm (int16 base64)}]}.
"""
import base64
import json
import subprocess
import sys
from fractions import Fraction
from pathlib import Path

import numpy as np
from scipy.signal import lfilter, resample_poly

SRC = Path(__file__).resolve().parents[1]
ASSETS = SRC / "assets" / "mpyre-src"
TR = 22050
PE = 0.85
sys.path.insert(0, str(SRC / "tools"))
from sampleio import yin  # noqa: E402


def load(path, sr=48000):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(path), "-ac", "1", "-ar", str(sr), "-f", "f32le", "-"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype="<f4").astype(np.float64)


def k_loud(x, fs):
    """BS.1770-style loudness of the loudest 100 ms"""
    b1 = [1.53512485958697, -2.69169618940638, 1.19839281085285]
    a1 = [1, -1.69065929318241, 0.73248077421585]
    k = lfilter([1, -2, 1], [1, -1.99004745483398, 0.99007225036621], lfilter(b1, a1, x)) ** 2
    w = int(0.1 * fs)
    c = np.concatenate([[0], np.cumsum(k)])
    return 10 * np.log10(((c[w:] - c[:-w]) / w).max() + 1e-12) - 0.691


def segments(x, sr, n_expect):
    """note starts: the level rises over -30 dB (of the file's peak) after 0.25 s under -45 dB"""
    w = int(0.005 * sr)
    env = np.sqrt(np.convolve(x * x, np.ones(w) / w, "same"))
    pk = np.abs(x).max()
    out, quiet, armed = [], 0, True
    for j in range(0, len(env), 48):
        v = env[j]
        quiet = quiet + 48 if v < pk * 10 ** (-45 / 20) else 0
        if quiet > 0.25 * sr:
            armed = True
        if armed and v > pk * 10 ** (-30 / 20):
            out.append(j)
            armed = False
    if len(out) != n_expect:
        raise SystemExit(f"prep_mpyre: {n_expect} notes expected, {len(out)} found")
    return out


def note(x, sr, s, e, root, keep, loop_lo, loop_hi, flatten=True, pitch_at=(0.2, 1.0), onset=0.10):
    """one recorded note -> (float samples at TR, loop start, loop end)"""
    a = max(0, s - int(0.05 * sr))
    sub = x[a:e]
    sp = np.abs(sub).max()
    # the note starts where it is heard: a tenth of its peak (M-PYRE: was 2 %: the bass's slow pick took ~10 ms to come
    # up from there - played in unison it lagged the driven guitars, whose amps lift that quiet start at once), 1.5 ms
    # before it, faded in over 0.8 ms
    o = max(0, a + int(np.argmax(np.abs(sub) > sp * (0.12 if not flatten else onset))) - int(0.0015 * sr))
    target = 440 * 2 ** ((root - 69) / 12)
    hz = yin(x[o + int(pitch_at[0] * sr):o + int(pitch_at[1] * sr):2].tolist(), sr // 2, fmax=1500, fmin=25)
    if not hz or abs(1200 * np.log2(hz / target)) > 150:   # (a palm mute rings briefly: no pitch, or an octave off)
        hz = target
    fr = Fraction(TR * hz / (sr * target)).limit_denominator(1000)   # read at hz / target: in tune
    y = resample_poly(x[o:e], fr.numerator, fr.denominator)
    if flatten:                                       # the drift after 0.15 s, frame by frame
        hop, win = int(0.03 * TR), int(0.08 * TR)
        ts, cs = [], []
        for a in range(int(0.1 * TR), min(len(y) - win, int(3.0 * TR)), hop):
            h = yin(y[a:a + win].tolist(), TR, fmax=1500, fmin=25)
            if h:
                ts.append((a + win / 2) / TR)
                cs.append(1200 * np.log2(h / target))
        ts, cs = np.array(ts), np.array(cs)
        if len(cs) > 5:
            ok = np.abs(cs - np.median(cs)) < 30
            ts, cs = ts[ok], cs[ok]
            cs = np.convolve(np.pad(cs, 3, mode="edge"), np.ones(7) / 7, "valid")
            t = np.arange(len(y)) / TR
            c = np.interp(t, ts, cs)
            c[t < 0.15] = np.interp(0.15, ts, cs)
            pos = np.concatenate([[0], np.cumsum(2 ** (-c / 1200))[:-1]])
            pos = pos[pos < len(y) - 2]
            i0 = pos.astype(int)
            f = pos - i0
            y = y[i0] * (1 - f) + y[i0 + 1] * f
    fi = int(0.0008 * TR)
    y = y.copy()
    y[:fi] *= np.linspace(0, 1, fi)
    L = int(keep * TR)
    if loop_lo is None:                               # a one-shot (the palm mutes): its own decay, then a fade
        z = y[:L].copy()
        fd = int(L * 0.35)
        z[-fd:] *= np.linspace(1, 0, fd) ** 2
        return z, None, None
    per = TR / target
    W = max(int(per), int(0.01 * TR))
    best = (-9.0, int(round(per)))
    for nper in range(max(1, int(loop_lo * TR / per)), int(loop_hi * TR / per) + 1, max(1, int(0.01 * TR / per))):
        for d in range(-int(per // 2), int(per // 2) + 1):
            l2 = int(round(nper * per)) + d
            if l2 < 8 or l2 >= L - W - 10:
                continue
            u, v = y[L - W:L], y[L - l2 - W:L - l2]
            cr = np.dot(u, v) / (np.linalg.norm(u) * np.linalg.norm(v) + 1e-12)
            if cr > best[0]:
                best = (cr, l2)
    ll = best[1]
    ls = L - ll
    xf = int(ll * 0.5)
    z = y[:L].copy()
    r = np.linspace(0, 1, xf)
    z[L - xf:L] = y[L - xf:L] * (1 - r) + y[ls - xf:ls] * r          # linear: the two sides are alike
    # M-PYRE: the loop level: a decaying note's loop would drop each time round and its partials beat - played over
    # and over that pulses ("wabert"). The level from a little before the loop on is held at the loop start's (an RMS
    # over two periods); the dying away comes from the preset's envelope (DEC, SUS) instead
    w = max(int(2 * per), int(0.03 * TR))
    env = np.sqrt(np.convolve(z ** 2, np.ones(w) / w, "same")) + 1e-9
    a0 = max(0, ls - int(0.1 * TR))
    g = np.ones(L)
    g[a0:] = env[ls] / env[a0:]
    g[a0:ls] = 1 + (g[a0:ls] - 1) * np.linspace(0, 1, ls - a0)        # (eased in)
    z *= g
    return z, ls, L - 1


def melodic(path, roots, keep, loop_lo, loop_hi, skip=(), flatten=True, pitch_at=(0.2, 1.0), onset=0.10):
    x = load(path)
    sr = 48000
    segs = [s for s in segments(x, sr, len(roots) + len(skip))]
    segs = [s for k, s in enumerate(segs) if k not in skip]
    zones = []
    for k, (name, root) in enumerate(roots):
        e = segs[k + 1] if k + 1 < len(segs) else len(x)
        z, ls, le = note(x, sr, segs[k], e, root, keep, loop_lo, loop_hi, flatten, pitch_at, onset)
        zones.append(dict(name=name, root=root, x=z, loop=None if ls is None else [ls, le]))
    ref = np.mean([k_loud(z["x"], TR) for z in zones])            # the same loudness, note to note
    for z in zones:
        z["x"] *= 10 ** ((ref - k_loud(z["x"], TR)) / 20)
    # pre-emphasis, one scale for the whole set (its notes stay matched): the loudest stored sample at 30000
    pes = [lfilter([1, -PE], [1], z["x"]) / (1 - PE) for z in zones]
    g = 30000 / max(np.abs(p).max() for p in pes)
    for z, p in zip(zones, pes):
        z["pcm"] = np.clip(np.round(p * g), -32768, 32767).astype("<i2")
        z["deemph"] = True
        del z["x"]
    return zones


# the drum track's 16 lanes (firmware/src/drums.c LANE_NOTE): GM note, file, length (s), level vs the kick (dB)
# (M-PYRE: the cymbals a little shorter - their tails fade out - for the flash the new look takes)
KIT = [(36, "Kick", 0.6, 0), (38, "Snare", 0.9, -1), (40, "Snare Ghost", 0.6, -9),
       (42, "Hihat Closed", 0.3, -11), (46, "Hihat Open", 1.5, -9), (44, "Hihat Foot", 0.35, -12),
       (51, "Ride", 1.35, -13), (53, "Bell", 1.2, -12), (48, "Tom High", 1.0, -3.5), (47, "Tom Middle", 1.0, -3),
       (43, "Tom Floor", 1.1, -3), (49, "Crash 1", 1.4, -10), (57, "Crash 2", 1.4, -10),
       (52, "China", 1.35, -10), (84, "REV:Crash 1", 0.9, -10), (83, "FX:drop", 1.2, -3)]
KICK_LUFS, PEAK, KICK_PEAK = -14.5, -4.0, -1.5


def fx_sound(kind, secs):
    """the effect lanes: RISER, noise swept up through a band-pass and a rising tone under it, swelling into the next
    downbeat (it ends at full level); DROP, an impact: a click and a sub tone falling from 110 Hz to 32 Hz, decaying"""
    rng = np.random.default_rng(7)
    n = int(secs * TR)
    t = np.arange(n) / TR
    if kind == "riser":
        noise = rng.standard_normal(n)
        y = np.zeros(n)
        fc = 250 * (7000 / 250) ** (t / secs)                     # the band's centre, 250 Hz .. 7 kHz
        zi = np.zeros(2)
        for i0 in range(0, n, 256):                               # a band-pass a block at a time (its state carried)
            i1 = min(n, i0 + 256)
            w = 2 * np.pi * fc[i0] / TR
            al = np.sin(w) / (2 * 2.5)
            y[i0:i1], zi = lfilter([al, 0, -al], [1 + al, -2 * np.cos(w), 1 - al], noise[i0:i1], zi=zi)
        f0 = 110 * (880 / 110) ** (t / secs)
        tone = np.sin(2 * np.pi * np.cumsum(f0) / TR) * 0.35
        x = (y / (np.abs(y).max() + 1e-9) + tone) * (t / secs) ** 2.2
        x[-int(0.01 * TR):] *= np.linspace(1, 0.2, int(0.01 * TR))
        return x
    # drop
    f0 = 32 + 78 * np.exp(-t * 4.5)
    sub = np.tanh(2.2 * np.sin(2 * np.pi * np.cumsum(f0) / TR)) * np.exp(-t * 2.2)
    click = rng.standard_normal(n) * np.exp(-t * 90) * 0.6
    x = sub + click
    x[-int(0.2 * TR):] *= np.linspace(1, 0, int(0.2 * TR)) ** 2
    return x


def drums():
    out, cache = [], {}
    for gm, fn, keep, rel in KIT:
        if fn.startswith("FX:"):                      # M-PYRE: made here, no recording
            out.append(dict(name=fn[3:].upper(), gm=gm, x=fx_sound(fn[3:], keep), rel=rel))
            continue
        rev = fn.startswith("REV:")
        f = fn[4:] if rev else fn
        if f not in cache:
            cache[f] = load(ASSETS / "drums" / f"{f}.aif", TR)
        x = cache[f]
        on = max(0, int(np.argmax(np.abs(x) > np.abs(x).max() * 0.01)) - 22)
        x = x[on:on + int((1.6 if rev else keep) * TR)].copy()
        fd = int(len(x) * 0.4)
        x[-fd:] *= np.linspace(1, 0, fd) ** 2
        if rev:                                       # the crash backwards, swelling into the next downbeat
            x = x[::-1][-int(keep * TR):].copy()
            x[:int(0.05 * TR)] *= np.linspace(0, 1, int(0.05 * TR))
        out.append(dict(name=("REV " if rev else "") + f, gm=gm, x=x, rel=rel))
    # loudness relative to the kick, then one gain for the kit: nothing over its peak limit
    gains = [10 ** ((KICK_LUFS + d["rel"] - k_loud(d["x"], TR)) / 20) for d in out]
    fac = min((10 ** ((KICK_PEAK if d["name"] in ("Kick", "Snare") else PEAK) / 20)) / (np.abs(d["x"]).max() * g)
              for d, g in zip(out, gains))
    zones = []
    for d, g in zip(out, gains):
        pcm = np.clip(np.round(d["x"] * g * fac * 32767), -32768, 32767).astype("<i2")
        zones.append(dict(name=d["name"], root=d["gm"], lo=d["gm"], hi=d["gm"], loop=None, deemph=False, pcm=pcm))
    return zones


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else SRC / "build" / "mpyre"
    out.mkdir(parents=True, exist_ok=True)
    sets = {
        "PERC": drums(),
        "BASS": melodic(ASSETS / "bass.aif", [("B0", 23), ("E1", 28), ("A1", 33), ("D2", 38), ("G2", 43),
                                              ("C3", 48), ("G3", 55), ("C4", 60)], 0.9, 0.3, 0.45,
                        onset=0.25),   # (the low strings swing up slowly: from a quarter of the peak, in time with a pick)
        "GUITAR": melodic(ASSETS / "guitar_di.aif", [("A1", 33), ("C2", 36), ("E2", 40), ("A2", 45), ("D3", 50),
                                                      ("G3", 55), ("B3", 59), ("E4", 64), ("A4", 69), ("E5", 76),
                                                      ("A5", 81)], 1.2, 0.4, 0.55),
        "PALM": melodic(ASSETS / "palm_di.aif", [("A1", 33), ("C2", 36), ("E2", 40), ("A2", 45), ("C3", 48),
                                                  ("F3", 53), ("A3", 57), ("D4", 62)], 0.55, None, None,
                        flatten=False, pitch_at=(0.02, 0.18)),   # (short, decaying: the pitch early, no drift to flatten)
    }
    js = {}
    for name, zones in sets.items():
        js[name] = [dict(name=z["name"], root=z["root"], lo=z.get("lo"), hi=z.get("hi"), loop=z["loop"],
                         deemph=z["deemph"], pcm=base64.b64encode(z["pcm"].tobytes()).decode()) for z in zones]
        n = sum(len(z["pcm"]) for z in zones)
        print(f"mpyre: {name:6} {len(zones):2} zones, {n / TR:5.2f} s, {n / 2 / 1024:6.1f} KiB ADPCM")
    (out / "sets.json").write_text(json.dumps(js))


if __name__ == "__main__":
    main()
