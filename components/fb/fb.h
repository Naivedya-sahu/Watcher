#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "epd.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// Framebuffer — 1-bit, 400×300 (15000 bytes)
// bit=1 → white (paper), bit=0 → black (ink)
// Origin: top-left (0,0). X=column, Y=row.
// ============================================================

#define FB_W     EPD_W           // 400
#define FB_H     EPD_H           // 300
#define FB_BYTES (FB_W * FB_H / 8)  // 15000

// Color constants
#define FB_WHITE  1
#define FB_BLACK  0

// Active font cell dimensions — keep in sync with included font header in fb.cpp
#define FONT_W  6   // Monocraft size-9 advance width (6x8 cell)
#define FONT_H  8   // Monocraft glyph height

// Partial refresh counter — after PARTIAL_LIMIT full refresh forced
#define FB_PARTIAL_LIMIT  60

typedef struct {
    uint8_t  buf[FB_BYTES];
    uint8_t  partial_count;
    bool     force_full_next;
} fb_t;

// Framebuffer lifecycle
void fb_init(fb_t *fb);
void fb_clear(fb_t *fb);               // fill with white
void fb_flush(fb_t *fb, epd_refresh_t refresh);  // send to display
void fb_flush_auto(fb_t *fb);          // auto-choose partial or full

// Pixel operations
void fb_set_pixel(fb_t *fb, int x, int y, int color);

// Filled primitives (fill entire rect)
void fb_fill_rect(fb_t *fb, int x, int y, int w, int h, int color);

// Outline primitives
void fb_draw_rect(fb_t *fb, int x, int y, int w, int h, int color);
void fb_draw_hline(fb_t *fb, int x, int y, int len, int color);
void fb_draw_vline(fb_t *fb, int x, int y, int len, int color);

// ── Font descriptor (for swappable fonts) ────────────────────
// Pass a font_desc_t* to _f variants; NULL = use built-in FONT_MONO6.
typedef struct {
    const uint8_t *data;  // glyph array: (last-first+1) * w bytes
    uint8_t        w;     // glyph width in pixels
    uint8_t        h;     // glyph height in pixels
    uint8_t        first; // first ASCII codepoint in data
    uint8_t        last;  // last  ASCII codepoint in data
} font_desc_t;

// Text (6×8 bitmap font — built-in FONT_MONO6)
// Returns pixel width drawn
int fb_draw_char(fb_t *fb, int x, int y, char c, int color);
int fb_draw_str(fb_t *fb, int x, int y, const char *s, int color);
int fb_draw_str_centered(fb_t *fb, int cx, int y, const char *s, int color);

// Font-pointer variants — pass NULL font to fall back to built-in
int fb_draw_char_f(fb_t *fb, int x, int y, char c, int color, const font_desc_t *f);
int fb_draw_str_f(fb_t *fb, int x, int y, const char *s, int color, const font_desc_t *f);
int fb_draw_str_centered_f(fb_t *fb, int cx, int y, const char *s, int color, const font_desc_t *f);

// Pill button outline (rounded rect, radius r)
void fb_draw_pill(fb_t *fb, int x, int y, int w, int h, int color);
void fb_fill_pill(fb_t *fb, int x, int y, int w, int h, int color);

// DAY_RING — 60 squares of 10×10px, clockwise from top-left.
// Geometry matches screens.jsx: SQ=10, PX=22, PY=21, COLS=18, ROWS=14, x0=8, y0=9.
// filled_count 0..60. Pass -1 for dot_color/empty_color to skip that category.
void fb_draw_dot_ring(fb_t *fb, int filled_count, int dot_color, int empty_color);

// 120s wave ring — filled squares solid black, empty squares as outlines.
// cycle = (minutes*60 + seconds) % 120
// First 60s: empties clockwise (indices 0..cycle are empty).
// Next 60s: refills clockwise (indices 0..(cycle-60) are filled).
void fb_draw_dot_ring_wave(fb_t *fb, int cycle);

// Play triangle (▶) — used for Pomodoro start button
void fb_draw_play_tri(fb_t *fb, int cx, int cy, int size, int color);

// Pause bars (║) — used for Pomodoro pause button
void fb_draw_pause(fb_t *fb, int cx, int cy, int size, int color);

// Stop square (■) — used for Pomodoro stop button
void fb_draw_stop_sq(fb_t *fb, int cx, int cy, int size, int color);

#ifdef __cplusplus
}
#endif

