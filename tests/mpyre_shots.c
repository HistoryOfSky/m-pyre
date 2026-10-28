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
    settings.palette = 4;
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
    {   /* the FX section's guitar pages */
        static const char *const T[4] = {"AMP", "AMP 2", "PEDAL", "OCTAVER"};
        for (g = 0; g < 4u; g++) {
            for (i = 0; i < NPAGES; i++)
                if (PAGES[i].fam == FAM_FX && !strcmp(PAGES[i].title, T[g]))
                    break;
            ui.home = 0; ui.page = (uint8_t)i; ui.force = 1; frames(3);
            snprintf(nm, sizeof nm, "mp-fx-%u", g + 1u); ppm(nm);
        }
    }
    { int m = 0; for (i = 0; i < SCOPE_N; i++) if (abs(scope_buf[i]) > m) m = abs(scope_buf[i]); printf("scope peak %d\n", m); }
    printf("ok\n");
    return 0;
}
