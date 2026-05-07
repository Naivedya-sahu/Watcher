#pragma once
#include "fb.h"
#include "button.h"
#include <stdbool.h>

// ── EPD refresh mode aliases (match epd.h values) ────────────
#define EPD_FULL    0   // EPD_REFRESH_FULL
#define EPD_PARTIAL 1   // EPD_REFRESH_PARTIAL

// ── Button type aliases (shorter names for screen callbacks) ──
typedef button_id_t  btn_id_t;
typedef button_evt_t btn_evt_t;
#define BTN_1    BTN_ID_1
#define BTN_2    BTN_ID_2
#define BTN_3    BTN_ID_3
#define BTN_ENC  BTN_ID_ENC
#define BTN_SHORT BTN_EVT_SHORT
#define BTN_LONG  BTN_EVT_LONG

#ifdef __cplusplus
extern "C" {
#endif

// ── Extensible screen registry ────────────────────────────────
// To add a new screen:
//   1. Define a static screen_def_t in your screen's .cpp file.
//   2. Call screen_register(&your_screen) from main.cpp before screen_mgr_start().
//   3. Done. No central header or enum update needed.

typedef struct screen_def {
    const char *id;           // unique string: "clock", "alarm", ...
    const char *label;        // display label: "CLOCK"
    const char *group;        // grouping: "time", "work", "sys"

    // Lifecycle (all optional, NULL = noop)
    void (*enter)(void);                    // called when screen becomes active
    void (*exit)(void);                     // called when leaving
    void (*tick)(void);                     // called every 50ms from main loop
    void (*render)(fb_t *fb);              // draw to framebuffer

    // Input (all optional)
    void (*on_button)(btn_id_t id, btn_evt_t evt);
    void (*on_encoder)(int delta);          // +1=CW, -1=CCW
    void (*on_enc_click)(void);            // encoder button click

    // Set to true when this screen needs a re-render
    bool needs_render;
    bool force_full;           // next render = full refresh (set by screen)
} screen_def_t;

// ── Manager API ───────────────────────────────────────────────
void screen_register(screen_def_t *def);

// Call after all screens registered. Returns false if no screens.
bool screen_mgr_start(const char *initial_id);

// Tick: calls active screen's tick(). Returns true if render needed.
bool screen_mgr_tick(void);

// Render active screen to fb. Clears dirty flag.
// Returns EPD_FULL or EPD_PARTIAL (from epd.h).
int  screen_mgr_render(fb_t *fb);

// Input dispatch
void screen_mgr_button(btn_id_t id, btn_evt_t evt);
void screen_mgr_encoder(int delta);
void screen_mgr_enc_click(void);

// Navigation (callable from within screens)
void screen_goto(const char *id);
void screen_next(void);
void screen_prev(void);
void screen_force_full(void);    // mark next render as full
void screen_force_render(void);  // mark needs_render without changing screen

// Queries
const char *screen_current_id(void);
int         screen_count(void);
const screen_def_t *screen_get(int idx);
const screen_def_t *screen_find(const char *id);

#ifdef __cplusplus
}
#endif
