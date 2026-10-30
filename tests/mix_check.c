/* SPDX-License-Identifier: GPL-3.0-only */
/* M-PYRE: the default song's parts as a rock song plays them (a riff in 8ths on the guitar, the bass with it, a lead
 * line over it, the drums' beat), 4 bars at 100 BPM, each part alone and all together: raw stereo int16 files, for
 * the levels of the sample presets in the mix (tools: loudness per part).
 *   cc -O1 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o build/host/mix_check tests/mix_check.c -lm
 *   build/host/mix_check OUTDIR */
#include <stdint.h>
static uint32_t host_slots[4u * 0x14000u / 4u];
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static void run(const char *dir, const char *name, uint32_t mask)
{
    static const uint8_t RIFF[8] = {40, 40, 40, 43, 40, 40, 45, 43}, LEAD[8] = {64, 0, 67, 0, 69, 67, 64, 0};
    char path[512];
    FILE *f;
    uint32_t i, k, b, step = FS * 60u / 100u / 2u, on[NTRK] = {0};
    int32_t o[CTL * 2];
    host_tracks_init();
    for (i = 0; i < NPART; i++) { set_engine_of(&trk[i], TRK_DEF[i][0]); apply_preset_to(&trk[i], TRK_DEF[i][1]); trk[i].engine = trk[i].eng_req; }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    song.g[G_BPM] = 100;
    for (b = 0; b < FS / CTL; b++) mix_block(o, CTL);
    snprintf(path, sizeof path, "%s/%s.raw", dir, name);
    if (!(f = fopen(path, "wb"))) return;
    for (b = 0; b < 32u * step / CTL; b++) {
        uint32_t t0 = b * CTL, s = t0 / step;
        if (t0 % step < CTL) {
            for (k = 0; k < NTRK; k++)
                if (on[k]) { trk_note_off(&trk[k], on[k]); on[k] = 0; }
            if (mask & 1u) { trk_note_on(&trk[0], RIFF[s % 8u] + (uint32_t)trk[0].p[P_TRANS] + 12u, 110); on[0] = RIFF[s % 8u] + (uint32_t)trk[0].p[P_TRANS] + 12u; }
            if ((mask & 2u) && LEAD[s % 8u]) { trk_note_on(&trk[1], LEAD[s % 8u], 110); on[1] = LEAD[s % 8u]; }
            if (mask & 4u) { on[2] = (uint32_t)((int32_t)RIFF[s % 8u] + 12 + trk[2].p[P_TRANS]); trk_note_on(&trk[2], on[2], 110); }
            if (mask & 8u) {
                trk_note_on(TDRUM, 42, 90);
                if (s % 4u == 0u || s % 8u == 5u) trk_note_on(TDRUM, 36, 120);
                if (s % 4u == 2u) trk_note_on(TDRUM, 38, 120);
            }
        }
        mix_block(o, CTL);
        {
            int16_t w[CTL * 2];
            for (i = 0; i < CTL * 2u; i++) w[i] = (int16_t)clamp(o[i], -32768, 32767);
            fwrite(w, 2, CTL * 2u, f);
        }
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "build/host";
    FILE *sp = fopen("build/mpyre/soundpack.bin", "rb");
    uint32_t i;
    if (sp) { if (fread(host_slots, 1, sizeof host_slots, sp)) {} fclose(sp); }
    for (i = 0; i < SMP_USER_SLOTS; i++) smp_user_scan(i);
    run(dir, "gtr", 1); run(dir, "lead", 2); run(dir, "bass", 4); run(dir, "drums", 8); run(dir, "mix", 15);
    printf("TRN gtr %d lead %d bass %d  LEVEL %d %d %d\n", trk[0].p[P_TRANS], trk[1].p[P_TRANS], trk[2].p[P_TRANS],
           trk[0].p[P_LEVEL], trk[1].p[P_LEVEL], trk[2].p[P_LEVEL]);
    return 0;
}
