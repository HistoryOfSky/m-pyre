/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 History Of Sky (M-PYRE) */
/* M-PYRE: render a script through the firmware's own DSP (hostsim.c's sources), with the sound pack
 * (build/mpyre/soundpack.bin) in a RAM image of the user slots, as the FM-1 has it in flash.
 *   build/host/mpyre_render SCRIPT OUT.wav [SOUNDPACK]
 * SCRIPT: one command a line, '#' comments; times in seconds, events sorted by the renderer:
 *   len S                      the render's length
 *   bpm B                      the tempo (the delay)
 *   eng TRACK ENGINE PRESET    a factory sound on track 0..2 (engine index as ENGINES[], preset index)
 *   p TRACK PARAM VALUE        a track parameter (its index in core.h's enum)
 *   g PARAM VALUE              a global parameter
 *   on T TRACK NOTE VEL        a note on at T s
 *   off T TRACK NOTE           a note off
 *   pset T TRACK PARAM VALUE   a track parameter at T s (live knobs)
 *   drum T NOTE VEL            a drum hit (GM note)
 */
#include <stdint.h>
static uint32_t host_slots[4u * 0x14000u / 4u];
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#define main hostsim_main
#include "hostsim.c"
#undef main

typedef struct { double t; int kind, a, b, c; } ev_t;   /* kind: 0 on, 1 off, 2 pset, 3 drum */
static ev_t ev[20000];
static int nev;
static int ev_cmp(const void *x, const void *y)
{
    const ev_t *a = x, *b = y;
    return a->t < b->t ? -1 : a->t > b->t ? 1 : a->kind - b->kind;
}

int main(int argc, char **argv)
{
    FILE *f, *w;
    char line[256];
    double len = 4.0, t;
    int i, k;
    if (argc < 3)
        return fprintf(stderr, "usage: mpyre_render SCRIPT OUT.wav [SOUNDPACK]\n"), 2;
    f = fopen(argc > 3 ? argv[3] : "build/mpyre/soundpack.bin", "rb");
    if (f) {
        fread(host_slots, 1, sizeof host_slots, f);
        fclose(f);
    } else
        fprintf(stderr, "mpyre_render: no sound pack, the slots are empty\n");
    host_tracks_init();
    for (k = 0; k < SMP_USER_SLOTS; k++)
        smp_user_scan((uint32_t)k);
    if (!(f = fopen(argv[1], "r")))
        return perror(argv[1]), 1;
    while (fgets(line, sizeof line, f)) {
        char cmd[16];
        double a = 0;
        int x = 0, y = 0, z = 0, q = 0;
        if (line[0] == '#' || sscanf(line, "%15s", cmd) != 1)
            continue;
        if (!strcmp(cmd, "len"))
            sscanf(line, "%*s %lf", &len);
        else if (!strcmp(cmd, "bpm"))
            sscanf(line, "%*s %d", &x), song.g[G_BPM] = (int16_t)x;
        else if (!strcmp(cmd, "eng")) {
            sscanf(line, "%*s %d %d %d", &x, &y, &z);
            host_preset(&trk[x], (uint32_t)y, (uint32_t)z);
        } else if (!strcmp(cmd, "p"))
            sscanf(line, "%*s %d %d %d", &x, &y, &z), trk[x].p[y] = (int16_t)z;
        else if (!strcmp(cmd, "g"))
            sscanf(line, "%*s %d %d", &y, &z), song.g[y] = (int16_t)z;
        else if (!strcmp(cmd, "on"))
            sscanf(line, "%*s %lf %d %d %d", &a, &x, &y, &z), ev[nev++] = (ev_t){a, 0, x, y, z};
        else if (!strcmp(cmd, "off"))
            sscanf(line, "%*s %lf %d %d", &a, &x, &y), ev[nev++] = (ev_t){a, 1, x, y, 0};
        else if (!strcmp(cmd, "pset"))
            sscanf(line, "%*s %lf %d %d %d", &a, &x, &y, &q), ev[nev++] = (ev_t){a, 2, x, y, q};
        else if (!strcmp(cmd, "drum"))
            sscanf(line, "%*s %lf %d %d", &a, &x, &y), ev[nev++] = (ev_t){a, 3, x, y, 0};
    }
    fclose(f);
    qsort(ev, (size_t)nev, sizeof ev[0], ev_cmp);
    w = fopen(argv[2], "wb");
    {
        uint32_t frames = (uint32_t)(len * FS) / CTL * CTL, n, e = 0;
        int32_t o[2 * CTL];
        wav_hdr(w, frames);
        for (n = 0; n < frames; n += CTL) {
            t = (double)n / FS;
            while ((int)e < nev && ev[e].t <= t) {
                ev_t *v = &ev[e++];
                if (v->kind == 0)
                    trk_note_on(&trk[v->a], (uint32_t)v->b, (uint32_t)v->c);
                else if (v->kind == 1)
                    trk_note_off(&trk[v->a], (uint32_t)v->b);
                else if (v->kind == 2)
                    trk[v->a].p[v->b] = (int16_t)v->c;
                else
                    drum_on((uint32_t)v->a, (uint32_t)v->b);
            }
            mix_block(o, CTL);
            for (i = 0; i < CTL; i++)
                wav_put(w, o[2 * i], o[2 * i + 1]);
        }
    }
    fclose(w);
    return 0;
}
