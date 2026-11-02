/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 History Of Sky (M-PYRE) */
/* M-PYRE: a song exported from the browser emulator (SONG EXPORTIEREN: project.c's bytes) played on the host, the
 * sound pack in the user slots: a WAV of it and what each track holds (sounds, settings, the steps).
 *   cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/song_render tests/song_render.c -lm
 *   build/host/song_render SONG.mpyre OUT.wav [BARS] */
#include <stdint.h>
static uint32_t host_slots[4u * 0x14000u / 4u];
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { static const uint8_t E[NPART] = {4, 4, 4}; return i < NPART ? E[i] : 0u; }
#include "../firmware/src/project.c"

int main(int argc, char **argv)
{
    static project_t p;
    static uint8_t buf[sizeof p + 16];
    FILE *f;
    uint32_t i, k, n, bars = argc > 3 ? (uint32_t)atoi(argv[3]) : 4u, frames;
    if (argc < 3) { fprintf(stderr, "song_render SONG.mpyre OUT.wav [BARS]\n"); return 2; }
    if ((f = fopen("build/mpyre/soundpack.bin", "rb"))) { if (fread(host_slots, 1, sizeof host_slots, f)) {} fclose(f); }
    host_tracks_init();
    for (i = 0; i < SMP_USER_SLOTS; i++) smp_user_scan(i);
    if (!(f = fopen(argv[1], "rb"))) return 1;
    n = (uint32_t)fread(buf, 1, sizeof buf, f);
    fclose(f);
    if (!proj_import(&p, buf, (int)n)) { fprintf(stderr, "not a project (%u bytes, want %u)\n", n, (unsigned)sizeof p); return 1; }
    proj_apply(&p, 1);
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        const engine_t *e = ENGINES[t->eng_req % NENGINES];
        uint32_t used = 0;
        t->engine = t->eng_req;
        for (i = 0; i < NSTEP; i++) used += k == TRK_DRUM ? dstep_mask(&t->dstep[i]) != 0 : t->step[i].n != 0 && t->step[i].time == ST_NOTE;
        printf("track %u: %s %s  len %d div %d gate %d  level %d  amp %d gain %d  oct %d omix %d  palm %d  voice %d trans %d  "
               "atk %d dec %d sus %d rel %d  steps with notes %u\n",
               k + 1u, k == TRK_DRUM ? "DRUMS" : e->name, k == TRK_DRUM ? DRUM_KIT_NAMES[drum_kit()] : e->presets[t->preset % e->npresets].name,
               t->p[P_SLEN], t->p[P_SDIV], t->p[P_SGATE], t->p[P_LEVEL], t->p[P_AMP], t->p[P_AGAIN], t->p[P_OCT] - 24, t->p[P_OMIX],
               t->p[P_PALM], t->p[P_VOICE], t->p[P_TRANS], t->p[P_ATK], t->p[P_DEC], t->p[P_SUS], t->p[P_REL], used);
        if (k != TRK_DRUM)
            for (i = 0; i < (uint32_t)t->p[P_SLEN] && i < 32u; i++) {
                const step_t *s = &t->step[i];
                printf(i % 16u ? " " : "   ");
                if (s->time == ST_TIE) printf("--");
                else if (!s->n || s->time == ST_REST) printf(" .");
                else printf("%2u", s->note[0]);
                if (i % 16u == 15u) printf("\n");
            }
        printf("\n");
    }
    printf("bpm %d swing %d\n", song.g[G_BPM], song.g[G_SWING]);
    frames = (uint32_t)((uint64_t)FS * 240u * bars / (uint32_t)song.g[G_BPM]);
    if (!(f = fopen(argv[2], "wb"))) return 1;
    wav_hdr(f, frames);
    transport_req = 1;
    for (n = 0; n < frames; n += CTL) {
        static int32_t o[CTL * 2];
        int16_t w[CTL * 2];
        mix_block(o, CTL);
        for (i = 0; i < CTL * 2u; i++) w[i] = (int16_t)clamp(o[i], -32768, 32767);
        fwrite(w, 2, CTL * 2u, f);
    }
    fclose(f);
    printf("wrote %s: %u bars, %.1f s\n", argv[2], bars, frames / (double)FS);
    return 0;
}
