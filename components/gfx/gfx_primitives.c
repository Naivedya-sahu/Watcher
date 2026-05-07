// gfx_primitives.c — scanline polygon rasterizer
#include "gfx_primitives.h"
#include <math.h>

void gfx_fill_polygon(fb_t *fb,
                      const float *pts_x, const float *pts_y, int n,
                      float scale, int ox, int oy, int color)
{
    if (n < 3 || n > GFX_MAX_VERTS) return;

    // Scale and translate into framebuffer coordinates.
    float vx[GFX_MAX_VERTS], vy[GFX_MAX_VERTS];
    float ymin =  1e9f, ymax = -1e9f;
    for (int i = 0; i < n; i++) {
        vx[i] = pts_x[i] * scale + (float)ox;
        vy[i] = pts_y[i] * scale + (float)oy;
        if (vy[i] < ymin) ymin = vy[i];
        if (vy[i] > ymax) ymax = vy[i];
    }

    int y_lo = (int)floorf(ymin);
    int y_hi = (int)ceilf(ymax);

    // Clamp to framebuffer bounds.
    if (y_lo < 0)      y_lo = 0;
    if (y_hi >= FB_H)  y_hi = FB_H - 1;

    float xs[GFX_MAX_VERTS];

    for (int y = y_lo; y <= y_hi; y++) {
        // Sample at pixel centre — no vertex will fall exactly here after
        // scaling by a non-integer (e.g. 116/110), so the crossing test is clean.
        float yf = (float)y + 0.5f;
        int cnt = 0;

        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            float ya = vy[i], yb = vy[j];

            // Skip horizontal edges entirely.
            if (ya == yb) continue;

            // Half-open interval: include bottom endpoint, exclude top.
            // This means: (ya < yf <= yb) OR (yb < yf <= ya)
            // Equivalent to: (ya < yf && yb >= yf) || (yb < yf && ya >= yf)
            if (!((ya < yf && yb >= yf) || (yb < yf && ya >= yf))) continue;

            float t  = (yf - ya) / (yb - ya);
            xs[cnt++] = vx[i] + t * (vx[j] - vx[i]);
        }

        if (cnt < 2) continue;

        // Insertion sort — polygon has ≤16 verts so n is tiny.
        for (int a = 1; a < cnt; a++) {
            float key = xs[a];
            int   b   = a - 1;
            while (b >= 0 && xs[b] > key) { xs[b + 1] = xs[b]; b--; }
            xs[b + 1] = key;
        }

        // Fill between intersection pairs (even–odd rule).
        for (int k = 0; k + 1 < cnt; k += 2) {
            int x0 = (int)floorf(xs[k]);
            int x1 = (int)ceilf(xs[k + 1]);
            if (x0 < 0)      x0 = 0;
            if (x1 >= FB_W)  x1 = FB_W - 1;
            for (int px = x0; px < x1; px++) {
                fb_set_pixel(fb, px, y, color);
            }
        }
    }
}
