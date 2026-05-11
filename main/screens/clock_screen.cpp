// clock_screen.cpp — Digital clock: 7-seg HH:MM inside 60-square perimeter ring + date
// Ring: 120s wave (first 60s empties clockwise, next 60s refills) — screens.jsx default.
// Geometry from DigitalClockScreen in screens.jsx.

#include "clock_screen.h"
#include "screen_mgr.h"
#include "fb.h"
#include "gfx_7seg.h"
#include "time_svc.h"
#include "buzzer.h"
#include "config_store.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

static const char *TAG = "clock";

// ── Layout — design truth: DigitalClockScreen in screens.jsx ─────────────────
//
// digitH = 116, gap = 8, native digit = 62×110, native colon = 18×110
// scale  = 116 / 110  =  1.054545…
// digitW = round(62  × scale) = 65   (65.382)
// colonW = round(18  × scale) = 19   (18.982)
// totalW = 4×65 + 19 + 4×8   = 279 + 32 = 311
// startX = round(200 − 311/2) = 45
// startY = round(150 − 116/2 − 10) = 82  (center vertically, nudge up 10px)
// dateY  = startY + 116 + 30  = 228
//
// X positions:
//   H1  = 45
//   H2  = 45 + 65 + 8 = 118
//   COL = 118 + 65 + 8 = 191
//   M1  = 191 + 19 + 8 = 218
//   M2  = 218 + 65 + 8 = 291

#define CLOCK_SCALE  (116.0f / 110.0f)
#define DIG_Y     82
#define DIG_X_H1  45
#define DIG_X_H2  118
#define DIG_X_COL 191
#define DIG_X_M1  218
#define DIG_X_M2  291
#define DATE_Y    228

// ── Internal state ────────────────────────────────────────────
static int  s_last_sec    = -1;
static bool s_colon_on    = true;
static int  s_date_format = 0;  // mirrors g_cfg.date_format, set on enter
static bool s_was_synced  = false; // tracks NTP sync transition

// ── Helpers ───────────────────────────────────────────────────
static const char *WDAY[] = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };
static const char *MON[]  = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

static const char *ordinal(int n) {
    int m = n % 100;
    if (m >= 11 && m <= 13) return "th";
    switch (n % 10) { case 1: return "st"; case 2: return "nd"; case 3: return "rd"; }
    return "th";
}

static void fmt_date(char *buf, size_t cap, const struct tm *t) {
    int d = t->tm_mday, y = 1900 + t->tm_year, mo = t->tm_mon + 1;
    switch (g_cfg.date_format) {
        case 1: snprintf(buf,cap,"%s %d %s", WDAY[t->tm_wday]+0, d, MON[t->tm_mon]); break; // short
        case 2: snprintf(buf,cap,"%04d-%02d-%02d", y, mo, d);                           break; // iso
        case 3: snprintf(buf,cap,"%02d/%02d/%04d", d, mo, y);                           break; // numeric
        default: snprintf(buf,cap,"%s, %d%s %s. %d", WDAY[t->tm_wday], d, ordinal(d), MON[t->tm_mon], y);
    }
}

// ── Render ────────────────────────────────────────────────────
static void clock_render(fb_t *fb) {
    if (!time_svc_is_synced()) {
        fb_draw_str_centered(fb, 200, 130, "--:--", FB_BLACK);
        fb_draw_str_centered(fb, 200, 145, "Waiting for NTP...", FB_BLACK);
        return;
    }

    time_t now = time_svc_get();
    struct tm t;
    localtime_r(&now, &t);

    // Dot ring: 120s wave (screens.jsx default). Filled=solid, empty=outline.
    fb_draw_dot_ring_wave(fb, (t.tm_min * 60 + t.tm_sec) % 120);

    // 7-seg time (hour tens suppressed if zero)
    int h = g_cfg.time_24h ? t.tm_hour : (t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12);
    if (h / 10 != 0)
        gfx_draw_7seg_digit(fb, DIG_X_H1, DIG_Y, h / 10, CLOCK_SCALE, FB_BLACK, -1);
    gfx_draw_7seg_digit(fb, DIG_X_H2, DIG_Y, h % 10, CLOCK_SCALE, FB_BLACK, -1);
    gfx_draw_7seg_colon(fb, DIG_X_COL, DIG_Y, s_colon_on, CLOCK_SCALE, FB_BLACK, -1);
    gfx_draw_7seg_digit(fb, DIG_X_M1, DIG_Y, t.tm_min / 10, CLOCK_SCALE, FB_BLACK, -1);
    gfx_draw_7seg_digit(fb, DIG_X_M2, DIG_Y, t.tm_min % 10, CLOCK_SCALE, FB_BLACK, -1);

    // Date string (centered, below digits)
    char date[64];
    fmt_date(date, sizeof(date), &t);
    fb_draw_str_centered(fb, 200, DATE_Y, date, FB_BLACK);
}

// ── Tick ──────────────────────────────────────────────────────
static void clock_tick(void) {
    bool synced = time_svc_is_synced();

    if (!synced) {
        s_was_synced = false;
        static uint32_t s_wait = 0;
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (now_ms - s_wait >= 2000) { s_wait = now_ms; screen_force_render(); }
        return;
    }

    // First tick after NTP sync: force full refresh to clear "--:--" ghost
    if (!s_was_synced) {
        s_was_synced = true;
        s_last_sec   = -1;  // force re-render this second
        screen_force_full();
        return;
    }

    time_t now = time_svc_get();
    struct tm t; localtime_r(&now, &t);
    if (t.tm_sec != s_last_sec) {
        s_last_sec = t.tm_sec;
        s_colon_on = true;   // static filled colon — no blink
        screen_force_render();
    }
}

// ── Button handler (matches React handleBtn for 'clock') ──────
static void clock_btn(btn_id_t id, btn_evt_t evt) {
    if (id == BTN_1 && evt == BTN_SHORT) { screen_goto("alarm");  return; }
    if (id == BTN_2 && evt == BTN_SHORT) { screen_goto("pomo");   return; }
    if (id == BTN_3 && evt == BTN_SHORT) {
        // cycle date format
        g_cfg.date_format = (g_cfg.date_format + 1) % 4;
        cfg_save();
        screen_force_render();
    }
}

// ── Encoder: rotate = nothing on clock, click = next screen ──
static void clock_enc(int delta) { (void)delta; }
static void clock_enc_click(void) { screen_next(); }

// ── Screen definition ─────────────────────────────────────────
screen_def_t clock_screen = {
    .id          = "clock",
    .label       = "CLOCK",
    .group       = "time",
    .enter       = NULL,
    .exit        = NULL,
    .tick        = clock_tick,
    .render      = clock_render,
    .on_button   = clock_btn,
    .on_encoder  = clock_enc,
    .on_enc_click= clock_enc_click,
    .needs_render= true,
    .force_full  = true,
};
