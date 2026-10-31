/* SPDX-License-Identifier: GPL-3.0-only */
/* M-PYRE: the LIVE view's groups (guitar track playing) and the TRACKS screen to PPM */
#include <stdint.h>
static uint32_t host_slots[4u * 0x14000u / 4u];
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(int argc, char **argv)
{
    uint32_t i, g;
    char nm[64];
    outdir = argc > 1 ? argv[1] : "build/host";
    panel = PANEL_DEFAULT;
    layers_init();
    settings.palette = 4;                            /* GLUT */
    palette_set(4);
    { FILE *f = fopen(argc > 2 ? argv[2] : "build/mpyre/soundpack.bin", "rb"); if (f) { fread(host_slots, 1, sizeof host_slots, f); fclose(f); } }
    host_tracks_init();
    for (i = 0; i < SMP_USER_SLOTS; i++) smp_user_scan(i);
    for (i = 0; i < NPART; i++) { set_engine_of(&trk[i], TRK_DEF[i][0]); apply_preset_to(&trk[i], TRK_DEF[i][1]); trk[i].engine = trk[i].eng_req; }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    go_home(); ui.force = 1; frame();
    tap(B_HOME); frames(4);
    fm1_in.notes |= 1u << 0; fm1_in.notes |= 1u << 7;    /* a fifth held: the scope moves */
    frames(30);
    for (g = 0; g < NLIVE; g++) {
        ui.force = 1; frames(3);
        snprintf(nm, sizeof nm, "mp-live-%u", g + 1u); ppm(nm);
        tap(B_HOME); frames(2);
    }
    { int m = 0; for (i = 0; i < SCOPE_N; i++) if (abs(scope_buf[i]) > m) m = abs(scope_buf[i]); printf("held peak %d\n", m); }
    fm1_in.notes = 0; frames(4);
    press(B_HOME); frames(60); release(B_HOME); frames(3); ppm("mp-hold-tracks");
    {   /* M-PYRE: the AMP view (EDIT > AMP, AMP 2), each amp; then the PEDAL page; then the BEIGE palette */
        uint32_t pa = 0, pb = 0, pp = 0;
        for (i = 0; i < NPAGES; i++) {
            if (PAGES[i].fam == FAM_EDIT && !strcmp(PAGES[i].title, "AMP")) pa = i;
            if (PAGES[i].fam == FAM_EDIT && !strcmp(PAGES[i].title, "AMP 2")) pb = i;
            if (PAGES[i].fam == FAM_FX && !strcmp(PAGES[i].title, "PEDAL")) pp = i;
        }
        for (g = 0; g <= 5u; g++) {
            TSEL->p[P_AMP] = (int16_t)g;
            ui.home = 0; ui.page = (uint8_t)(g & 1u ? pa : pb); ui.force = 1; frames(3);
            snprintf(nm, sizeof nm, "mp-amp-%u", g); ppm(nm);
        }
        ui.page = (uint8_t)pp; ui.force = 1; frames(3); ppm("mp-pedal");
        for (g = 0; g <= 5u; g++) {                    /* the pedals: OFF BOOST OD DIST FUZZ OCT */
            TSEL->p[P_PEDAL] = (int16_t)g; TSEL->p[P_PDRV] = (int16_t)(30 + g * 15);
            ui.page = (uint8_t)pp; ui.force = 1; frames(3);
            snprintf(nm, sizeof nm, "mp-ped-%u", g); ppm(nm);
        }
        TSEL->p[P_PEDAL] = 0;
        for (i = 0; i < NPAGES; i++)                   /* MOD (each kind), DELAY, REVERB */
            if (PAGES[i].fam == FAM_FX && !strcmp(PAGES[i].title, "MOD")) {
                for (g = 1; g <= 4u; g++) {
                    TSEL->p[P_ITYPE] = (int16_t)g;
                    ui.page = (uint8_t)i; ui.force = 1; frames(3);
                    snprintf(nm, sizeof nm, "mp-mod-%u", g); ppm(nm);
                }
                TSEL->p[P_ITYPE] = 0;
            } else if (PAGES[i].fam == FAM_FX && (!strcmp(PAGES[i].title, "DELAY") || !strcmp(PAGES[i].title, "REVERB") ||
                                                   !strcmp(PAGES[i].title, "FX"))) {
                ui.page = (uint8_t)i; ui.force = 1; frames(3);
                snprintf(nm, sizeof nm, "mp-fxp-%s", PAGES[i].title); ppm(nm);
            }
        TSEL->p[P_AMP] = 2;
        settings.palette = 3; palette_set(3);
        ui.page = (uint8_t)pa; ui.force = 1; frames(3); ppm("mp-amp-beige");
        go_home(); ui.force = 1; frames(3); ppm("mp-live-beige");
        settings.palette = 2; palette_set(2);
    }
    {   /* M-PYRE: the DRUMS screen's sound editor (EDIT), the snare, then the ride's bell; the drums' LIVE view, hits lit */
        song.sel = TRK_DRUM; studio_open(SC_DRUM); drum_page = 2; drum_lane = 1; drum_lvl[1] = 2; drum_punch[1] = 2;
        ui.force = 1; frames(3); ppm("mp-kit-1");
        drum_lane = 7; drum_lvl[7] = -2; drum_punch[7] = 3; ui.force = 1; frames(3); ppm("mp-kit-2");
        drum_lane = 0; ui.force = 1; frames(3); ppm("mp-kit-3");
        go_live(); ui.force = 1; frames(2);
        pad_lit[0] = 6; pad_lit[1] = 2; pad_lit[11] = 6; ui.force = 1; frame(); ppm("mp-kit-live");
    }
    { int m = 0; for (i = 0; i < SCOPE_N; i++) if (abs(scope_buf[i]) > m) m = abs(scope_buf[i]); printf("scope peak %d\n", m); }
    printf("ok\n");
    return 0;
}
