#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { int w, h; } fb_t;

#define EPD_REFRESH_FULL 1
#define EPD_REFRESH_PARTIAL 2

#define FB_BLACK 1
#define FB_WHITE 0

static inline void fb_init(fb_t* f) { (void)f; }
static inline void fb_clear(fb_t* f) { (void)f; }
static inline void fb_flush(fb_t* f, int mode) { (void)f; (void)mode; }
static inline void fb_draw_rect(fb_t* f, int x, int y, int w, int h, int color) { (void)f; (void)x; (void)y; (void)w; (void)h; (void)color; }
static inline void fb_draw_7seg_digit(fb_t* f, int x, int y, int size, int fg, int bg) { (void)f; (void)x; (void)y; (void)size; (void)fg; (void)bg; }
static inline void fb_draw_7seg_colon(fb_t* f, int x, int y, int on, int fg) { (void)f; (void)x; (void)y; (void)on; (void)fg; }
static inline void fb_draw_str_centered(fb_t* f, int cx, int y, const char* s, int fg) { (void)f; (void)cx; (void)y; (void)s; (void)fg; }
static inline void fb_draw_hline(fb_t* f, int x, int y, int len, int fg) { (void)f; (void)x; (void)y; (void)len; (void)fg; }
static inline void fb_draw_str(fb_t* f, int x, int y, const char* s, int fg) { (void)f; (void)x; (void)y; (void)s; (void)fg; }
static inline void fb_draw_dot_ring(fb_t* f, int filled, int fg, int bg) { (void)f; (void)filled; (void)fg; (void)bg; }

#ifdef __cplusplus
}
#endif
