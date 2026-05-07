#include "fb.h"
#include "font_monocraft.h"
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── Framebuffer lifecycle ────────────────────────────────────

void fb_init(fb_t *fb) {
    memset(fb->buf, 0xFF, FB_BYTES);  // all white
    fb->partial_count  = 0;
    fb->force_full_next = false;
}

void fb_clear(fb_t *fb) {
    memset(fb->buf, 0xFF, FB_BYTES);
}

void fb_flush(fb_t *fb, epd_refresh_t refresh) {
    epd_flush(fb->buf, refresh);
    if (refresh == EPD_REFRESH_FULL) {
        fb->partial_count  = 0;
        fb->force_full_next = false;
    } else {
        fb->partial_count++;
        if (fb->partial_count >= FB_PARTIAL_LIMIT)
            fb->force_full_next = true;
    }
}

void fb_flush_auto(fb_t *fb) {
    if (fb->force_full_next) {
        fb_flush(fb, EPD_REFRESH_FULL);
    } else {
        fb_flush(fb, EPD_REFRESH_PARTIAL);
    }
}

// ── Pixel ────────────────────────────────────────────────────

void fb_set_pixel(fb_t *fb, int x, int y, int color) {
    if (x < 0 || x >= FB_W || y < 0 || y >= FB_H) return;
    int idx = y * (FB_W / 8) + x / 8;
    int bit = 7 - (x & 7);
    if (color == FB_BLACK)
        fb->buf[idx] &= ~(1 << bit);
    else
        fb->buf[idx] |=  (1 << bit);
}

// ── Filled rect ──────────────────────────────────────────────

void fb_fill_rect(fb_t *fb, int x, int y, int w, int h, int color) {
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = (x + w) > FB_W ? FB_W : (x + w);
    int y2 = (y + h) > FB_H ? FB_H : (y + h);
    for (int py = y1; py < y2; py++) {
        int row_base = py * (FB_W / 8);
        for (int px = x1; px < x2; px++) {
            int bit = 7 - (px & 7);
            if (color == FB_BLACK)
                fb->buf[row_base + px / 8] &= ~(1 << bit);
            else
                fb->buf[row_base + px / 8] |=  (1 << bit);
        }
    }
}

// ── Outline ──────────────────────────────────────────────────

void fb_draw_hline(fb_t *fb, int x, int y, int len, int color) {
    fb_fill_rect(fb, x, y, len, 1, color);
}
void fb_draw_vline(fb_t *fb, int x, int y, int len, int color) {
    fb_fill_rect(fb, x, y, 1, len, color);
}
void fb_draw_rect(fb_t *fb, int x, int y, int w, int h, int color) {
    fb_draw_hline(fb, x,     y,     w, color);
    fb_draw_hline(fb, x,     y+h-1, w, color);
    fb_draw_vline(fb, x,     y,     h, color);
    fb_draw_vline(fb, x+w-1, y,     h, color);
}

// ── Text ─────────────────────────────────────────────────────

int fb_draw_char(fb_t *fb, int x, int y, char c, int color) {
    if (c < 0x20 || c > 0x7E) c = '?';
    const uint8_t *glyph = FONT_MONO7[c - 0x20];
    for (int col = 0; col < FONT_W; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < FONT_H; row++) {
            // Font encoding: bit 0 = TOP pixel, bit 7 = BOTTOM pixel.
            // (Classic column-major bitmap font convention — LSB = top row.)
            if (bits & (1 << row))
                fb_set_pixel(fb, x + col, y + row, color);
        }
    }
    return FONT_W;
}

// ── Font-pointer variant — draw with any font descriptor ──────
int fb_draw_char_f(fb_t *fb, int x, int y, char c, int color, const font_desc_t *f) {
    if (!f) return fb_draw_char(fb, x, y, c, color);
    if ((uint8_t)c < f->first || (uint8_t)c > f->last) c = '?';
    const uint8_t *glyph = f->data + ((uint8_t)c - f->first) * f->w;
    for (int col = 0; col < f->w; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < f->h; row++) {
            if (bits & (1 << row))
                fb_set_pixel(fb, x + col, y + row, color);
        }
    }
    return f->w;
}

int fb_draw_str_f(fb_t *fb, int x, int y, const char *s, int color, const font_desc_t *f) {
    int cx = x;
    while (*s) { cx += fb_draw_char_f(fb, cx, y, *s++, color, f); }
    return cx - x;
}

int fb_draw_str_centered_f(fb_t *fb, int cx, int y, const char *s, int color, const font_desc_t *f) {
    int w = f ? (int)strlen(s) * f->w : (int)strlen(s) * FONT_W;
    return fb_draw_str_f(fb, cx - w / 2, y, s, color, f);
}

int fb_draw_str(fb_t *fb, int x, int y, const char *s, int color) {
    int cx = x;
    while (*s) { cx += fb_draw_char(fb, cx, y, *s++, color); }
    return cx - x;
}

int fb_draw_str_centered(fb_t *fb, int cx, int y, const char *s, int color) {
    int len = 0;
    for (const char *p = s; *p; p++) len++;
    int total_w = len * FONT_W;
    int x = cx - total_w / 2;
    return fb_draw_str(fb, x, y, s, color);
}

// ── Pill button ──────────────────────────────────────────────

void fb_draw_pill(fb_t *fb, int x, int y, int w, int h, int color) {
    int r = h / 2;
    fb_draw_hline(fb, x + r, y, w - 2*r, color);
    fb_draw_hline(fb, x + r, y + h - 1, w - 2*r, color);
    fb_draw_vline(fb, x, y + r, h - 2*r, color);
    fb_draw_vline(fb, x + w - 1, y + r, h - 2*r, color);
    // Corner arcs — draw the 1px band at radius r (pixels inside r, outside r-1).
    // Half-open: (r-1)² < dist² ≤ r² keeps exactly one pixel layer on the arc.
    for (int dy = 0; dy < r; dy++) {
        for (int dx = 0; dx < r; dx++) {
            if (dx*dx + dy*dy <= (r-1)*(r-1)) continue;
            if (dx*dx + dy*dy >  r*r)         continue;
            // top-left arc
            fb_set_pixel(fb, x + r - 1 - dx, y + r - 1 - dy, color);
            // top-right arc
            fb_set_pixel(fb, x + w - r + dx, y + r - 1 - dy, color);
            // bottom-left arc
            fb_set_pixel(fb, x + r - 1 - dx, y + h - r + dy, color);
            // bottom-right arc
            fb_set_pixel(fb, x + w - r + dx, y + h - r + dy, color);
        }
    }
}

void fb_fill_pill(fb_t *fb, int x, int y, int w, int h, int color) {
    int r = h / 2;
    fb_fill_rect(fb, x + r, y, w - 2*r, h, color);
    for (int dy = 0; dy < r; dy++) {
        for (int dx = 0; dx < r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                fb_set_pixel(fb, x + r - 1 - dx, y + r - 1 - dy, color);
                fb_set_pixel(fb, x + r - 1 - dx, y + h - r + dy, color);
                fb_set_pixel(fb, x + w - r + dx, y + r - 1 - dy, color);
                fb_set_pixel(fb, x + w - r + dx, y + h - r + dy, color);
            }
        }
    }
}

// ── Dot ring — DAY_RING geometry from screens.jsx ────────────
// 60 squares of 10×10px, clockwise from top-left.
// Matches JS exactly: SQ=10, PX=22, PY=21, COLS=18, ROWS=14, x0=8, y0=9
//
// Generated by the same loop as screens.jsx:
//   Top  18: x=8+i*22,  y=9         i=0..17
//   Right 13: x=382,    y=9+i*21    i=1..13
//   Bottom 17: x=8+i*22, y=282      i=16..0
//   Left  12: x=8,      y=9+i*21    i=12..1
//   Total: 18+13+17+12 = 60
//
// Perimeter rect: (8,9)→(392,292) — fits 400×300 with margin.

static const uint16_t DOT_RING_X[60] = {
    // Top 18 (i=0..17): x=8+i*22
    8,30,52,74,96,118,140,162,184,206,228,250,272,294,316,338,360,382,
    // Right 13 (i=1..13): x=382
    382,382,382,382,382,382,382,382,382,382,382,382,382,
    // Bottom 17 (i=16..0): x=8+i*22
    360,338,316,294,272,250,228,206,184,162,140,118,96,74,52,30,8,
    // Left 12 (i=12..1): x=8
    8,8,8,8,8,8,8,8,8,8,8,8
};
static const uint16_t DOT_RING_Y[60] = {
    // Top 18: y=9
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    // Right 13 (i=1..13): y=9+i*21
    30,51,72,93,114,135,156,177,198,219,240,261,282,
    // Bottom 17: y=282
    282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,
    // Left 12 (i=12..1): y=9+i*21
    261,240,219,198,177,156,135,114,93,72,51,30
};

void fb_draw_dot_ring(fb_t *fb, int filled_count, int dot_color, int empty_color) {
    for (int i = 0; i < 60; i++) {
        int color = (i < filled_count) ? dot_color : empty_color;
        if (color != -1) {
            fb_fill_rect(fb, DOT_RING_X[i], DOT_RING_Y[i], 10, 10, color);
        }
    }
}

void fb_draw_dot_ring_wave(fb_t *fb, int cycle) {
    // cycle = (min*60 + sec) % 120
    // 0..59: emptying — indices 0..cycle are empty (outline), rest filled (solid)
    // 60..119: filling  — indices 0..(cycle-60) are filled (solid), rest empty (outline)
    bool emptying = (cycle < 60);
    int  head     = emptying ? cycle : (cycle - 60);
    for (int i = 0; i < 60; i++) {
        bool filled = emptying ? (i > head) : (i <= head);
        if (filled)
            fb_fill_rect(fb, DOT_RING_X[i], DOT_RING_Y[i], 10, 10, FB_BLACK);
        else
            fb_draw_rect(fb, DOT_RING_X[i], DOT_RING_Y[i], 10, 10, FB_BLACK);
    }
}

// ── Icons ────────────────────────────────────────────────────

void fb_draw_play_tri(fb_t *fb, int cx, int cy, int size, int color) {
    // Right-pointing triangle centered at (cx, cy), size = half-height
    for (int dy = -size; dy <= size; dy++) {
        int half_w = size - (dy < 0 ? -dy : dy);
        for (int dx = -half_w; dx <= half_w; dx++) {
            if (dx >= 0) fb_set_pixel(fb, cx + dx, cy + dy, color);
        }
    }
}

void fb_draw_pause(fb_t *fb, int cx, int cy, int size, int color) {
    int bar_w = size / 3;
    int bar_h = size * 2;
    int gap   = size / 2;
    fb_fill_rect(fb, cx - gap - bar_w, cy - bar_h/2, bar_w, bar_h, color);
    fb_fill_rect(fb, cx + gap,         cy - bar_h/2, bar_w, bar_h, color);
}

void fb_draw_stop_sq(fb_t *fb, int cx, int cy, int size, int color) {
    fb_fill_rect(fb, cx - size/2, cy - size/2, size, size, color);
}

#ifdef __cplusplus
}
#endif

