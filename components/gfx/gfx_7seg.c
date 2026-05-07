// gfx_7seg.c — chamfered 7-segment digit renderer
// Vertices extracted verbatim from SvgDigit paths in screens.jsx.
// Native space: 62 wide × 110 tall.
//
// Segment index order (matches SEG_MAP in screens.jsx):
//   0 top   1 tl   2 tr   3 mid   4 bl   5 br   6 bot

#include "gfx_7seg.h"
#include "gfx_primitives.h"
#include <math.h>

// ── Segment vertex tables (native 62×110 space) ──────────────────────────────
// Each row: 6 vertices, path from screens.jsx SVG "M…L…L…L…L…L…Z"
// VX[seg][vert], VY[seg][vert]

static const float SEG_VX[7][6] = {
    // 0 top:  M14,0 L48,0 L54,4.5 L48,9 L14,9 L8,4.5
    { 14.0f, 48.0f, 54.0f, 48.0f, 14.0f,  8.0f },
    // 1 tl:   M4.5,14 L0,18.5 L0,45 L4.5,50 L9,45 L9,18.5
    {  4.5f,  0.0f,  0.0f,  4.5f,  9.0f,  9.0f },
    // 2 tr:   M57.5,14 L53,18.5 L53,45 L57.5,50 L62,45 L62,18.5
    { 57.5f, 53.0f, 53.0f, 57.5f, 62.0f, 62.0f },
    // 3 mid:  M14,50 L48,50 L54,54.5 L48,59 L14,59 L8,54.5
    { 14.0f, 48.0f, 54.0f, 48.0f, 14.0f,  8.0f },
    // 4 bl:   M4.5,65 L0,69.5 L0,96 L4.5,101 L9,96 L9,69.5
    {  4.5f,  0.0f,  0.0f,  4.5f,  9.0f,  9.0f },
    // 5 br:   M57.5,65 L53,69.5 L53,96 L57.5,101 L62,96 L62,69.5
    { 57.5f, 53.0f, 53.0f, 57.5f, 62.0f, 62.0f },
    // 6 bot:  M14,101 L48,101 L54,105.5 L48,110 L14,110 L8,105.5
    { 14.0f, 48.0f, 54.0f, 48.0f, 14.0f,  8.0f },
};

static const float SEG_VY[7][6] = {
    // 0 top
    {  0.0f,  0.0f,  4.5f,  9.0f,  9.0f,  4.5f },
    // 1 tl
    { 14.0f, 18.5f, 45.0f, 50.0f, 45.0f, 18.5f },
    // 2 tr
    { 14.0f, 18.5f, 45.0f, 50.0f, 45.0f, 18.5f },
    // 3 mid
    { 50.0f, 50.0f, 54.5f, 59.0f, 59.0f, 54.5f },
    // 4 bl
    { 65.0f, 69.5f, 96.0f,101.0f, 96.0f, 69.5f },
    // 5 br
    { 65.0f, 69.5f, 96.0f,101.0f, 96.0f, 69.5f },
    // 6 bot
    {101.0f,101.0f,105.5f,110.0f,110.0f,105.5f },
};

// ── Digit→segment LUT (matches SEG_MAP in screens.jsx) ───────────────────────
// Index order: [top, tl, tr, mid, bl, br, bot]
static const uint8_t SEG_MAP[10][7] = {
    { 1,1,1, 0, 1,1,1 }, // 0
    { 0,0,1, 0, 0,1,0 }, // 1
    { 1,0,1, 1, 1,0,1 }, // 2
    { 1,0,1, 1, 0,1,1 }, // 3
    { 0,1,1, 1, 0,1,0 }, // 4
    { 1,1,0, 1, 0,1,1 }, // 5
    { 1,1,0, 1, 1,1,1 }, // 6
    { 1,0,1, 0, 0,1,0 }, // 7
    { 1,1,1, 1, 1,1,1 }, // 8
    { 1,1,1, 1, 0,1,1 }, // 9
};

// ── API ───────────────────────────────────────────────────────────────────────

void gfx_draw_7seg_digit(fb_t *fb, int x, int y, int digit,
                         float scale,
                         int active_color, int inactive_color)
{
    if (digit < 0 || digit > 9) digit = 8;
    const uint8_t *segs = SEG_MAP[digit];

    for (int s = 0; s < 7; s++) {
        int color = segs[s] ? active_color : inactive_color;
        if (color == -1) continue;
        gfx_fill_polygon(fb,
                         SEG_VX[s], SEG_VY[s], 6,
                         scale, x, y, color);
    }
}

void gfx_draw_7seg_colon(fb_t *fb, int x, int y, bool visible,
                         float scale, int on_color, int off_color)
{
    // SvgColon in screens.jsx (native 18×110 viewBox):
    //   <rect x="4.5" y="40" width="9" height="9" fill={on} />
    //   <rect x="4.5" y="65" width="9" height="9" fill={on} />
    int color = visible ? on_color : off_color;
    if (color == -1) return;

    // Scale rect position and size.
    int dot_x = x + (int)roundf(4.5f * scale);
    int dot_w = (int)roundf(9.0f  * scale);
    int dot_h = dot_w;  // square dot

    int dot1_y = y + (int)roundf(40.0f * scale);
    int dot2_y = y + (int)roundf(65.0f * scale);

    fb_fill_rect(fb, dot_x, dot1_y, dot_w, dot_h, color);
    fb_fill_rect(fb, dot_x, dot2_y, dot_w, dot_h, color);
}
