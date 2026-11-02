/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 History Of Sky (M-PYRE) */
/* M-PYRE: the demo songs (tools/demo_songs.py -> build/demo/songs.h) built as projects on the host: a .mpyre file
 * each (the project's bytes: the emulator's SONG LADEN, the FM-1's editor) and a WAV of each, two times round.
 *   python3 tools/demo_songs.py build/demo/songs.h
 *   cc -O2 -w -Ibuild/gen -Ibuild/demo -Ifirmware/src -Ifirmware/hal -o build/host/demo_songs tests/demo_songs.c -lm
 *   build/host/demo_songs OUTDIR [LOOPS] */
#include <stdint.h>
static uint32_t host_slots[4u * 0x14000u / 4u];
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { static const uint8_t E[NPART] = {4, 4, 4}; return i < NPART ? E[i] : 0u; }
#include "../firmware/src/project.c"
#include "songs.h"

static int find_preset(const char *name, uint32_t *e, uint32_t *pi)
{
    uint32_t k, i;
    for (k = 0; k < NENGINES; k++)                     /* (the sample presets first: the guitars, the bass) */
        for (i = 0; i < ENGINES[(k + 4u) % NENGINES]->npresets; i++)
            if (!strcmp(ENGINES[(k + 4u) % NENGINES]->presets[i].name, name)) {
                *e = (k + 4u) % NENGINES, *pi = i;
                return 1;
            }
    return 0;
}

static void build(const dm_song_t *s)
{
    uint32_t k, i;
    host_tracks_init();
    song.g[G_BPM] = s->bpm;
    song.g[G_DTIME] = s->dly[0], song.g[G_DFDBK] = s->dly[1], song.g[G_DCOLOR] = s->dly[2];
    song.g[G_RTYPE] = s->rev[0], song.g[G_RSIZE] = s->rev[1], song.g[G_RDAMP] = s->rev[2];
    for (k = 0; k < NPART; k++) {
        const dm_track_t *dt = &s->t[k];
        track_t *t = &trk[k];
        uint32_t e = 4, pi = 0;
        if (!find_preset(dt->preset, &e, &pi))
            fprintf(stderr, "%s: no preset %s\n", s->name, dt->preset);
        host_preset(t, e, pi);
        t->p[P_SLEN] = s->slen;
        t->p[P_SDIV] = s->div;
        for (i = 0; i < dt->npar; i++)
            t->p[dt->par[i][0]] = dt->par[i][1];
        steps_clear(t);
        for (i = 0; i < s->slen; i++) {
            const dm_step_t *d = &dt->st[i];
            step_t *st = &t->step[i];
            st->time = d->time == 0 ? ST_NOTE : d->time == 1 ? ST_TIE : ST_REST;
            if (d->time == 0) {
                memcpy(st->note, d->note, 4);
                st->n = d->n;
                st->vel = 100;
                st->flags = d->acc ? SF_ACCENT : 0;
                if (d->palm)
                    lock_set(t, i, P_PALM, 1);
            }
        }
    }
    TDRUM->p[P_E0] = s->kit;
    TDRUM->p[P_SLEN] = s->slen;
    TDRUM->p[P_SDIV] = s->div;
    steps_clear(TDRUM);
    for (i = 0; i < s->slen; i++)
        for (k = 0; k < DRUM_LANES; k++)
            if ((s->drum[i] >> k) & 1u)
                dstep_set(&TDRUM->dstep[i], k, (s->hard[i] >> k) & 1u ? LV_HARD : (s->ghost[i] >> k) & 1u ? LV_GHOST : LV_NORM, 0);
    song.sel = 0;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "build/demo";
    uint32_t loops = argc > 2 ? (uint32_t)atoi(argv[2]) : 2u, s, k, n;
    FILE *f = fopen("build/mpyre/soundpack.bin", "rb");
    static project_t p;
    if (f) { if (fread(host_slots, 1, sizeof host_slots, f)) {} fclose(f); }
    for (k = 0; k < SMP_USER_SLOTS; k++)
        smp_user_scan(k);
    for (s = 0; s < DM_N; s++) {
        const dm_song_t *d = &DM_SONGS[s];
        char path[512], base[64];
        uint32_t frames, steps_per_beat = d->div == 1 ? 2u : 4u;
        for (k = 0; d->name[k] && k < 60u; k++)
            base[k] = d->name[k] == ' ' ? '-' : d->name[k];
        base[k] = 0;
        build(d);
        for (k = 0; k < NTRK; k++)
            trk[k].engine = trk[k].eng_req;
        proj_capture(&p);
        snprintf(path, sizeof path, "%s/%02u-%s.mpyre", dir, s + 1u, base);
        if ((f = fopen(path, "wb"))) { fwrite(&p, sizeof p, 1, f); fclose(f); }
        {   /* the sounds settle, then the song: LOOPS times round, and a second of its tail */
            static int32_t o[CTL * 2];
            int16_t w[CTL * 2];
            for (n = 0; n < FS / CTL / 2u; n++) mix_block(o, CTL);
            frames = (uint32_t)((uint64_t)FS * 60u * d->slen * loops / steps_per_beat / d->bpm) + FS;
            snprintf(path, sizeof path, "%s/%02u-%s.wav", dir, s + 1u, base);
            if (!(f = fopen(path, "wb"))) return 1;
            wav_hdr(f, frames);
            transport_req = 1;
            for (n = 0; n < frames; n += CTL) {
                uint32_t i;
                if (n + FS >= frames && n < frames - FS + CTL)
                    transport_req = 2;                  /* (stop for the last second: the tails) */
                mix_block(o, CTL);
                for (i = 0; i < CTL * 2u; i++) w[i] = (int16_t)clamp(o[i], -32768, 32767);
                fwrite(w, 2, CTL * 2u, f);
            }
            fclose(f);
            transport_req = 2;
            for (n = 0; n < 50u; n++) mix_block(o, CTL);
        }
        printf("%02u %-14s %-18s %3u BPM  %.1f s\n", s + 1u, d->name, d->style, d->bpm, frames / (double)FS);
    }
    return 0;
}
