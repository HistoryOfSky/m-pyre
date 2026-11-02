/* SPDX-License-Identifier: GPL-3.0-only */
/* Modified for M-PYRE by History Of Sky, 2026 (CHANGELOG.md) */
/* The screen fonts (gfx.c cv_text, tools/gen_font.py). M-PYRE: Barlow Condensed - S SemiBold anti-aliased (4 bits) in
 * the 16-row cell, L Bold at 1 bit in the 32-row cell, capitals 32..95 only. Checked: every glyph of both in several
 * colours, at several offsets (clipped at every edge, and with a graph y offset), hashed (the pixels pinned); every
 * capital, figure and sign draws; lower case and the umlauts as their capitals; tabular figures (values do not jump);
 * the cells' heights; the flash they take. Exit status: the number of failed checks.
 *   build/host/font_test x  prints the hashes */
#include <stdint.h>
#define FONT_S_HASH 0x6cf09671u                 /* Barlow Condensed SemiBold 16 px (M-PYRE) */
#define FONT_L_HASH 0x7c84cc7bu                 /* Barlow Condensed Bold 31 px */
#define FONT_W_HASH 0x3d1bc830u                 /* their widths */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p) { (void)x; (void)y; (void)w; (void)h; (void)p; }
#define __attribute__(x)         /* (as tests/hostsim.c: gfx.c puts its canvas in the pool section, which clang on
                                 * macOS refuses for a host build; sloop-lab, 11/10/2026) */
#include "../firmware/src/gfx.c"
static int bad;
static void check(int ok, const char *what) { printf("font: %-70s %s\n", what, ok ? "ok" : "FAIL"); bad += !ok; }
static uint32_t hash_cv(uint32_t h)
{
    uint32_t i;
    for (i = 0; i < cv_w * cv_h; i++)
        h = (h ^ cv_px[i]) * 16777619u;
    return h;
}
static uint32_t render_all(const felucca_font_t *f, uint32_t ch_hi)
{
    static const uint16_t COL[4] = {0xFFFFu, 0x07E0u, 0xF81Fu, 0x39E7u};
    static const int32_t XY[6][3] = {{0, 0, 0}, {5, 3, 0}, {-3, 0, 0}, {50, -5, 0}, {52, 30, 0}, {10, 4, -9}};
    uint32_t h = 0x811C9DC5u, c, k, i;
    char s[2] = {0, 0};
    for (c = f->first; c <= ch_hi; c++)
        for (k = 0; k < 4u; k++)
            for (i = 0; i < 6u; i++) {
                cv_begin(64, 48, 0);
                cv_oy = XY[i][2];
                s[0] = (char)c;
                cv_text(XY[i][0], XY[i][1], f, s, COL[k]);
                cv_oy = 0;
                h = hash_cv(h);
            }
    cv_begin(240, 40, 0);                                   /* a line of text, as the pages draw it */
    cv_text(2, 2, &FONT_S, "SLOOP 2.5 HUGELTON BPM 96 -12.5 dB", 0xFFFFu);
    cv_text(2, 18, &FONT_S, "\xdc\xe4 kick snare", 0x07E0u);
    return hash_cv(h);
}
int main(int argc, char **argv)
{
    uint32_t hs = render_all(&FONT_S, 95u), hl = render_all(&FONT_L, 95u), wsum = 0, c, empty = 0;
    for (c = 0; c < 64u; c++) wsum = wsum * 31u + FONT_S.adv[c] * 7u + FONT_S.bw[c];
    for (c = 0; c < 64u; c++) wsum = wsum * 31u + FONT_L.adv[c] * 7u + FONT_L.bw[c];
    if (argc > 1) {
        printf("%08x %08x %08x %u %u\n", hs, hl, wsum, (unsigned)sizeof FONT_S_DATA, (unsigned)sizeof FONT_L_DATA);
        return 0;
    }
    for (c = 33; c < 96u; c++) {                        /* every capital, figure and sign draws something */
        char s[2] = {(char)c, 0};
        uint32_t h0, f;
        cv_begin(40, 32, 0);
        h0 = hash_cv(7u);
        for (f = 0; f < 2u; f++) {
            cv_begin(40, 32, 0);
            cv_text(4, 0, f ? &FONT_L : &FONT_S, s, 0xFFFFu);
            if (hash_cv(7u) == h0) {
                printf("font: '%c' empty in %s\n", (int)c, f ? "L" : "S");
                empty++;
            }
        }
    }
    check(!empty, "every character 33..95 draws, in S and L");
    {
        uint32_t x;
        cv_begin(240, 16, 0); cv_text(0, 0, &FONT_S, "kick snare h\xfcgelton", 0xFFFFu); x = hash_cv(1u);
        cv_begin(240, 16, 0); cv_text(0, 0, &FONT_S, "KICK SNARE HUGELTON", 0xFFFFu);
        check(x == hash_cv(1u) && text_w(&FONT_S, "kick") == text_w(&FONT_S, "KICK"),
              "lower case shows as capitals, the umlauts as their letters, in the same place");
    }
    {
        int same = 1;
        for (c = '1'; c <= '9'; c++)
            same &= FONT_S.adv[c - 32u] == FONT_S.adv['0' - 32u] && FONT_L.adv[c - 32u] == FONT_L.adv['0' - 32u];
        check(same, "tabular figures: every digit as wide (a value does not jump as it changes)");
    }
    check(FONT_S.h == 16u && FONT_L.h == 32u && FONT_S.bits == 0u && FONT_L.bits == 1u && FONT_S.last == 95u &&
          FONT_L.last == 95u, "S anti-aliased in the 16-row cell, L 1 bit in the 32-row cell, 32..95");
    check(!FONT_S_HASH || hs == FONT_S_HASH, "FONT_S: every glyph, 4 colours, 6 places (clipped)");
    check(!FONT_L_HASH || hl == FONT_L_HASH, "FONT_L: every glyph, 4 colours, 6 places (clipped)");
    check(!FONT_W_HASH || wsum == FONT_W_HASH, "advance and bitmap widths (the layout)");
    check(sizeof FONT_S_DATA + sizeof FONT_L_DATA <= 10500u, "the two fonts' bitmaps in 10.5 KB of flash");
    printf("%s\n", bad ? "FONT TEST FAILED" : "font test passed");
    return bad;
}
