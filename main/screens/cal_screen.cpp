// cal_screen.cpp — Full-bleed month calendar grid
// Geometry from EpdCanvas CalendarScreen (React source verified):
//   Header: 30px, Day-strip: 14px, Grid: 256px (400×300 full bleed)

#include "cal_screen.h"
#include "screen_mgr.h"
#include "fb.h"
#include "time_svc.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

// ── State ─────────────────────────────────────────────────────
static int s_year  = 2026;
static int s_month = 3;    // 0-based

static const char *MONTH_NAMES[] = {
    "JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE",
    "JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"
};
static const char *WDAY3[] = { "SUN","MON","TUE","WED","THU","FRI","SAT" };

// ── Layout ────────────────────────────────────────────────────
#define HDR_H   30    // header height
#define STRIP_H 14    // day-name strip height
#define GRID_H  256   // remaining for grid
#define COL_W   57    // 400/7 ≈ 57 (last col gets remainder)

// ── Render ────────────────────────────────────────────────────
static void cal_render(fb_t *fb) {
    // BUG-8: guard against un-synced NTP — show placeholder instead of stale date
    if (!time_svc_is_synced()) {
        fb_draw_str_centered(fb, 200,  10, "CALENDAR",       FB_BLACK);
        fb_draw_hline(fb, 0, HDR_H, 400, FB_BLACK);
        fb_draw_str_centered(fb, 200, 140, "WAITING FOR NTP", FB_BLACK);
        fb_draw_str_centered(fb, 200, 154, "CONNECT WIFI TO SYNC", FB_BLACK);
        return;
    }

    // ── Header ──────────────────────────────────────────────
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "%s %d", MONTH_NAMES[s_month], s_year);
    fb_draw_str_centered(fb, 200, 10, hdr, FB_BLACK);
    fb_draw_hline(fb, 0, HDR_H, 400, FB_BLACK);

    // ── Day-name strip ───────────────────────────────────────
    for (int d = 0; d < 7; d++) {
        int cx = d * COL_W + COL_W / 2;
        fb_draw_str_centered(fb, cx, HDR_H + 3, WDAY3[d], FB_BLACK);
    }
    fb_draw_hline(fb, 0, HDR_H + STRIP_H, 400, FB_BLACK);

    // ── Calendar grid ─────────────────────────────────────────
    // First day of month (weekday)
    struct tm tm_first;
    memset(&tm_first, 0, sizeof(tm_first));
    tm_first.tm_year = s_year - 1900;
    tm_first.tm_mon  = s_month;
    tm_first.tm_mday = 1;
    mktime(&tm_first);
    int first_wday = tm_first.tm_wday;  // 0=Sun

    // Days in month
    struct tm tm_next;
    memset(&tm_next, 0, sizeof(tm_next));
    tm_next.tm_year = s_year - 1900;
    tm_next.tm_mon  = s_month + 1;
    tm_next.tm_mday = 0;
    mktime(&tm_next);
    int days_in_month = tm_next.tm_mday;

    // Today (for highlight)
    time_t now = time_svc_get();
    struct tm tnow; localtime_r(&now, &tnow);
    int today_day   = tnow.tm_mday;
    int today_month = tnow.tm_mon;
    int today_year  = 1900 + tnow.tm_year;

    // Build cell array
    int total_cells = first_wday + days_in_month;
    int rows   = (total_cells + 6) / 7;
    int cell_h = GRID_H / rows;
    int extra  = GRID_H % rows;   // BUG-9: remainder pixels given to last row

    int grid_top = HDR_H + STRIP_H;
    int cell_num = 1 - first_wday;
    int cy = grid_top;  // running y accumulator — avoids O(n²) inner loop

    for (int row = 0; row < rows; row++) {
        int this_h = (row == rows - 1) ? (cell_h + extra) : cell_h;

        for (int col = 0; col < 7; col++) {
            int cx = col * COL_W;
            int cw = (col == 6) ? (400 - cx) : COL_W;

            // Cell border
            fb_draw_rect(fb, cx, cy, cw, this_h, FB_BLACK);

            int n = cell_num++;
            bool this_month = (n >= 1 && n <= days_in_month);
            if (!this_month) continue;   // skip adjacent-month cells

            bool is_today = (n == today_day &&
                             s_month == today_month &&
                             s_year  == today_year);

            if (is_today)
                fb_fill_rect(fb, cx + 1, cy + 1, cw - 2, this_h - 2, FB_BLACK);

            char num[4];
            snprintf(num, sizeof(num), "%d", n);
            int tx = cx + cw/2 - ((int)strlen(num) * FONT_W) / 2;
            int ty = cy + this_h/2 - FONT_H/2;
            fb_draw_str(fb, tx, ty, num, is_today ? FB_WHITE : FB_BLACK);
        }
        cy += this_h;  // advance row top for next iteration
    }
}

// ── Buttons ───────────────────────────────────────────────────
static void advance_month(int delta) {
    s_month += delta;
    if (s_month < 0)  { s_month = 11; s_year--; }
    if (s_month > 11) { s_month = 0;  s_year++; }
    screen_force_full();
    screen_force_render();
}

static void cal_btn(btn_id_t id, btn_evt_t evt) {
    if (id == BTN_1 && evt == BTN_SHORT) { advance_month(-1); return; }
    if (id == BTN_2 && evt == BTN_SHORT) { advance_month(+1); return; }
    if (id == BTN_3 && evt == BTN_SHORT) { screen_goto("tasks"); return; }
}

static void cal_enc(int delta) { advance_month(delta); }
static void cal_enc_click(void) { screen_goto("tasks"); }

static void cal_enter(void) {
    // Snap to current month on entry
    if (time_svc_is_synced()) {
        time_t now = time_svc_get();
        struct tm t; localtime_r(&now, &t);
        s_year  = 1900 + t.tm_year;
        s_month = t.tm_mon;
    }
    screen_force_render();
}

screen_def_t cal_screen = {
    .id           = "cal",
    .label        = "CALENDAR",
    .group        = "work",
    .enter        = cal_enter,
    .exit         = NULL,
    .tick         = NULL,
    .render       = cal_render,
    .on_button    = cal_btn,
    .on_encoder   = cal_enc,
    .on_enc_click = cal_enc_click,
    .needs_render = true,
    .force_full   = true,
};
