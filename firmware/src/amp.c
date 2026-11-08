/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 History Of Sky (M-PYRE) */
/* M-PYRE: the amp of a synth part (fx.c mix_part_run, after the drive pedal P_DIST): models of a clean, a crunch, a
 * high-gain and a lead amp (tools/gen_mpyre_amp.py: build/gen/mpyre_amp.h), each
 *   pre      three biquads at 44.1 kHz (the hiss cut, the tightening high-pass, the mid boost of a pedal in front)
 *   stages   gain into tanh (with a bias: even harmonics), two biquads, a second and a power stage, at 88.2 kHz
 *            (2x: the harmonics above 22 kHz fold back far less)
 *   post     the amp's own voicing (three biquads), then the four knobs BASS MID TREBLE PRESENCE as bands added to or
 *            taken from the signal (64: none)
 *   cabinet  an impulse response, 256 taps at 22.05 kHz (11.6 ms; a guitar speaker passes little above 8 kHz, so the
 *            cabinet runs at half rate after an anti-alias low-pass, then is interpolated back)
 * Fixed point: signals in Q15 (32768 = 1.0) inside, biquads with Q28 coefficients in 64-bit sums. State per part. */
#include "mpyre_amp.h"

typedef struct { int32_t s[4]; } bq_st_t;       /* x1 x2, y1 y2 (y with 8 more bits: x 256) */
/* a biquad, its output kept 8 bits finer than the signal: a low-frequency filter's feedback at Q28 with the
 * output rounded to the signal's own steps had a dead band (a quiet guitar note into the high-gain amp's
 * high-pass came out as silence, then in bursts: the "wobble") */
static inline int32_t bq(const bq_coef_t *c, bq_st_t *st, int32_t x)
{
    int64_t a = (((int64_t)c->c[0] * x + (int64_t)c->c[1] * st->s[0] + (int64_t)c->c[2] * st->s[1]) << 8) -
                (int64_t)c->c[3] * st->s[2] - (int64_t)c->c[4] * st->s[3];
    int32_t yf = (int32_t)(a >> 28);                /* (Q28 coefficients; y x 256) */
    st->s[1] = st->s[0];
    st->s[0] = x;
    st->s[3] = st->s[2];
    st->s[2] = yf;
    return yf >> 8;
}
static inline int bq_used(const bq_coef_t *c) { return c->c[1] | c->c[2] | c->c[3] | c->c[4] | (c->c[0] != (1 << 28)); }

/* tanh(x / 32768) * 32767 over 0 .. 8, linearly interpolated (dsp.c softclip stops at 2: a hard edge) */
static inline int32_t amp_tanh(int32_t x)
{
    int32_t a = x < 0 ? -x : x, i = a >> 8, y;
    y = i >= 1024 ? 32767 : AMP_TANH[i] + (((AMP_TANH[i + 1] - AMP_TANH[i]) * (a & 255)) >> 8);
    return x < 0 ? -y : y;
}

typedef struct {
    bq_st_t pre[3], mid[3], post[3], band[4], aa, dec[2], up;   /* (mid, dec: at 88.2 kHz, two samples a step) */
    bq_st_t bs[12];                                 /* the bass amps' (track_bass) */
    bq_st_t hs, lm;                                 /* M-PYRE: after the cabinet (CAB_HS, CAB_LM) */
    bq_st_t eq[5];                                  /* M-PYRE 1.0: HI-GAIN and LEAD after their full-rate cabinet */
    int32_t xprev;                                  /* the last input (the 2x interpolation) */
    int32_t fir[2 * CAB_TAPS];                      /* the cabinet's history, twice (a straight read) */
    uint32_t fw;                                    /* its write position */
    int32_t c0, c1;                                 /* the cabinet's last two outputs (22.05 kHz) */
    int32_t dcx, dcy;                               /* the DC blocker after the amp (asymmetric stages leave an offset) */
    int32_t tl0, tl1;                               /* the bass amp's tone knob (two one-poles) */
    int32_t genv, gg;                               /* the gate before the driven amps: the input's envelope, its gain (Q15) */
    uint8_t ph, model, on;                         /* ph: the cabinet's half-rate phase */
} amp_st_t;
/* one pole at ~20 Hz: y = x - x[-1] + 0.99715 y[-1] (y kept 6 bits finer) */
static inline int32_t amp_dc(amp_st_t *a, int32_t x)
{
    a->dcy = (int32_t)(((int64_t)(x - a->dcx) << 6) + (((int64_t)a->dcy * 32674 + 16384) >> 15));
    a->dcx = x;
    return a->dcy >> 6;
}
static amp_st_t amp_st[NPART];

/* ---- in front of the amp (track_pre), on every synth part: VOL, TONE, COMP, the PEDAL */
typedef struct {
    bq_st_t pd[3], wah;
    int32_t vol, env, cg;                           /* VOL as it glides (Q15); COMP's envelope and gain (Q15) */
    int32_t tl, tl2;                                /* the pedal's TONE (two one-pole low-passes, ~600 Hz) */
    int32_t pt;                                     /* .. and its share after the amp (a one-pole, ~1.5 kHz) */
    uint8_t ped;
} pre_st_t;
static pre_st_t pre_st[NPART];
static void track_pre(track_t *t, int32_t *b, uint32_t n)
{
    pre_st_t *s = &pre_st[(uint32_t)(t - trk) % NPART];
    /* VOL as a guitar's volume pot (audio taper: half way about -20 dB, 10^(2 (v / 127 - 1)) = 2^((v - 127) * 10 / 192)) */
    int32_t vt = t->p[P_PVOL] >= 127 ? 32767 : t->p[P_PVOL] <= 0 ? 0 : (int32_t)(pow2_q16((t->p[P_PVOL] - 127) * 10) >> 1);
    int32_t comp = t->p[P_COMP], ped = t->p[P_PEDAL] < 5 ? t->p[P_PEDAL] : 0, wah = t->p[P_WAH], i;   /* (5: OCT, voice.c) */
    /* M-PYRE: a drive pedal's TONE (0 dark .. 64 as voiced .. 127 bright: the highs over ~1.2 kHz x0 .. x1 .. x2) and
     * LEVEL (P_OMIX: 64 as voiced, 2^((v - 64) / 16): -12 .. +12 dB). TONE after the clipping, as a stomp box's: the
     * highs over ~600 Hz x0 (only the lows: dark, woolly) .. x1 .. x5 (cutting) */
    int32_t tk = t->p[P_TONE] * 1290, lv = (int32_t)(pow2_q16((t->p[P_OMIX] - 64) * 12) >> 4);
    int32_t pm = t->p[P_DIST] * 258;                /* the pedal's MIX (the FX overview's DST): dry .. the pedal, Q15 */
    const bq_coef_t *wc = wah ? &WAH_BQ[(wah - 1) * 63 / 126] : 0;
    int32_t pg = (int32_t)(pow2_q16((t->p[P_PDRV] - 64) * 6) >> 4);   /* DRIVE: Q12, 4096 = as voiced */
    if (vt == 32767 && s->vol == 0)
        s->vol = 32767;
    if (s->vol == 32767 && vt == 32767 && !comp && !ped && !wah)
        return;                                     /* nothing turned: as before */
    if (ped != s->ped) {                            /* another pedal: no stale state */
        memset(s->pd, 0, sizeof s->pd);
        s->ped = (uint8_t)ped;
    }
    if (comp) {                                     /* COMP: the block's peak sets a gain that glides (fast down, slow up) */
        int32_t pk = 1, thr = 20000 - comp * 140, tg;   /* threshold falls as COMP turns up (a guitar note: ~17000 peak;
                                                     * was 60000 - 400 COMP: nothing below ~85 %) */
        for (i = 0; i < (int32_t)n; i++) {
            int32_t a = b[i] < 0 ? -b[i] : b[i];
            if (a > pk)
                pk = a;
        }
        s->env = pk > s->env ? pk : s->env - ((s->env - pk) >> 6);   /* release ~50 ms */
        tg = s->env > thr ? (int32_t)(((int64_t)(thr + (s->env - thr) / 4) << 15) / s->env) : 32767;   /* 4:1 */
        s->cg = tg < s->cg ? tg : s->cg + ((tg - s->cg) >> 4);
        if (!s->cg)
            s->cg = 32767;
    } else
        s->cg = 32767;
    for (i = 0; i < (int32_t)n; i++) {
        int32_t x = b[i], y, x0;
        s->vol += (vt - s->vol) >> 8;               /* VOL glides (~6 ms): fades without steps */
        x = (int32_t)(((int64_t)x * s->vol) >> 15);
        if (comp) {
            x = (int32_t)(((int64_t)x * s->cg) >> 15);
            x += (int32_t)(((int64_t)x * comp) >> 7);   /* make-up, up to 2x */
        }
        x0 = x;
        switch (ped) {
        case 1:                                     /* BOOST: louder, the highs up */
            y = x + ((bq(&PD_BOOST, &s->pd[0], x) * 3) >> 1);
            x = (int32_t)(((int64_t)y * pg) >> 11);
            break;
        case 2:                                     /* OD: the mids above 720 Hz, softly clipped, onto the clean signal */
            y = bq(&PD_ODHP, &s->pd[0], x >> 2);
            y = amp_tanh((int32_t)(((int64_t)y * pg * 12) >> 12));
            x = bq(&PD_ODLP, &s->pd[1], (x >> 2) + y) << 2;
            break;
        case 3:                                     /* DIST: hard */
            y = bq(&PD_DSHP, &s->pd[0], x >> 2);
            y = amp_tanh((int32_t)(((int64_t)y * pg * 40) >> 12));
            y = clamp(y + (y >> 1), -30000, 30000);
            x = bq(&PD_DSLP, &s->pd[1], y) >> 2;    /* (about +5 dB over the dry note, as the others) */
            break;
        case 4:                                     /* FUZZ, a "muff": two clipping stages one after the other (the
                                                     * second near square: the endless sustain), then the tone stack's
                                                     * scoop - a low-pass and a high-pass at 1 kHz added (the mids down) */
            y = amp_tanh((int32_t)(((int64_t)x * pg * 28) >> 12));
            y = amp_tanh(y * 7);
            y = bq(&PD_FZAA, &s->pd[2], y);
            x = (bq(&PD_FZLP, &s->pd[0], y) + bq(&PD_FZHP, &s->pd[1], y)) >> 3;
            break;
        default:
            break;
        }
        if (ped) {                                  /* the pedal's TONE, LEVEL and MIX */
            s->tl += (int32_t)(((int64_t)(x - s->tl) * 2800) >> 15);   /* (two poles, ~600 Hz) */
            s->tl2 += (int32_t)(((int64_t)(s->tl - s->tl2) * 2800) >> 15);
            x = s->tl2 + (int32_t)(((int64_t)(x - s->tl2) * tk) >> 15);
            x = (int32_t)(((int64_t)x * lv) >> 12);
            if (pm < 32766)
                x = x0 + (int32_t)(((int64_t)(x - x0) * pm) >> 15);
        }
        if (wc)                                     /* WAH: the band (resonant), with a little of the dry under it; the
                                                     * higher the band the less of a guitar it finds: made up (as loud
                                                     * all the way, toe to heel) */
            x = (int32_t)(((int64_t)(bq(wc, &s->wah, x) * 4 + (x >> 3)) * (384 + wah * 9)) >> 9);   /* (the band
                                                     * stronger, less dry: a direct, vocal wah) */
        b[i] = x;
    }
}

/* M-PYRE: a drive pedal's TONE after the amp too - in front of a high-gain amp a pedal's tone is mostly squeezed out
 * again; here the highs over ~1.5 kHz follow it as well, x0.4 (-8 dB) .. x1 (64) .. x2 (+6 dB) */
static void track_ptone(track_t *t, int32_t *b, uint32_t n)
{
    pre_st_t *s = &pre_st[(uint32_t)(t - trk) % NPART];
    int32_t g, i;
    if (t->p[P_PEDAL] < 1 || t->p[P_PEDAL] > 4 || t->p[P_TONE] == 64 || !t->p[P_AMP])
        return;
    g = t->p[P_TONE] < 64 ? 13107 + t->p[P_TONE] * 307 : 32767 + (t->p[P_TONE] - 64) * 520;
    for (i = 0; i < (int32_t)n; i++) {
        s->pt += (int32_t)(((int64_t)(b[i] - s->pt) * 6700) >> 15);
        b[i] = s->pt + (int32_t)(((int64_t)(b[i] - s->pt) * g) >> 15);
    }
}

#define AMP_IN_SHIFT 0                              /* the part's buffer as the amp's Q15: a loud guitar note near 0.5 (as the amps were voiced) */

static void amp_reset(amp_st_t *a)
{
    memset(a, 0, sizeof *a);
}

/* the cabinet: one input at 44.1 kHz in, one out; every other input feeds the 22.05 kHz FIR */
static inline int32_t cab_run(amp_st_t *a, const int16_t *h, int32_t x)
{
    x = bq(&CAB_AA, &a->aa, x);
    if (a->ph ^= 1u) {
        int64_t acc = 0;
        const int32_t *p;
        uint32_t k;
        a->fw = (a->fw + CAB_TAPS - 1u) % CAB_TAPS;  /* newest first: p[k] is x[n - k] */
        a->fir[a->fw] = a->fir[a->fw + CAB_TAPS] = x;
        p = &a->fir[a->fw];
        for (k = 0; k < CAB_TAPS; k += 4u)
            acc += (int64_t)p[k] * h[k] + (int64_t)p[k + 1u] * h[k + 1u] + (int64_t)p[k + 2u] * h[k + 2u] +
                   (int64_t)p[k + 3u] * h[k + 3u];
        a->c0 = a->c1;
        a->c1 = (int32_t)(acc >> 15);
        return (a->c0 + a->c1) >> 1;                /* (halfway: the interpolation) */
    }
    return a->c1;
}

/* M-PYRE 1.0: the HI-GAIN and LEAD cabinets at the full rate (CABF_IR): their highs as recorded. 1.0.1: CABF_TAPS
 * (64) of them, the rest of the box's response in the model's eq - a quarter of the cost (the ring stays CAB_TAPS) */
static inline int32_t cabf_run(amp_st_t *a, const int16_t *h, int32_t x)
{
    int64_t acc = 0;
    const int32_t *p;
    uint32_t k;
    a->fw = (a->fw + CAB_TAPS - 1u) % CAB_TAPS;      /* newest first: p[k] is x[n - k] */
    a->fir[a->fw] = a->fir[a->fw + CAB_TAPS] = x;
    p = &a->fir[a->fw];
    for (k = 0; k < CABF_TAPS; k += 4u)
        acc += (int64_t)p[k] * h[k] + (int64_t)p[k + 1u] * h[k + 1u] + (int64_t)p[k + 2u] * h[k + 2u] +
               (int64_t)p[k + 3u] * h[k + 3u];
    return (int32_t)(acc >> 15);
}

/* one 88.2 kHz sample through the gain stages */
static inline int32_t amp_stages(amp_st_t *a, const amp_model_t *m, int32_t g1, int32_t g2, int32_t u, int32_t b0)
{
    int32_t y = amp_tanh(((u * g1) >> 4) + m->b1) - b0;
    y = bq(&m->mid[0], &a->mid[0], y);
    y = bq(&m->mid[1], &a->mid[1], y);
    if (g2)
        y = amp_tanh((y * g2) >> 4);
    if (m->g3) {
        y = bq(&m->mid[2], &a->mid[2], y);
        y = amp_tanh((y * m->g3) >> 4);
    }
    {
        const bq_coef_t *d = m->full < 2u ? AMP_DECF : AMP_DEC;   /* (88.2 -> 44.1 kHz: no fold-back fizz; the fitted amps
                                                     * keep up to 14 kHz) */
        return bq(&d[1], &a->dec[1], bq(&d[0], &a->dec[0], y));
    }
}

/* the bass amp: the low end clean, the mids and highs driven, in parallel. GAIN 0: the bass clean (the split faded
 * out over the first quarter), up from there the drive band 2^(g / 18) harder (x3 .. x400: a growl, then a fuzz).
 * TREBLE: under 64 a passive tone knob closing (a two-pole low-pass at ~250 Hz faded in: at 0 only the bass left),
 * over 64 the clank (+ up to ~x4 at 1.8 kHz, ~x2.5 over 2.2 kHz). PRES is the drive's BLEND (64: as voiced) */
static void track_bass(track_t *t, amp_st_t *a, int32_t *b, uint32_t n)
{
    const int16_t *h = t->p[P_CAB] == 0 ? CAB_IR[4] : t->p[P_CAB] <= CAB_N ? CAB_IR[t->p[P_CAB] - 1] : 0;
    int32_t gn = t->p[P_AGAIN], g = (int32_t)(pow2_q16(gn * 32 / 3) >> 6);   /* the drive 2^(g / 18), Q10 (x3 at 0 .. ~x400) */
    int32_t sp = gn >= 32 ? 32 : gn;                 /* the split, /32 */
    int32_t bl = t->p[P_APRES] * 280, kb[2], tv = t->p[P_ATREB] - 64, i, j;   /* BLEND, Q15: 64 -> 0.55 */
    for (j = 0; j < 2; j++) {
        int32_t v = t->p[P_ABASS + j] - 64;
        kb[j] = v > 0 ? v * 260 : v * 128;
    }
    for (i = 0; i < (int32_t)n; i++) {
        int32_t x = (b[i] * 21) >> 5, lo, d, y;     /* (the bass samples come in hotter than the guitar: to ~0.5) */
        lo = bq(&BD_LO, &a->bs[1], bq(&BD_LO, &a->bs[0], x));
        d = bq(&BD_HI, &a->bs[3], bq(&BD_HI, &a->bs[2], x));
        d = bq(&BD_PK1, &a->bs[4], d);
        d = amp_tanh((int32_t)(((int64_t)d * 3 * g) >> 10));
        d = bq(&BD_PK2, &a->bs[5], d);
        d = bq(&BD_LP2, &a->bs[7], bq(&BD_LP1, &a->bs[6], d));
        y = lo + ((((d * 15) >> 5) * bl) >> 14);     /* (the drive at about the low end's level) x BLEND x 2 */
        y = x + (((y - x) * sp) >> 5);
        {
            int32_t yb = y;
            for (j = 0; j < 2; j++)                 /* BASS MID */
                if (kb[j])
                    y += (bq(&AMP_BANDS[j], &a->band[j], yb) * kb[j]) >> 14;
            if (tv > 0)                             /* TREBLE up: the clank (the string on the frets, ~1.8 kHz) */
                y += ((bq(&AMP_BANDS[2], &a->band[2], yb) * tv * 390) >> 14) + ((bq(&TN_BITE, &a->band[3], yb) * tv * 1040) >> 14);
        }
        a->tl0 += ((y - a->tl0) * 1147) >> 15;      /* (the tone knob: ~250 Hz, two poles) */
        a->tl1 += ((a->tl0 - a->tl1) * 1147) >> 15;
        if (tv < 0)
            y += ((a->tl1 - y) * -tv) >> 6;
        y = bq(&B_LS, &a->bs[10], bq(&B_HP30, &a->bs[11], y));
        if (h)
            y = cab_run(a, h, y);
        b[i] = amp_dc(a, (y * 9) >> 3);
    }
}

static void track_amp(track_t *t, int32_t *b, uint32_t n)
{
    uint32_t k = (uint32_t)(t - trk) % NPART, i, j;
    amp_st_t *a = &amp_st[k];
    int32_t type = t->p[P_AMP], cab = t->p[P_CAB];
    const amp_model_t *m;
    const int16_t *h;
    int32_t g1, g2, b0, kb[4], mk, gmk = 4096;
    if (type < 1 || type > AMP_NMODELS + 3) {        /* OFF: nothing */
        a->on = 0;
        return;
    }
    if (!a->on || a->model != (uint8_t)type) {      /* coming on, or another amp: no stale state (a thump) */
        amp_reset(a);
        a->on = 1;
        a->model = (uint8_t)type;
    }
    if (type > AMP_NMODELS) {                        /* BASS (6, 7: the old bass amps, the same) */
        track_bass(t, a, b, n);
        return;
    }
    m = &AMP_MODELS[type - 1];
    h = cab >= 1 && cab <= CAB_N ? CAB_IR[cab - 1] : cab == 0 ? CAB_IR[m->cab] : 0;
    b0 = amp_tanh(m->b1);
    {   /* GAIN. CLEAN: 2^((g - 64) / 32) on its one stage, 64 as voiced. The driven amps: on both stages, the first
         * 2^((g - 80) / 16), the second 2^((g - 80) / 24) - 80 as voiced (the factory sounds sit there), turned down
         * to 0 a light crunch (1/32 and 1/10), up to 127 the clipping thickens and squeezes (x 7.6, x 3.9). The level
         * stays about the same: the stages saturate (measured, /tmp an/ampt) */
        int32_t gk = t->p[P_AGAIN] - (type > 1 ? 80 : 64);
        g1 = (int32_t)((((uint64_t)m->g1 << 8) * pow2_q16(gk * (type > 1 ? 12 : 6))) >> 24);
        g2 = m->g2 ? (int32_t)((((uint64_t)m->g2 << 8) * pow2_q16(gk * 8)) >> 24) : 0;
        if (g2 < 1 && m->g2)
            g2 = 1;
        if (type > 1 && gk < 0) {                       /* below 80 the stages leave saturation: made up, measured
                                                         * (CRNCH -29 dB at 0, HIGH -7.5, LEAD -7: ~ gk^2) */
            static const uint8_t MKC[3] = {147, 38, 36};
            gmk = (int32_t)(pow2_q16((gk * gk * MKC[(type - 2) % 3]) >> 10) >> 4);
        }
    }
    for (j = 0; j < 4u; j++) {                       /* the knobs: + up to x1 (about +6 dB) .. - x0.5 */
        int32_t v = t->p[P_ABASS + j] - 64;
        kb[j] = v > 0 ? v * 260 : v * 128;          /* Q14 */
    }
    {   /* make-up per amp: a held note about as loud as the CLEAN amp's, under the master limiter (measured renders) */
        static const int16_t MK[4] = {13000, 1840, 578, 801};    /* (1.0: HI-GAIN, LEAD as loud as before; 1.0.1: -3.4 / -2.7 dB,
                                                         * what the refitted eq adds) */
        mk = (int32_t)(((int64_t)MK[(type - 1) & 3] * gmk) >> 12);
    }
    for (i = 0; i < n; i++) {
        int32_t x = b[i] >> AMP_IN_SHIFT, y, u0;
        if (type > 1) {                             /* a gate (the reference's, -40 dB): between notes the driven amps
                                                     * would raise the samples' hiss to a roar */
            int32_t ax = x < 0 ? -x : x;
            a->genv = ax > a->genv ? ax : a->genv - (a->genv >> 11);   /* (the peak, falling ~50 ms) */
            a->gg += a->genv > 330 ? (32767 - a->gg) >> 5 : -(a->gg >> 11);
            x = (int32_t)(((int64_t)x * a->gg) >> 15);
        }
        for (j = 0; j < 3u; j++)
            if (bq_used(&m->pre[j]))
                x = bq(&m->pre[j], &a->pre[j], x);
        u0 = (a->xprev + x) >> 1;                   /* 2x: the half-way sample, then this one */
        a->xprev = x;
        (void)amp_stages(a, m, g1, g2, u0, b0);     /* (the half-way sample: through the filters, then dropped) */
        y = amp_stages(a, m, g1, g2, x, b0);
        for (j = 0; j < 3u; j++)
            if (bq_used(&m->post[j]))
                y = bq(&m->post[j], &a->post[j], y);
        {
            int32_t yb = y;
            for (j = 0; j < 4u; j++)
                if (kb[j])
                    y += (bq(&AMP_BANDS[j], &a->band[j], yb) * kb[j]) >> 14;
        }
        if (h && m->full < 2u && h == CAB_IR[m->cab]) {   /* M-PYRE 1.0: HI-GAIN / LEAD on their own cabinet */
            y = cabf_run(a, CABF_IR[m->full], y);
            for (j = 0; j < 5u; j++)
                y = bq(&m->eq[j], &a->eq[j], y);
        } else if (h) {
            y = bq(&CAB_AA, &a->up, cab_run(a, h, y));   /* (the half-rate cabinet back to 44.1 kHz: its images cut) */
            y = bq(&CAB_LM, &a->lm, bq(&CAB_HS, &a->hs, y));   /* (M-PYRE: its highs given back, the low mids fuller) */
        }
        b[i] = amp_dc(a, (int32_t)(((int64_t)y * mk) >> (13 - AMP_IN_SHIFT)));
    }
}
