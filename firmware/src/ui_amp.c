/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 History Of Sky (M-PYRE) */
/* M-PYRE: the AMP view (EDIT > AMP and AMP 2, a synth track's): on top AMP CAB PRES COMP, under them the amp itself, its
 * face after the amp family it plays (no real maker's marks: the cloth, the panel, a wordmark of our own), its knobs
 * GAIN BASS MID TREB on its panel as rings of fire LEDs. KNOB 1..4 turn the top row on AMP, the amp's knobs on AMP 2
 * (the row they turn is lit). Drawn on the screen's canvas in two strips (it holds 124 rows), only when it changed.
 *   CLEAN  tweed: a lacquered twill, oxblood linen with gold stripes, a chrome plate, a script
 *   CRNCH  black levant, a black basketweave with a gold thread, a brushed gold panel, a white script
 *   HIGH   black, a diamond-plate front with its screws, a black panel, chrome block letters on a plate
 *   LEAD   oxblood, a wine grille, a mirror-chrome panel, italic letters in gold and fire
 *   BASS               the bass amp: black, a ribbed grille, a blue-grey panel
 * The wordmarks: tools/gen_mpyre_amplogo.py (fonts under the SIL OFL, assets/mpyre/fonts) */
#include "mpyre_amplogo.h"

#define AV_HX0 3                        /* the head on the screen (down into the footer's step row: the footer shows */
#define AV_HX1 236                      /* only its name row here, draw_foot) */
#define AV_HY0 58
#define AV_HY1 217
#define AV_GY0 64                       /* its grille */
#define AV_GY1 136
#define AV_PY0 143                      /* its control panel */
#define AV_PY1 210
#define AV_X0 10                        /* the grille's and the panel's sides */
#define AV_X1 229

enum { CAB_TWEED, CAB_LEVANT };
enum { GR_LINEN, GR_BASKET, GR_PLATE, GR_WINE, GR_RIBS };
enum { KN_CHICKEN, KN_GOLD, KN_CHROME, KN_BLACK, KN_SILVER };   /* each amp's knob, inside the ring of LEDs */
typedef struct {
    uint8_t cab, grille, logo, script, knob;  /* the covering, the grille, the wordmark (AMPLOGO), 1: a script (a shadow) */
    uint16_t tolex;                     /* the levant's colour */
    uint16_t pan_top, pan_bot, ink;     /* the panel: its gradient, its print */
    uint16_t piping;                    /* the frame round the grille and the panel */
    uint16_t logo_a, logo_b, logo_line; /* the wordmark: top / bottom of its gradient, its outline (0: none) */
} amp_face_t;
static const amp_face_t AMP_FACES[5] = {
    {CAB_TWEED, GR_LINEN, 0, 1, KN_CHICKEN, 0, RGB(214, 214, 218), RGB(150, 150, 156), RGB(24, 22, 20), RGB(110, 80, 40),
     RGB(244, 230, 196), RGB(236, 214, 170), 0},
    {CAB_LEVANT, GR_BASKET, 1, 1, KN_GOLD, RGB(22, 20, 19), RGB(226, 186, 92), RGB(168, 126, 50), RGB(26, 18, 8), RGB(200, 164, 90),
     RGB(246, 244, 236), RGB(232, 228, 216), 0},
    {CAB_LEVANT, GR_PLATE, 2, 0, KN_CHROME, RGB(22, 20, 19), RGB(34, 34, 38), RGB(14, 14, 16), RGB(226, 226, 230), RGB(90, 90, 96),
     RGB(250, 250, 255), RGB(120, 120, 132), RGB(8, 8, 10)},
    {CAB_LEVANT, GR_WINE, 3, 0, KN_BLACK, RGB(58, 10, 16), RGB(236, 236, 240), RGB(176, 176, 186), RGB(60, 8, 14), RGB(220, 180, 100),
     RGB(255, 214, 120), RGB(226, 120, 20), RGB(30, 4, 8)},
    {CAB_LEVANT, GR_RIBS, 4, 0, KN_SILVER, RGB(22, 20, 19), RGB(150, 168, 190), RGB(98, 114, 136), RGB(14, 20, 32), RGB(120, 130, 150),
     RGB(226, 232, 240), RGB(200, 208, 220), RGB(10, 12, 18)},
};

static uint32_t av_sig;                 /* what the view shows (redrawn when it changes) */

UI_SMALL static int amp_view_on(void)
{
    const page_t *pg = cur_page();
    return !ui.home && pg->scope == SC_TRACK && (pg->id[0] == P_AMP || pg->id[0] == P_AGAIN) && !is_drum(TSEL);
}

static inline uint16_t av_mix(uint32_t a, uint32_t b, uint32_t k, uint32_t n)   /* a .. b at k / n, RGB565 */
{
    uint32_t r = (((a >> 11) & 31u) * (n - k) + ((b >> 11) & 31u) * k) / n;
    uint32_t g = (((a >> 5) & 63u) * (n - k) + ((b >> 5) & 63u) * k) / n;
    uint32_t bl = ((a & 31u) * (n - k) + (b & 31u) * k) / n;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
static inline uint16_t av_add(uint32_t c, int32_t d)                 /* lighter (d > 0) / darker, each channel */
{
    int32_t r = (int32_t)((c >> 11) & 31u) + d / 8, g = (int32_t)((c >> 5) & 63u) + d / 4, b = (int32_t)(c & 31u) + d / 8;
    return (uint16_t)(clamp(r, 0, 31) << 11 | clamp(g, 0, 63) << 5 | clamp(b, 0, 31));
}
static inline uint32_t av_hash(int32_t x, int32_t y)                 /* a fixed grain (the same at every redraw) */
{
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static inline uint16_t av_get(int32_t x, int32_t y)
{
    y += cv_oy;
    return (uint32_t)x < cv_w && (uint32_t)y < cv_h ? swap16(cv_px[(uint32_t)y * cv_w + (uint32_t)x]) : 0u;
}
/* a filled disc of radius r (r^2 + r: rounder at small sizes) */
static void av_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t x, y;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++)
            if (x * x + y * y <= r * r + r)
                cv_pset(cx + x, cy + y, c);
}
/* the point at angle deg (0: right, 90: down) and radius r (Q4) from (cx, cy) */
static void av_polar(int32_t cx, int32_t cy, int32_t deg, int32_t r16, int32_t *x, int32_t *y)
{
    uint32_t ph = (uint32_t)(((int64_t)deg << 32) / 360);
    *x = cx + ((sine_i(ph + 0x40000000u) * r16 + (1 << 18)) >> 19);
    *y = cy + ((sine_i(ph) * r16 + (1 << 18)) >> 19);
}

/* ---- the covering, the grilles, the panel ---- */
static void av_cab(const amp_face_t *f)
{
    int32_t x, y;
    for (y = AV_HY0; y <= AV_HY1; y++) {
        if (y + cv_oy < 0 || y + cv_oy >= (int32_t)cv_h)
            continue;
        for (x = AV_HX0; x <= AV_HX1; x++) {
            uint32_t h = av_hash(x, y);
            uint16_t c;
            if (f->cab == CAB_TWEED) {                  /* a diagonal twill, lacquered */
                uint32_t d = (uint32_t)(x + y) % 6u;
                c = d == 0u ? RGB(150, 112, 62) : d == 3u ? RGB(176, 140, 84) : RGB(204, 176, 112);
                if ((h & 31u) == 0u)
                    c = RGB(120, 88, 44);
            } else {                                    /* levant: a fine grain */
                c = (h & 7u) == 0u ? av_add(f->tolex, 8 + (int32_t)((h >> 3) & 15u)) : f->tolex;
            }
            cv_pset(x, y, c);
        }
    }
    {   /* the rounded corners and the outline */
        static const uint8_t RC[4] = {3, 1, 1, 0};
        for (y = 0; y < 4; y++)
            for (x = 0; x < RC[y]; x++) {
                cv_pset(AV_HX0 + x, AV_HY0 + y, C_BLACK), cv_pset(AV_HX1 - x, AV_HY0 + y, C_BLACK);
                cv_pset(AV_HX0 + x, AV_HY1 - y, C_BLACK), cv_pset(AV_HX1 - x, AV_HY1 - y, C_BLACK);
            }
    }
}
static void av_frame(int32_t y0, int32_t y1, uint16_t c)              /* the piping round grille / panel */
{
    cv_rect(AV_X0 - 1, y0 - 1, AV_X1 - AV_X0 + 3, 1, c);
    cv_rect(AV_X0 - 1, y1 + 1, AV_X1 - AV_X0 + 3, 1, c);
    cv_rect(AV_X0 - 1, y0, 1, y1 - y0 + 1, c);
    cv_rect(AV_X1 + 1, y0, 1, y1 - y0 + 1, c);
}
static void av_grille(const amp_face_t *f)
{
    int32_t x, y;
    for (y = AV_GY0; y <= AV_GY1; y++) {
        if (y + cv_oy < 0 || y + cv_oy >= (int32_t)cv_h)
            continue;
        for (x = AV_X0; x <= AV_X1; x++) {
            uint16_t c;
            switch (f->grille) {
            case GR_LINEN:                              /* oxblood linen, gold stripes */
                c = (x - AV_X0) % 7 == 3 ? RGB(138, 92, 58) : (y & 1) ? RGB(112, 62, 42) : RGB(96, 50, 34);
                break;
            case GR_BASKET:                             /* a black basketweave, a gold thread every 24 px */
                c = ((x - AV_X0) % 24 == 1) ? RGB(92, 78, 50)
                    : (((x + ((y >> 2) & 1) * 2) & 3) < 2 && (y & 3) < 2) ? RGB(44, 40, 34) : RGB(18, 17, 16);
                break;
            case GR_PLATE: {                            /* the diamond plate: a light / dark pair of strokes in rows */
                int32_t g = 150 + 50 * (AV_GY1 - y) / (AV_GY1 - AV_GY0), ry = (y - AV_GY0 - 3) & 7, row = (y - AV_GY0 - 3) >> 3;
                int32_t cx = (x - AV_X0 - 3 - (row & 1) * 6) % 12;
                c = RGB(g, g, g + 6);
                if (y - AV_GY0 >= 3 && y < AV_GY1 - 2 && cx >= 0 && cx < 6) {
                    if (cx + ry == 3 && cx < 5)
                        c = RGB(236, 236, 240);
                    else if (cx + ry == 5 && cx >= 1)
                        c = RGB(96, 96, 104);
                }
                break;
            }
            case GR_WINE:
                c = ((x + (y % 3)) % 3 == 0 && y % 3 == 0) ? RGB(84, 26, 38) : RGB(52, 14, 22);
                break;
            default:                                    /* ribs */
                c = (y - AV_GY0) % 4 == 2 ? RGB(84, 92, 110) : RGB(20, 22, 28);
                break;
            }
            cv_pset(x, y, c);
        }
    }
    if (f->grille == GR_PLATE) {                        /* the screws */
        static const int8_t SX[2] = {AV_X0 + 4, AV_X1 - 4}, SY[2] = {AV_GY0 + 4, AV_GY1 - 4};
        for (x = 0; x < 2; x++)
            for (y = 0; y < 2; y++) {
                av_disc(SX[x], SY[y], 2, RGB(80, 80, 86));
                cv_rect(SX[x] - 1, SY[y], 3, 1, RGB(30, 30, 30));
            }
    }
    av_frame(AV_GY0, AV_GY1, f->piping);
}
static void av_panel(const amp_face_t *f)
{
    int32_t y, x, n = AV_PY1 - AV_PY0;
    for (y = AV_PY0; y <= AV_PY1; y++) {
        uint16_t c = av_mix(f->pan_top, f->pan_bot, (uint32_t)(y - AV_PY0), (uint32_t)n);
        cv_rect(AV_X0, y, AV_X1 - AV_X0 + 1, 1, c);
        for (x = AV_X0; x <= AV_X1; x++)                /* brushed: short lighter strokes */
            if ((av_hash(x >> 4, y) & 7u) == 0u && (av_hash(x, y) & 3u) != 0u)
                cv_pset(x, y, av_add(c, 7));
    }
    cv_rect(AV_X0, AV_PY0, AV_X1 - AV_X0 + 1, 1, av_add(f->pan_top, 50));
    cv_rect(AV_X0, AV_PY1, AV_X1 - AV_X0 + 1, 1, av_add(f->pan_bot, -60));
    av_frame(AV_PY0, AV_PY1, f->piping);
}

/* the wordmark k, its middle at (cx, cy): its gradient over what is under it, an outline, a script's shadow */
static void av_logo(const amp_face_t *f, int32_t cx, int32_t cy)
{
    const amplogo_t *L = &AMPLOGO[f->logo];
    int32_t w = L->w, h = L->h, x0 = cx - w / 2, y0 = cy - h / 2, x, y, pass;
#define AV_COV(xx, yy) ((uint32_t)(xx) < (uint32_t)w && (uint32_t)(yy) < (uint32_t)h ? \
    (AMPLOGO_DATA[L->off + ((uint32_t)(yy) * (uint32_t)w + (uint32_t)(xx)) / 4u] >> (6u - 2u * (((uint32_t)(yy) * (uint32_t)w + (uint32_t)(xx)) & 3u))) & 3u : 0u)
    for (pass = 0; pass < 3; pass++) {                  /* 0: the shadow / outline, 1: (unused), 2: the letters */
        if (pass == 1)
            continue;
        for (y = -1; y <= h; y++)
            for (x = -1; x <= w; x++) {
                uint32_t a = AV_COV(x, y), k;
                uint16_t c;
                if (pass == 0) {
                    if (f->script) {                    /* a script: its shadow one down and right */
                        a = AV_COV(x - 1, y - 1);
                        c = RGB(0, 0, 0);
                    } else if (f->logo_line) {          /* block letters: an outline */
                        a = 0;
                        for (k = 0; k < 9u; k++) {
                            uint32_t b = AV_COV(x + (int32_t)(k % 3u) - 1, y + (int32_t)(k / 3u) - 1);
                            a = b > a ? b : a;
                        }
                        c = f->logo_line;
                    } else {
                        continue;
                    }
                } else {
                    c = av_mix(f->logo_a, f->logo_b, (uint32_t)clamp(y, 0, h), (uint32_t)(h ? h : 1));
                }
                if (a)
                    cv_pset(x0 + x, y0 + y, a == 3u ? c : av_mix(av_get(x0 + x, y0 + y), c, a, 3u));
            }
    }
#undef AV_COV
}

/* a shaded cap of radius r: lit from the top left (light .. dark across it), a highlight, a rim; knurl: a toothed edge */
static void av_cap(int32_t cx, int32_t cy, int32_t r, uint16_t light, uint16_t dark, uint16_t rim, int knurl)
{
    int32_t x, y;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++) {
            int32_t d2 = x * x + y * y;
            uint16_t c;
            if (d2 > r * r + r)
                continue;
            c = av_mix(light, dark, (uint32_t)(x + y + 2 * r), (uint32_t)(4 * r));
            if ((x + r / 3) * (x + r / 3) + (y + r / 3) * (y + r / 3) <= r * r / 9)
                c = av_mix(c, RGB(255, 255, 255), 1, 2);   /* the highlight */
            if (d2 > (r - 1) * (r - 1) + (r - 1))
                c = knurl && ((x * 3 + y * 7) & 2) ? av_add(rim, 30) : rim;
            cv_pset(cx + x, cy + y, c);
        }
}
/* a line from radius r0 to r1 (Q4) at angle a, w pixels wide (1 or 2) */
static void av_ray(int32_t cx, int32_t cy, int32_t a, int32_t r0, int32_t r1, int w, uint16_t c)
{
    int32_t x, y, x1, y1;
    av_polar(cx, cy, a, r0, &x, &y);
    av_polar(cx, cy, a, r1, &x1, &y1);
    cv_line(x, y, x1, y1, c);
    if (w > 1) {
        cv_line(x + 1, y, x1 + 1, y1, c);
        cv_line(x, y + 1, x1, y1 + 1, c);
    }
}
/* a knob, the amp's own (KN_*), at the value (0..1000), its scale round it */
static void av_knob(int32_t cx, int32_t cy, int32_t ratio, int dark_panel, uint32_t kind, uint16_t ink)
{
    int32_t k, x, y, a = -240 + 300 * ratio / 1000;
    (void)dark_panel;
    for (k = 0; k <= 10; k++) {                         /* the scale printed round it: 11 ticks, the panel's ink */
        av_polar(cx, cy, -240 + 30 * k, 232, &x, &y);   /* (radius 14.5) */
        cv_pset(x, y, ink);
        if (!(k % 5))
            av_polar(cx, cy, -240 + 30 * k, 256, &x, &y), cv_pset(x, y, ink);
    }
    switch (kind) {
    case KN_CHICKEN:                                    /* black chicken head: a round base, the long pointed head */
        av_cap(cx, cy, 8, RGB(70, 70, 74), RGB(8, 8, 10), RGB(0, 0, 0), 0);
        for (k = 0; k < 8; k++) {                       /* the head: tapering from the base out along the value */
            static const uint8_t HR[8] = {5, 5, 4, 4, 3, 3, 2, 1};
            av_polar(cx, cy, a, 8 + k * 26, &x, &y);
            av_disc(x, y, HR[k], k < 3 ? RGB(34, 34, 36) : RGB(20, 20, 22));
        }
        av_polar(cx, cy, a + 180, 80, &x, &y);          /* its short tail */
        av_disc(x, y, 3, RGB(20, 20, 22));
        av_ray(cx, cy, a, 48, 196, 1, RGB(240, 240, 236));   /* the white line along it */
        break;
    case KN_GOLD:                                       /* gold metal, knurled */
        av_cap(cx, cy, 11, RGB(255, 236, 160), RGB(150, 104, 30), RGB(110, 76, 20), 1);
        av_ray(cx, cy, a, 32, 168, 2, RGB(30, 20, 6));
        break;
    case KN_CHROME:                                     /* silver chrome */
        av_cap(cx, cy, 11, RGB(255, 255, 255), RGB(96, 98, 108), RGB(60, 62, 70), 1);
        av_cap(cx, cy, 6, RGB(150, 152, 160), RGB(250, 250, 255), RGB(120, 122, 130), 0);   /* (the dished centre) */
        av_ray(cx, cy, a, 96, 172, 2, RGB(10, 10, 12));
        break;
    case KN_BLACK:                                      /* black, a white line */
        av_cap(cx, cy, 11, RGB(84, 84, 88), RGB(10, 10, 12), RGB(0, 0, 0), 0);
        av_ray(cx, cy, a, 48, 168, 2, RGB(250, 250, 250));
        break;
    default:                                            /* silver, a black outline */
        av_disc(cx, cy, 12, RGB(0, 0, 0));
        av_cap(cx, cy, 11, RGB(236, 238, 242), RGB(128, 132, 142), RGB(90, 94, 104), 0);
        av_ray(cx, cy, a, 32, 172, 2, RGB(0, 0, 0));
        break;
    }
}

/* text in the middle of cx, in colour c (FONT_S: one bit a pixel, crisp on any texture) */
static void av_text_c(int32_t cx, int32_t y, const char *s, uint16_t c)
{
    cv_plain = 0;                                       /* (its edges onto the panel under it) */
    cv_text(cx - text_w(&FONT_S, s) / 2, y, &FONT_S, s, c);
    cv_plain = 1;
}

/* the head into the canvas (its rows y0 .. y0 + cv_h - 1 of the screen) */
static void av_head(const track_t *t, int32_t y0, int on_amp2)
{
    int32_t type = t->p[P_AMP], k;
    cv_oy = -y0;
    if (type < 1) {                                     /* OFF: an empty stage */
        cv_rect(AV_HX0, AV_HY0, AV_HX1 - AV_HX0 + 1, AV_HY1 - AV_HY0 + 1, TE_G1);
        av_text_c(120, 112, "AMP OFF", C_HI);
        av_text_c(120, 136, "KNOB 1: AN AMP", C_DIM);
        av_text_c(120, 154, "(SYNTHS, KEYS: OFF)", C_DIM);
        cv_oy = 0;
        return;
    }
    {
        const amp_face_t *f = &AMP_FACES[type >= 5 ? 4 : type - 1];
        int dark = (((f->pan_top >> 11) & 31u) + ((f->pan_top >> 5) & 63u) / 2u) < 30u;
        av_cab(f);
        av_grille(f);
        if (f->grille == GR_PLATE) {                    /* the metal front: the name on a black plate */
            cv_rect(120 - 64, (AV_GY0 + AV_GY1) / 2 - 15, 129, 31, RGB(190, 190, 196));
            cv_rect(120 - 63, (AV_GY0 + AV_GY1) / 2 - 14, 127, 29, RGB(10, 10, 12));
            cv_rect(120 - 61, (AV_GY0 + AV_GY1) / 2 - 12, 123, 1, RGB(70, 70, 76));
            cv_rect(120 - 61, (AV_GY0 + AV_GY1) / 2 + 12, 123, 1, RGB(70, 70, 76));
        }
        av_logo(f, 120, (AV_GY0 + AV_GY1 + 1) / 2);
        av_disc(AV_X1 - 9, AV_GY0 + 9, 4, RGB(30, 10, 6));   /* the pilot light, in the grille's corner */
        av_disc(AV_X1 - 9, AV_GY0 + 9, 3, RGB(255, 64, 24));
        cv_pset(AV_X1 - 10, AV_GY0 + 8, RGB(255, 230, 200));
        av_panel(f);
        if (on_amp2) {                                  /* the four knobs turn the amp's: a frame round its panel */
            uint16_t fr = RGB(200, 200, 200);
            cv_rect(AV_X0 - 3, AV_PY0 - 3, AV_X1 - AV_X0 + 7, 1, fr), cv_rect(AV_X0 - 3, AV_PY1 + 3, AV_X1 - AV_X0 + 7, 1, fr);
            cv_rect(AV_X0 - 3, AV_PY0 - 2, 1, AV_PY1 - AV_PY0 + 5, fr), cv_rect(AV_X1 + 3, AV_PY0 - 2, 1, AV_PY1 - AV_PY0 + 5, fr);
        }
        for (k = 0; k < 4; k++) {                       /* GAIN BASS MID TREB on 1/8, 3/8, 5/8, 7/8 of the panel */
            uint32_t id;
            const param_desc_t *d;
            char val[12];
            const char *unit;
            int32_t cx = AV_X0 + (2 * k + 1) * (AV_X1 - AV_X0 + 1) / 8;
            uint16_t ink = f->ink;
            id = k == 0 ? P_AGAIN : k == 1 ? P_ABASS : k == 2 ? P_AMID : P_ATREB;
            d = track_desc(t, id);
            param_format(d, t->p[id], val, &unit);
            if (on_amp2 && (uint32_t)k == ui.hot_col && ui.hot_t)
                ink = dark ? RGB(255, 255, 255) : RGB(0, 0, 0);
            av_text_c(cx, AV_PY0 + 2, d->label, ink);
            av_knob(cx, AV_PY0 + 34, RATIO(d, t->p[id]), dark, f->knob, f->ink);
            av_text_c(cx, AV_PY0 + 51, val, ink);
        }
    }
    cv_oy = 0;
}

/* the top row: AMP CAB PRES COMP, lit on AMP (its knobs), dimmed on AMP 2 */
static void av_top(const track_t *t, int on_amp2)
{
    static const uint8_t ID[4] = {P_AMP, P_CAB, P_APRES, P_COMP};
    uint32_t k;
    cv_begin(240, 34, C_BLACK);
    for (k = 0; k < 4u; k++) {
        const param_desc_t *d = track_desc(t, ID[k]);
        char val[12];
        const char *unit;
        int32_t cx = 30 + (int32_t)k * 60;
        param_format(d, t->p[ID[k]], val, &unit);
        if (unit && unit[0] && str_len(val) + str_len(unit) < 11u)
            str_cpy(val + str_len(val), unit, 12 - str_len(val));
        av_text_c(cx, 1, d->label, C_GRAY);
        av_text_c(cx, 16, val, on_amp2 ? C_DIM : VAL(k));
        if (k)
            cv_rect((int32_t)k * 60, 4, 1, 26, C_LINE);
    }
    if (!on_amp2) {                                     /* the four knobs turn these: a frame round them */
        cv_rect(1, 0, 238, 1, C_GRAY), cv_rect(1, 33, 238, 1, C_GRAY);
        cv_rect(0, 1, 1, 32, C_GRAY), cv_rect(239, 1, 1, 32, C_GRAY);
    }
    cv_blit(0, 22);
}

UI_SMALL static void amp_view_draw(void)
{
    const track_t *t = TSEL;
    int on_amp2 = cur_page()->id[0] == P_AGAIN;
    uint32_t sig = 2166136261u, i;
    static const uint8_t ID[8] = {P_AMP, P_CAB, P_APRES, P_COMP, P_AGAIN, P_ABASS, P_AMID, P_ATREB};
    for (i = 0; i < 8u; i++)
        sig = (sig ^ (uint32_t)t->p[ID[i]]) * 16777619u;
    sig ^= (uint32_t)on_amp2 * 0x9E37u + (ui.hot_t ? ui.hot_col + 1u : 0u) * 0x51u + song.sel * 0x3301u + pal_bg;
    if (!ui.force && sig == av_sig)
        return;
    av_sig = sig;
    lcd_fill(0, H_HEAD, 240, 22 - H_HEAD, C_BLACK);
    av_top(t, on_amp2);
    lcd_fill(0, 56, 240, 2, C_BLACK);
    cv_begin(240, 124, C_BLACK);                        /* rows 58 .. 181 */
    av_head(t, AV_HY0, on_amp2);
    cv_blit(0, AV_HY0);
    cv_begin(240, AV_HY1 + 3 - (AV_HY0 + 124), C_BLACK);   /* rows 182 .. 219 (the footer's name row: from 220) */
    av_head(t, AV_HY0 + 124, on_amp2);
    cv_blit(0, AV_HY0 + 124);
}
