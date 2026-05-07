#pragma once
// gfx_primitives.h — scanline polygon rasterizer for 1-bit framebuffer
// Vertices are in "native" float space (e.g. 62×110 for 7-seg).
// scale + offset map native → framebuffer pixel coordinates.

#include "fb.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum supported vertex count per polygon call.
#define GFX_MAX_VERTS  16

// Fill a convex (or simple) polygon into the framebuffer.
//
//   pts_x, pts_y  — vertex arrays, length n, in native/design space
//   n             — vertex count (1..GFX_MAX_VERTS)
//   scale         — uniform scale applied before offset
//                     e.g. 116.0f/110.0f for clock digits
//   ox, oy        — integer offset (top-left of bounding box in framebuffer)
//   color         — FB_BLACK or FB_WHITE
//
// Scanline samples at pixel-centre (y + 0.5) — avoids edge cases on integer
// scanlines and handles fractional vertices (.5 coords in screens.jsx) cleanly.
void gfx_fill_polygon(fb_t *fb,
                      const float *pts_x, const float *pts_y, int n,
                      float scale, int ox, int oy, int color);

#ifdef __cplusplus
}
#endif
