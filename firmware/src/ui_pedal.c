/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 History Of Sky (M-PYRE) */
/* M-PYRE: the FX pages as pedals (FX > PEDAL, MOD, DELAY, REVERB). On top the page's four values (as LIVE shows them),
 * under them the top half of the pedal, close up: its colour, its knobs at their values, the CHECK light (on when the
 * pedal plays), its name. After the stomp boxes' look, no maker's marks:
 *   BOOST  green, black toothed knobs with silver caps       OD    yellow, gold caps
 *   DIST   orange, silver caps                              FUZZ  hammered silver, a sweep printed round DRIVE
 *   OCT    a brown frame round a black panel, white print
 *   PHASR light green, FLANG magenta, CHOR light blue, TREM petrol: RATE DEPTH MIX in a row
 *   DELAY  grey, blue knobs: TIME FDBK TONE MIX              REVERB  dark grey: TYPE SIZE TONE MIX
 * (TIME and TYPE turned fully left: OFF; MIX the selected track's, as the FX overview's DLY and REV)
 * The drive pedals: DRIVE left, TONE small in the middle, LEVEL right. Drawn into one strip of 240 x 102 under the
 * values, only when something it shows changed. Uses the AMP view's drawing (ui_amp.c) */
enum { PK_BOOST, PK_OD, PK_DIST, PK_FUZZ, PK_OCT, PK_PHASR, PK_FLANG, PK_CHOR, PK_TREM, PK_DELAY, PK_REVERB, PK_OFF };
typedef struct {
    uint16_t body, ink, cap_l, cap_d;    /* the enclosure, its print, the knobs' caps (light .. dark) */
    uint8_t row;                         /* 0: the drive layout (two big, a small one), 3: three in a row, 4: four */
    const char *name;
} pedal_t;
static const pedal_t PEDALS[PK_OFF + 1] = {
    {RGB(52, 170, 70), RGB(20, 70, 30), RGB(240, 240, 236), RGB(140, 140, 144), 0, "BOOST"},
    {RGB(244, 200, 60), RGB(30, 24, 10), RGB(255, 236, 170), RGB(170, 128, 48), 0, "OVERDRIVE"},
    {RGB(240, 120, 36), RGB(70, 40, 20), RGB(244, 244, 244), RGB(150, 150, 156), 0, "DISTORTION"},
    {RGB(176, 180, 186), RGB(50, 52, 58), RGB(244, 244, 244), RGB(150, 150, 156), 0, "FUZZ"},
    {RGB(96, 64, 46), RGB(240, 240, 240), RGB(84, 84, 88), RGB(10, 10, 12), 0, "OCTAVE"},
    {RGB(120, 214, 96), RGB(16, 50, 20), RGB(84, 84, 88), RGB(10, 10, 12), 3, "PHASER"},
    {RGB(206, 40, 150), RGB(250, 230, 244), RGB(84, 84, 88), RGB(10, 10, 12), 3, "FLANGER"},
    {RGB(110, 186, 236), RGB(14, 40, 70), RGB(84, 84, 88), RGB(10, 10, 12), 3, "CHORUS"},
    {RGB(20, 120, 128), RGB(220, 246, 246), RGB(84, 84, 88), RGB(10, 10, 12), 3, "TREMOLO"},
    {RGB(196, 198, 204), RGB(30, 32, 40), RGB(120, 170, 255), RGB(20, 60, 160), 4, "DELAY"},
    {RGB(92, 96, 106), RGB(236, 238, 244), RGB(84, 84, 88), RGB(10, 10, 12), 4, "REVERB"},
    {RGB(44, 44, 48), RGB(140, 140, 146), RGB(84, 84, 88), RGB(10, 10, 12), 3, ""},
};

/* the pedal the page shows (PK_*), -1: not a pedal page; *on: it plays (its CHECK light) */
UI_SMALL static int32_t pedal_kind(int *on)
{
    const page_t *pg = cur_page();
    int32_t v;
    *on = 1;
    if (ui.home || pg->fam != FAM_FX)
        return -1;
    if (pg->scope == SC_TRACK && pg->id[0] == P_PEDAL) {
        v = TSEL->p[P_PEDAL];
        *on = v != 0;
        return v ? v - 1 : PK_OFF;
    }
    if (pg->scope == SC_TRACK && pg->id[0] == P_ITYPE) {
        v = TSEL->p[P_ITYPE];
        *on = v != 0;
        return v ? PK_PHASR + v - 1 : PK_OFF;
    }
    if (pg->scope == SC_GLOBAL && pg->id[0] == G_DTIME) {
        *on = song.g[G_DTIME] != 0;                   /* (TIME OFF: off) */
        return PK_DELAY;
    }
    if (pg->scope == SC_GLOBAL && pg->id[0] == G_RTYPE) {
        *on = song.g[G_RTYPE] != 0;                   /* (TYPE OFF: off) */
        return PK_REVERB;
    }
    return -1;
}

/* a stomp box knob: a black toothed skirt, a cap (light .. dark), its line at the value (0..1000), the two dots of
 * its travel's ends printed beside it */
static void pd_knob(int32_t cx, int32_t cy, int32_t r, int32_t ratio, const pedal_t *p)
{
    int32_t k, x, y, a = -240 + 300 * ratio / 1000;
    int dark_cap = ((p->cap_l >> 11) & 31u) < 16u;
    av_polar(cx, cy, 120, (r + 7) * 16, &x, &y), av_disc(x, y, 1, p->ink);   /* (the ends: down left, down right) */
    av_polar(cx, cy, 60, (r + 7) * 16, &x, &y), av_disc(x, y, 1, p->ink);
    av_disc(cx + 1, cy + 2, r + 4, av_mix(p->body, RGB(0, 0, 0), 1, 2));   /* its shadow */
    av_disc(cx, cy, r + 4, RGB(12, 12, 14));
    for (k = 0; k < 360; k += 15)                       /* the skirt's teeth */
        av_ray(cx, cy, k, (r + 1) * 16, (r + 4) * 16, 1, RGB(44, 44, 48));
    av_cap(cx, cy, r, p->cap_l, p->cap_d, RGB(30, 30, 32), 0);
    av_ray(cx, cy, a, r * 5, r * 15, 2, dark_cap ? RGB(250, 250, 250) : RGB(20, 20, 22));
}

UI_SMALL static void pedal_draw(int32_t kind, int on)
{
    const pedal_t *p = &PEDALS[kind];
    const page_t *pg = cur_page();
    int32_t x, y, k, n = p->row ? p->row : 3, first = p->row == 4 ? 0 : 1;
    uint16_t panel = p->body;
    cv_begin(240, 102, C_BLACK);
    for (y = 2; y < 102; y++) {                         /* the enclosure: its top corners rounded, lit from above */
        int32_t in = y < 10 ? 8 - (int32_t)((y - 2) * (y - 2)) / 8 : 0, e = in < 0 ? 0 : in;
        uint16_t c = y < 5 ? av_add(p->body, 40) : y > 90 ? av_add(p->body, -(y - 90) * 3) : p->body;
        cv_rect(6 + e, y, 228 - 2 * e, 1, c);
    }
    cv_rect(6, 4, 2, 98, av_add(p->body, 24)), cv_rect(232, 4, 2, 98, av_add(p->body, -40));
    if (kind == PK_FUZZ)                                /* hammered: a grain of dents */
        for (y = 6; y < 102; y++)
            for (x = 8; x < 232; x++)
                if ((av_hash(x / 3, y / 3) & 15u) < 3u)
                    cv_pset(x, y, av_add(p->body, (av_hash(x, y) & 1u) ? 20 : -24));
    if (kind == PK_OCT) {                               /* the black panel in the brown frame */
        panel = RGB(18, 18, 20);
        cv_rect(16, 10, 208, 92, panel);
    }
    av_disc(120, 10, 3, RGB(60, 10, 8));               /* CHECK: the light */
    av_disc(120, 10, 2, on ? RGB(255, 40, 30) : RGB(110, 20, 16));
    if (on)
        cv_pset(119, 9, RGB(255, 200, 190));
    for (k = 0; k < n; k++) {
        int32_t slot = first + k, cx, cy, r;
        int16_t *vp = 0;
        const param_desc_t *d = page_desc(pg, (uint32_t)slot, &vp);
        uint16_t ink = p->ink;
        if (p->row == 0) {                              /* the drive layout: DRIVE left, TONE middle (small), LEVEL right */
            cx = k == 0 ? 56 : k == 1 ? 120 : 184, cy = k == 1 ? 52 : 36, r = k == 1 ? 10 : 15;
        } else {
            cx = p->row == 4 ? 36 + k * 56 : 52 + k * 68, cy = 38, r = p->row == 4 ? 12 : 14;
        }
        if (!d || !vp)
            continue;
        if (ui.hot_t && (uint32_t)slot == ui.hot_col)   /* the knob being turned: its name in the panel's other tone */
            ink = ((panel >> 11) & 31u) + ((panel >> 5) & 63u) / 2u < 30u ? RGB(255, 140, 30) : RGB(0, 0, 0);
        pd_knob(cx, cy, r, kind == PK_OFF ? 0 : RATIO(d, *vp), p);
        av_text_c(cx, p->row == 0 && k == 1 ? cy - r - 20 : cy + r + 7, d->label, ink);
    }
    av_text_c(120, 84, p->name[0] ? p->name : "OFF", p->ink);   /* the name, where the lower half begins */
    cv_blit(0, 98);
}
