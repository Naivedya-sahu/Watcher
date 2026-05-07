#pragma once
// gfx_7seg.h — exact chamfered 7-segment digit renderer
//
// Vertices from SvgDigit / SvgColon in screens.jsx (native 62×110 / 18×110 space).
// Pass `scale = digitH / 110.0f` to render at any height.
//
//   Clock:  scale = 116.0f / 110.0f
//   Pomo:   scale = 150.0f / 110.0f

#include "fb.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Draw one 7-segment digit (0–9) with exact chamfered hexagon segments.
//
//   x, y           — top-left of the scaled bounding box in the framebuffer
//   digit          — 0..9 (clamped to 8 if out of range)
//   scale          — digitH / 110.0f
//   active_color   — FB_BLACK or FB_WHITE for lit segments
//   inactive_color — color for unlit ghost segments; pass -1 to skip entirely
void gfx_draw_7seg_digit(fb_t *fb, int x, int y, int digit,
                         float scale,
                         int active_color, int inactive_color);

// Draw a colon glyph (18×110 native, two 9×9 squares).
// Dot y-positions from SvgColon in screens.jsx: y=40 and y=65 in native space.
//
//   x, y     — top-left of the scaled 18-wide bounding box
//   visible  — false → dots not drawn (or drawn in off_color if not -1)
//   scale    — same as digit scale
//   on_color — color when visible=true
//   off_color— color when visible=false; -1 = don't draw invisible dots
void gfx_draw_7seg_colon(fb_t *fb, int x, int y, bool visible,
                         float scale, int on_color, int off_color);

#ifdef __cplusplus
}
#endif
