/* SPDX-License-Identifier: GPL-3.0-only */
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

typedef struct { int32_t s[4]; } bq_st_t;       /* x1 x2 y1 y2 */
static inline int32_t bq(const bq_coef_t *c, bq_st_t *st, int32_t x)
{
    int64_t a = (int64_t)c->c[0] * x + (int64_t)c->c[1] * st->s[0] + (int64_t)c->c[2] * st->s[1] -
                (int64_t)c->c[3] * st->s[2] - (int64_t)c->c[4] * st->s[3];
    int32_t y = (int32_t)(a >> 28);                 /* (Q28 coefficients) */
    st->s[1] = st->s[0];
    st->s[0] = x;
    st->s[3] = st->s[2];
    st->s[2] = y;
    return y;
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
    bq_st_t pre[3], mid[2], post[3], band[4], aa;   /* (mid: at 88.2 kHz, two samples a step) */
    bq_st_t bs[12];                                 /* the bass amps' (track_bass) */
    int32_t xprev;                                  /* the last input (the 2x interpolation) */
    int32_t fir[2 * CAB_TAPS];                      /* the cabinet's history, twice (a straight read) */
    uint32_t fw;                                    /* its write position */
    int32_t c0, c1;                                 /* the cabinet's last two outputs (22.05 kHz) */
    uint8_t ph, model, on;                         /* ph: the cabinet's half-rate phase */
} amp_st_t;
static amp_st_t amp_st[NPART];

#define AMP_IN_SHIFT 2                              /* the part's buffer -> the amp's Q15: a loud note near 0.5 */

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

/* one 88.2 kHz sample through the gain stages */
static inline int32_t amp_stages(amp_st_t *a, const amp_model_t *m, int32_t g1, int32_t u, int32_t b0)
{
    int32_t y = amp_tanh(((u * g1) >> 4) + m->b1) - b0;
    y = bq(&m->mid[0], &a->mid[0], y);
    y = bq(&m->mid[1], &a->mid[1], y);
    if (m->g2)
        y = amp_tanh((y * m->g2) >> 4);
    if (m->g3)
        y = amp_tanh((y * m->g3) >> 4);
    return y;
}

/* the bass amps: the low end clean, the mids and highs driven, in parallel (B-CLEAN: the bands only). BLEND is
 * the PRESENCE knob (64: as voiced) */
static void track_bass(track_t *t, amp_st_t *a, int32_t type, int32_t *b, uint32_t n)
{
    const int16_t *h = t->p[P_CAB] == 0 ? CAB_IR[4] : t->p[P_CAB] <= CAB_N ? CAB_IR[t->p[P_CAB] - 1] : 0;
    int32_t g = (int32_t)((pow2_q16((t->p[P_AGAIN] - 64) * 6)) >> 4);   /* GAIN: Q12, 4096 = as voiced */
    int32_t bl = t->p[P_APRES] * 280, kb[3], i, j;   /* BLEND, Q15: 64 -> 0.55 */
    for (j = 0; j < 3; j++) {
        int32_t v = t->p[P_ABASS + j] - 64;
        kb[j] = v > 0 ? v * 260 : v * 128;
    }
    for (i = 0; i < (int32_t)n; i++) {
        int32_t x = b[i] >> AMP_IN_SHIFT, lo, d, y;
        if (type == 5) {                            /* B-CLEAN */
            y = x;
        } else if (type == 6) {                     /* B-DRIVE */
            lo = bq(&BD_LO, &a->bs[1], bq(&BD_LO, &a->bs[0], x));
            d = bq(&BD_HI, &a->bs[3], bq(&BD_HI, &a->bs[2], x));
            d = bq(&BD_PK1, &a->bs[4], d);
            d = amp_tanh((int32_t)(((int64_t)d * 30 * g) >> 12));
            d = bq(&BD_PK2, &a->bs[5], d);
            d = bq(&BD_LP2, &a->bs[7], bq(&BD_LP1, &a->bs[6], d));
            y = lo + ((((d * 15) >> 5) * bl) >> 14);   /* (the drive at about the low end's level) x BLEND x 2 */
        } else {                                    /* DJENT */
            lo = bq(&DJ_LO, &a->bs[1], bq(&DJ_LO, &a->bs[0], x));
            lo = amp_tanh(lo * 5) >> 1;             /* squeezed: every note as fat */
            d = bq(&DJ_HI, &a->bs[3], bq(&DJ_HI, &a->bs[2], x));
            d = bq(&DJ_PK1, &a->bs[4], d);
            d = amp_tanh((int32_t)(((int64_t)d * 120 * g) >> 12));
            d = bq(&DJ_HP, &a->bs[5], d);
            d = amp_tanh(d * 4);
            d = bq(&DJ_PK2, &a->bs[6], d);
            d = bq(&DJ_PK3, &a->bs[7], d);
            d = bq(&DJ_PK4, &a->bs[8], d);
            d = bq(&DJ_LP2, &a->bs[10], bq(&DJ_LP1, &a->bs[9], d));
            y = lo + ((((d * 15) >> 5) * bl * 3 / 2) >> 14);
        }
        {
            int32_t yb = y;
            for (j = 0; j < 3; j++)                 /* BASS MID TREBLE */
                if (kb[j])
                    y += (bq(&AMP_BANDS[j], &a->band[j], yb) * kb[j]) >> 14;
        }
        y = bq(&B_HP30, &a->bs[11], y);
        if (h)
            y = cab_run(a, h, y);
        b[i] = y << AMP_IN_SHIFT;
    }
}

static void track_amp(track_t *t, int32_t *b, uint32_t n)
{
    uint32_t k = (uint32_t)(t - trk) % NPART, i, j;
    amp_st_t *a = &amp_st[k];
    int32_t type = t->p[P_AMP], cab = t->p[P_CAB];
    const amp_model_t *m;
    const int16_t *h;
    int32_t g1, b0, kb[4], mk;
    if (type < 1 || type > AMP_NMODELS + 3) {        /* OFF: nothing */
        a->on = 0;
        return;
    }
    if (!a->on || a->model != (uint8_t)type) {      /* coming on, or another amp: no stale state (a thump) */
        amp_reset(a);
        a->on = 1;
        a->model = (uint8_t)type;
    }
    if (type > AMP_NMODELS) {                        /* B-CLEAN B-DRIVE DJENT */
        track_bass(t, a, type, b, n);
        return;
    }
    m = &AMP_MODELS[type - 1];
    h = cab >= 1 && cab <= CAB_N ? CAB_IR[cab - 1] : cab == 0 ? CAB_IR[m->cab] : 0;
    /* GAIN: 64 = as voiced, 0 = a quarter, 127 = about 4x (2^((g - 64) / 32), from the pitch table) */
    g1 = (int32_t)((((uint64_t)m->g1 << 8) * pow2_q16((t->p[P_AGAIN] - 64) * 6)) >> 24);
    b0 = amp_tanh(m->b1);
    for (j = 0; j < 4u; j++) {                       /* the knobs: + up to x1 (about +6 dB) .. - x0.5 */
        int32_t v = t->p[P_ABASS + j] - 64;
        kb[j] = v > 0 ? v * 260 : v * 128;          /* Q14 */
    }
    mk = type == 1 ? 26000 : 20000;                 /* make-up: about the level of the dry part */
    for (i = 0; i < n; i++) {
        int32_t x = b[i] >> AMP_IN_SHIFT, y, u0;
        for (j = 0; j < 3u; j++)
            if (bq_used(&m->pre[j]))
                x = bq(&m->pre[j], &a->pre[j], x);
        u0 = (a->xprev + x) >> 1;                   /* 2x: the half-way sample, then this one */
        a->xprev = x;
        y = amp_stages(a, m, g1, u0, b0);
        y = (y + amp_stages(a, m, g1, x, b0)) >> 1;
        for (j = 0; j < 3u; j++)
            if (bq_used(&m->post[j]))
                y = bq(&m->post[j], &a->post[j], y);
        {
            int32_t yb = y;
            for (j = 0; j < 4u; j++)
                if (kb[j])
                    y += (bq(&AMP_BANDS[j], &a->band[j], yb) * kb[j]) >> 14;
        }
        if (h)
            y = cab_run(a, h, y);
        b[i] = (int32_t)(((int64_t)y * mk) >> (13 - AMP_IN_SHIFT));
    }
}
