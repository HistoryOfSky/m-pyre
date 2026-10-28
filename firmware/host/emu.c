/* SPDX-License-Identifier: GPL-3.0-only */
/* M-PYRE browser emulator: the whole firmware (DSP, sequencer, UI) as one WebAssembly module, the FM-1's
 * hardware replaced by RAM: the screen a 240 x 240 RGB565 buffer, the keys / buttons / knobs set from
 * JavaScript, the user sample slots in RAM (the sound pack loaded into them). The audio runs in an
 * AudioWorklet; every 16 ms of audio a UI frame runs (ui_input, ui_leds, ui_draw), as the FM-1's main loop.
 * Projects and user presets are not kept (the emulator saves nothing). Built by tools/build_emu.py. */
#include <stdint.h>
#include <string.h>
#define memset felucca_memset
#define memcpy felucca_memcpy
#define memcmp felucca_memcmp
#include "felucca_tables.h"
#include "../src/libc.c"
#undef memset
#undef memcpy
#undef memcmp
#define FELUCCA_ARRANGER 1
#define __attribute__(x)
static uint8_t user_flash[4][0x14000];
#define SMP_USER_XIP(k) (user_flash[(k) & 3u])
static struct { volatile uint32_t notes, buttons; } fm1_in;
static volatile uint32_t fm1_remote_btn;
static volatile uint32_t fm1_ms;
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
#include "../src/core.h"
#include "../src/cpu1.h"                        /* no second core in the browser: every unit renders here */
static int cpu1_run(cpu1_job_t fn, uint32_t arg) { (void)fn; (void)arg; return 0; }
static int cpu1_wait(void) { return 0; }
static int cpu1_on(void) { return 0; }
static uint32_t cpu1_ticks(void) { return 0; }
#include "../src/engines.c"
#include "../src/drums.c"
#include "../src/params.c"
#include "../src/voice.c"
#include "../src/slicer.c"
#include "../src/fx.c"
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#include "../src/usb.c"
#include "../src/midi_uart.c"
#include "../src/arranger.c"
#include "../src/seq.c"

/* ---- the panel's hardware */
static uint16_t screen[240 * 240];                 /* RGB565, as the panel takes it (bytes swapped) */
static uint32_t screen_dirty;
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    uint32_t i, j;
    if (x + w > 240u || y + h > 240u) return;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            screen[(y + j) * 240u + x + i] = p[j * w + i];
    screen_dirty = 1;
}
#include "../src/gfx.c"
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    if (x + w > 240u || y + h > 240u) return;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            screen[(y + j) * 240u + x + i] = swap16(c);
    screen_dirty = 1;
}
static int32_t encs[7];
static uint32_t fm1_ticks(void) { return fm1_ms * 1000u * 24u; }
#define FM1_TICKS_PER_US 24u
static int32_t fm1_enc_take(uint32_t e) { int32_t s = encs[e]; encs[e] = 0; return s; }
static uint8_t fm1_led[16], fm1_led_dim[16], fm1_led_bg[16];
static volatile uint16_t fm1_led_bg_ns;
#define FM1_NCOL 16u
static const int8_t FM1_KEYMAP[5][16] = {   /* (the LEDs: id k at row 1 + k / 16, column k % 16; read back below) */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
    {32, 33, 34, 35, 36, 37, 38, 39, 40, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
static void fm1_led_key(uint32_t id, int on) { (void)id; (void)on; }
static uint32_t edges_btn, notes_seen;
static uint32_t fm1_input_edges(int x) { uint32_t e = edges_btn; (void)x; edges_btn = 0; return e; }
static uint32_t fm1_input_note_edges(void) { uint32_t e = fm1_in.notes & ~notes_seen; notes_seen = fm1_in.notes; return e; }
static void fm1_wdt_feed(void) {}
static int32_t fm1_adc_read(int c) { (void)c; return -1; }
static struct { uint32_t magic, stage, page, home, ui_frames; } felucca_dbg;
#define FELUCCA_ICONS 1
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N], scope_bufr[SCOPE_N];
static uint32_t scope_w;
#include "../src/panel.c"
#include "../src/ui.c"
/* ---- storage: none in the emulator (a message says so) */
static int project_used(uint32_t i) { (void)i; return 0; }
static void project_save(uint32_t i) { (void)i; ui_message("NOT IN EMULATOR"); }
static void project_load(uint32_t i) { (void)i; }
static void project_erase(uint32_t i) { (void)i; }
static void arrangement_save(void) {}
static uint32_t arrangement_ready(void) { return 0; }
static void arrangement_apply(uint32_t s) { (void)s; }
static uint32_t section_bars(uint32_t s) { (void)s; return 1u; }
static void song_backup(void) {}
static void song_restore(void) {}
static void section_store(uint32_t s) { live_sec = (int8_t)s; }
static void section_load(uint32_t s) { live_sec = (int8_t)s; }
static int up_used(uint32_t k) { (void)k; return 0; }
static int up_load(uint32_t k) { (void)k; return 0; }
static int up_load_to(track_t *t, uint32_t k) { (void)t; (void)k; return 0; }
static int up_arp_of(uint32_t k, int16_t *a) { (void)k; (void)a; return 0; }
static uint32_t up_count(void) { return 0; }
static uint32_t up_nth(uint32_t n) { return n; }
static uint32_t up_rank(uint32_t s) { return s; }
static void up_name(uint32_t k, char *b) { (void)k; str_cpy(b, "-", 13); }
static void up_slot_label(char *b, uint32_t k) { fmt_int(b, (int32_t)k + 1); }
static void up_ui(uint32_t op, uint32_t k) { (void)op; (void)k; ui_message("NOT IN EMULATOR"); }
static void settings_save(void) {}
#include "../src/ui_song.c"
#include "../src/ui_studio.c"
#include "../src/icons.c"
#include "../src/ui_draw.c"
static int vis_shown(void) { return 0; }
static void vis_draw(void) {}
static void vis_select(int32_t s) { (void)s; }
#include "../src/ui_layers.c"
#include "../src/ui_menu.c"
#include "../src/ui_input.c"
#include "../src/splash.c"
static int st_read(uint32_t off, void *dst, uint32_t n) { (void)off, (void)dst, (void)n; return -1; }
static int st_erase(uint32_t off) { (void)off; return -1; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { (void)off, (void)src, (void)n; return -1; }
#include "../src/storage.c"

/* ---- JavaScript's side */
static uint8_t transfer[0x14000];
static int32_t block[CTL * 2];
static float output[CTL * 2];
static uint32_t blocks_since_ui, booted_ms;

uint8_t *emu_buffer(void) { return transfer; }
uint16_t *emu_screen(void) { return screen; }
uint32_t emu_block_frames(void) { return CTL; }
uint32_t emu_rate(void) { return FS; }
void emu_slot(uint32_t k)                       /* the transfer buffer into user slot k (a sound pack's slot) */
{
    if (k >= SMP_USER_SLOTS) return;
    memcpy(user_flash[k], transfer, sizeof user_flash[k]);
    smp_user_scan(k);
}
void emu_init(void)
{
    uint32_t i, k;
    memset(user_flash, 0xFF, sizeof user_flash);
    for (k = 0; k < SMP_USER_SLOTS; k++) smp_user_scan(k);
    panel = PANEL_DEFAULT;
    for (i = 0; i < G_COUNT; i++) song.g[i] = GP[i].def;
    song.g[G_SYNC] = 0;
    fm6_init();
    dsu_defaults();
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults(t);
        if (i < NPART) {
            set_engine_of(t, TRK_DEF[i][0]);
            apply_preset_to(t, TRK_DEF[i][1]);
            t->engine = t->eng_req;
        }
        track_defaults_steps(t);
    }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    song.sel = 0;
    song.master_q12 = 2048;
    settings.palette = 4;
    palette_set(4);
    layers_init();
    sloop_splash();
    booted_ms = 1;
}
void emu_reset_tracks(void)                     /* after the sound pack: the guitar parts take their sound again */
{
    uint32_t i;
    for (i = 0; i < NPART; i++) {
        apply_preset_to(&trk[i], TRK_DEF[i][1]);
    }
}
void emu_volume(uint32_t v) { song.master_q12 = v > 4096u ? 4096u : v; }
/* the inputs: keys 0..26 (F3..G5, bit k), the buttons by label (B_FX .. B_OCTUP), the knobs by role (EN_*) */
void emu_keys(uint32_t bits) { fm1_in.notes = bits & 0x7FFFFFFu; }
void emu_button(uint32_t label, uint32_t down)
{
    uint32_t bit;
    if (label >= NB) return;
    bit = 1u << panel.btn[label];
    if (down) { fm1_in.buttons |= bit; edges_btn |= bit; }
    else fm1_in.buttons &= ~bit;
}
void emu_knob(uint32_t role, int32_t steps) { if (role < NE) encs[panel.enc[role]] += steps * panel.dir[role]; }
/* the LEDs: bit k = key k lit (full), dimmed; the buttons lit, bit = label */
static int led_on(const uint8_t *a, uint32_t id) { uint8_t q = led_pos[id]; return q != 0xFF && (a[q >> 3] >> (q & 7u)) & 1u; }
uint32_t emu_led_keys(uint32_t which)
{
    const uint8_t *a = which == 0 ? fm1_led : which == 1 ? fm1_led_dim : fm1_led_bg;
    uint32_t k, m = 0;
    for (k = 0; k < 27u; k++) if (led_on(a, 14u + k)) m |= 1u << k;
    return m;
}
uint32_t emu_led_buttons(void)
{
    uint32_t b, m = 0;
    for (b = 0; b < NB; b++) if (led_on(fm1_led, panel.btn[b])) m |= 1u << b;
    return m;
}
/* one UI frame, as the FM-1's main loop (16 ms); the splash for the first second */
uint32_t emu_ui(void)
{
    fm1_ms += 16u;
    if (booted_ms && fm1_ms < 1200u)
        return screen_dirty ? (screen_dirty = 0, 1u) : 0u;
    if (booted_ms) {
        booted_ms = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        go_home();
        ui.force = 1;
    }
    felucca_dbg.ui_frames++;
    ui_input();
    ui_leds();
    ui_draw();
    if (screen_dirty) { screen_dirty = 0; return 1u; }
    return 0u;
}
/* one block of audio (CTL frames, stereo float); the scope fed as audio.c does */
float *emu_render(void)
{
    uint32_t i;
    mix_block(block, CTL);
    for (i = 1; i < CTL; i += 2u) {
        scope_bufr[scope_w & (SCOPE_N - 1u)] = vis_tap[2u * i + 1u];
        scope_buf[scope_w++ & (SCOPE_N - 1u)] = vis_tap[2u * i];
    }
    for (i = 0; i < CTL * 2u; i++) output[i] = (float)block[i] * (1.0f / 32768.0f);
    return output;
}
