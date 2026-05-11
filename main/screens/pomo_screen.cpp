// pomo_screen.cpp — Pomodoro timer with dot ring progress
// Geometry from EpdCanvas PomoScreen (React source verified).

#include "pomo_screen.h"
#include "screen_mgr.h"
#include "fb.h"
#include "gfx_7seg.h"
#include "buzzer.h"
#include "config_store.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "time_svc.h"

// ── Timer modes ───────────────────────────────────────────────
typedef enum { MODE_FOCUS = 0, MODE_BREAK = 1, MODE_LONG = 2 } pomo_mode_t;
static const char *MODE_LABELS[] = { "FOCUS", "SHORT BREAK", "LONG BREAK" };

// Sequence: [FOCUS, BREAK] × g_cfg.pomo_cycles + [LONG]
// Max cycles=10 → max length 21. Default matches pomo_cycles=3 (config default).
#define MAX_SEQ_LEN 21
static pomo_mode_t s_seq[MAX_SEQ_LEN] = {
    MODE_FOCUS, MODE_BREAK,
    MODE_FOCUS, MODE_BREAK,
    MODE_FOCUS, MODE_BREAK,
    MODE_LONG
};
static int s_seq_len = 7;  // rebuilt on pomo_enter from g_cfg.pomo_cycles

static void build_seq(void) {
    int c = g_cfg.pomo_cycles;
    if (c < 1)  c = 1;
    if (c > 10) c = 10;
    s_seq_len = 0;
    for (int i = 0; i < c; i++) {
        s_seq[s_seq_len++] = MODE_FOCUS;
        s_seq[s_seq_len++] = MODE_BREAK;
    }
    s_seq[s_seq_len++] = MODE_LONG;
}

// ── State ─────────────────────────────────────────────────────
static pomo_mode_t  s_mode    = MODE_FOCUS;
static int          s_seq_idx = 0;
static uint32_t     s_total   = 25 * 60;
static uint32_t     s_remain  = 25 * 60;
static bool         s_running = false;
static bool         s_paused  = false;
static uint32_t     s_last_ms = 0;
static int          s_session = 1;

// ── Layout — design truth: PomodoroScreen in screens.jsx ─────────────────────
//
// digitH = 150, gap = 12, native digit = 62×110
// scale  = 150 / 110  =  1.363636…
// digitW = round(62 × scale) = 85   (84.545)
// totalW = 2×85 + 12 = 182
// startX = round(200 − 182/2) = 109
// startY = round(139 − 150/2) = 64   (inner-band centre y≈139)
// D2_X   = 109 + 85 + 12 = 206
//
// Header (inside ring safe zone, top: 24):
//   "POMODORO" at (32, 24), "SESSION N/M" right-aligned at x=368
//
// Bottom controls (bottom: 30 → y≈270, side padding 32px):
//   Play/Pause icon centre at (52, 271)
//   Pills centred: total width ≈ 62+6+60+6+58=192 → startX=104, y=260
//   Stop icon centre at (348, 271)
//
// No mode label or meta text below digits — pills show mode, header shows session.

#define POMO_SCALE   (150.0f / 110.0f)
#define POMO_DIG_Y    64
#define POMO_DIG_X0  109   // tens of minutes
#define POMO_DIG_X1  206   // units of minutes
#define POMO_HDR_Y    24   // header label y (inside top ring row)
#define POMO_PILL_Y  258   // pill top y (above bottom ring row)
#define POMO_PILL_H   22
#define POMO_BTN_CY  271   // play/pause/stop centre y

// ── Render ────────────────────────────────────────────────────
static void pomo_render(fb_t *fb) {
    // Ring: 120s wave driven by wall-clock time (matches clock screen animation)
    int ring_cycle = 0;
    if (time_svc_is_synced()) {
        time_t now_t = time_svc_get();
        struct tm tnow; localtime_r(&now_t, &tnow);
        ring_cycle = (tnow.tm_min * 60 + tnow.tm_sec) % 120;
    }
    fb_draw_dot_ring_wave(fb, ring_cycle);

    // ── Header — inside top ring safe area (y=24) ─────────────
    // "POMODORO" left at x=32, "SESSION N/M" right-aligned at x=368
    fb_draw_str(fb, 32, POMO_HDR_Y, "POMODORO", FB_BLACK);
    char sess_str[32];
    // Total focus sessions = (seq_len - 1) / 2  (seq = [F,B,...,F,B,L])
    snprintf(sess_str, sizeof(sess_str), "SESSION %d/%d", s_session, (s_seq_len - 1) / 2);
    int sw = (int)strlen(sess_str) * FONT_W;
    fb_draw_str(fb, 368 - sw, POMO_HDR_Y, sess_str, FB_BLACK);

    // ── Big MM readout — two 7-seg digits, minutes only ───────
    int mins = (int)(s_remain / 60);
    if (mins > 99) mins = 99;
    gfx_draw_7seg_digit(fb, POMO_DIG_X0, POMO_DIG_Y, mins / 10,
                        POMO_SCALE, FB_BLACK, -1);
    gfx_draw_7seg_digit(fb, POMO_DIG_X1, POMO_DIG_Y, mins % 10,
                        POMO_SCALE, FB_BLACK, -1);

    // ── Mode pills: FOCUS(62) BREAK(60) LONG(58) centred ──────
    // Total width = 62+6+60+6+58 = 192 → startX = (400-192)/2 = 104
    const char *pnames[] = { "FOCUS", "BREAK", "LONG" };
    const int   pwids[]  = { 62, 60, 58 };
    int px = 104;
    for (int i = 0; i < 3; i++) {
        bool active = (s_mode == (pomo_mode_t)i);
        if (active) {
            fb_fill_pill(fb, px, POMO_PILL_Y, pwids[i], POMO_PILL_H, FB_BLACK);
            fb_draw_str_centered(fb, px + pwids[i]/2, POMO_PILL_Y + (POMO_PILL_H - FONT_H)/2,
                                 pnames[i], FB_WHITE);
        } else {
            fb_draw_pill(fb, px, POMO_PILL_Y, pwids[i], POMO_PILL_H, FB_BLACK);
            fb_draw_str_centered(fb, px + pwids[i]/2, POMO_PILL_Y + (POMO_PILL_H - FONT_H)/2,
                                 pnames[i], FB_BLACK);
        }
        px += pwids[i] + 6;
    }

    // ── Play/Pause left (cx=52), Stop right (cx=348) ──────────
    if (s_running && !s_paused)
        fb_draw_pause(fb, 52, POMO_BTN_CY, 8, FB_BLACK);
    else
        fb_draw_play_tri(fb, 52, POMO_BTN_CY, 8, FB_BLACK);
    fb_draw_stop_sq(fb, 348, POMO_BTN_CY, 13, FB_BLACK);
}

// ── Timer logic ───────────────────────────────────────────────
// Shared advance logic — called by both pomo_tick (active) and pomo_bg_tick (background).
// Returns true if any state changed (caller decides whether to trigger render).
static bool pomo_advance(void) {
    if (!s_running || s_paused) return false;
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    if (s_last_ms == 0) { s_last_ms = now; return false; }
    uint32_t elapsed_ms = now - s_last_ms;
    if (elapsed_ms < 1000) return false;
    // Bulk-subtract all elapsed seconds at once — prevents fast-forward on re-entry
    uint32_t elapsed_s = elapsed_ms / 1000;
    s_last_ms += elapsed_s * 1000;
    bool changed = false;
    while (elapsed_s > 0 && s_running) {
        elapsed_s--;
        changed = true;
        if (s_remain > 0) s_remain--;
        if (s_remain == 0) {
            s_running = false;
            buzzer_tone(BUZZ_POMO_DONE);
            s_seq_idx = (s_seq_idx + 1) % s_seq_len;
            s_mode    = s_seq[s_seq_idx];
            s_total   = (uint32_t)(
                s_mode == MODE_FOCUS ? g_cfg.pomo_focus_mins :
                s_mode == MODE_BREAK ? g_cfg.pomo_break_mins :
                g_cfg.pomo_long_mins) * 60;
            s_remain  = s_total;
            if (s_mode == MODE_FOCUS) s_session++;
            pomo_screen.force_full = true;
        }
    }
    return changed;
}

// Called every tick when pomo is the active screen — advances timer + triggers render.
static void pomo_tick(void) {
    if (pomo_advance()) screen_force_render();
}

// Called every main loop iteration regardless of active screen — keeps timer alive.
void pomo_bg_tick(void) {
    if (!s_running || s_paused) return;
    if (pomo_advance()) {
        // Mark needs_render so screen_mgr picks it up when pomo becomes active.
        pomo_screen.needs_render = true;
    }
}

// ── Public controls (called from other screens too) ───────────
void pomo_start_stop(void) {
    if (!s_running) {
        s_running = true; s_paused = false; s_last_ms = 0;
        buzzer_tone(BUZZ_SUCCESS);
    } else if (!s_paused) {
        s_paused = true;
    } else {
        s_paused = false; s_last_ms = 0;
    }
    screen_force_render();
}

void pomo_reset(void) {
    s_running = false; s_paused = false;
    s_seq_idx = 0; s_mode = MODE_FOCUS;
    s_total   = (uint32_t)g_cfg.pomo_focus_mins * 60;
    s_remain  = s_total;
    s_session = 1;
    buzzer_tone(BUZZ_TICK);
    screen_force_full();
    screen_force_render();
}

bool pomo_is_running(void) { return s_running && !s_paused; }

uint32_t pomo_get_remaining_s(void) { return s_remain; }

const char *pomo_get_mode_str(void) { return MODE_LABELS[s_mode]; }

int pomo_get_session(void) { return s_session; }

// ── Button handler ────────────────────────────────────────────
static void pomo_btn(btn_id_t id, btn_evt_t evt) {
    if (id == BTN_3 && evt == BTN_LONG)  { pomo_reset(); return; }
    if (id == BTN_3 && evt == BTN_SHORT) { pomo_start_stop(); return; }
    if (id == BTN_1 && evt == BTN_SHORT) { screen_goto("clock"); return; }
    if (id == BTN_2 && evt == BTN_SHORT) { screen_goto("cal"); return; }
}

static void pomo_enc(int delta) { (void)delta; }
static void pomo_enc_click(void) { pomo_start_stop(); }

static void pomo_enter(void) {
    build_seq();  // rebuild from g_cfg.pomo_cycles (may have changed via web/NVS)
    // BUG-11: clamp seq_idx in case pomo_cycles was reduced while running
    if (s_seq_idx >= s_seq_len) { s_seq_idx = 0; s_session = 1; }
    // BUG-4: derive s_total from current mode, not always focus
    s_total = (uint32_t)(
        s_mode == MODE_FOCUS ? g_cfg.pomo_focus_mins :
        s_mode == MODE_BREAK ? g_cfg.pomo_break_mins :
        g_cfg.pomo_long_mins) * 60;
    if (!s_running) s_remain = s_total;
    screen_force_render();
}

screen_def_t pomo_screen = {
    .id           = "pomo",
    .label        = "POMODORO",
    .group        = "work",
    .enter        = pomo_enter,
    .exit         = NULL,
    .tick         = pomo_tick,
    .render       = pomo_render,
    .on_button    = pomo_btn,
    .on_encoder   = pomo_enc,
    .on_enc_click = pomo_enc_click,
    .needs_render = true,
    .force_full   = true,
};
