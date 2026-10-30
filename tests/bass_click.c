/* SPDX-License-Identifier: GPL-3.0-only */
/* M-PYRE: the PICK BASS as the sequencer plays it (8ths, the gate short of the next note) at a tempo: a WAV, to
 * hear and measure the clicks between notes (tools: an onset's high band against the note's).
 *   cc -O1 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o build/host/bass_click tests/bass_click.c -lm
 *   build/host/bass_click OUT.wav BPM [PRESET] [GATE %: 100 = each note to the next] [GAIN] [TREB] */
#include <stdint.h>
static uint32_t host_slots[4u * 0x14000u / 4u];
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#define main hostsim_main
#include "hostsim.c"
#undef main

int main(int argc, char **argv)
{
    static const uint8_t LINE[16] = {40, 40, 40, 40, 43, 43, 40, 40, 45, 45, 43, 43, 40, 40, 38, 38};
    FILE *f, *sp = fopen("build/mpyre/soundpack.bin", "rb");
    uint32_t k, e = 0, pi = 0, bpm = argc > 2 ? (uint32_t)atoi(argv[2]) : 90u, b, i, step, frames = 0, n = 0, on = 0;
    const char *want = argc > 3 ? argv[3] : "PICK BASS";
    uint32_t gate = argc > 4 ? (uint32_t)atoi(argv[4]) : 85u;
    int32_t o[2 * CTL];
    if (sp) { if (fread(host_slots, 1, sizeof host_slots, sp)) {} fclose(sp); }
    for (k = 0; k < SMP_USER_SLOTS; k++) smp_user_scan(k);
    for (k = 0; k < NENGINES; k++)
        for (i = 0; i < ENGINES[k]->npresets; i++)
            if (!strcmp(ENGINES[k]->presets[i].name, want)) { e = k; pi = i; }
    host_tracks_init();
    song.g[G_BPM] = (uint8_t)bpm;
    host_preset(&trk[0], e, pi);
    song.sel = 0;
    if (argc > 5) trk[0].p[P_AGAIN] = (uint8_t)atoi(argv[5]);   /* (GAIN, TREB: the amp's range) */
    if (argc > 6) trk[0].p[P_ATREB] = (uint8_t)atoi(argv[6]);
    for (b = 0; b < FS / CTL; b++) mix_block(o, CTL);
    if (!(f = fopen(argv[1], "wb"))) return 1;
    wav_hdr(f, 0);
    step = FS * 60u / bpm / 2u;                          /* 8ths */
    for (b = 0; b < 2u * 16u * step / CTL; b++) {
        uint32_t t0 = b * CTL, s = t0 / step;
        if (gate >= 100u && on && t0 % step < CTL) { trk_note_off(&trk[0], on); on = 0; }
        if (s < 32u && t0 % step < CTL) { trk_note_on(&trk[0], LINE[s % 16u], 110); on = LINE[s % 16u]; }
        if (on && t0 % step >= step * gate / 100u && t0 % step < step * gate / 100u + CTL) { trk_note_off(&trk[0], on); on = 0; }
        mix_block(o, CTL);
        {
            int16_t w[2 * CTL];
            for (i = 0; i < 2u * CTL; i++) w[i] = (int16_t)clamp(o[i], -32768, 32767);
            fwrite(w, 2, 2u * CTL, f);
        }
        frames += CTL;
    }
    (void)n;
    fseek(f, 0, SEEK_SET);
    wav_hdr(f, frames);
    fclose(f);
    printf("%s at %u bpm: %u frames\n", want, bpm, frames);
    return 0;
}
