#pragma once
#include <stdint.h>

// ISR-based button driver — short/long press detection
// Active-low, 10K external pull-ups on peripheral board
// Supports up to BTN_MAX_N buttons (default 4: 3 tactile + encoder click)

#define BTN_MAX_N 4

typedef enum {
    BTN_ID_1   = 0,   // IO39 — back / prev
    BTN_ID_2   = 1,   // IO40 — next / context
    BTN_ID_3   = 2,   // IO41 — Pomodoro toggle / long=stop+reset
    BTN_ID_ENC = 3,   // IO37 — encoder click / select
} button_id_t;

typedef enum {
    BTN_EVT_SHORT = 0,
    BTN_EVT_LONG  = 1,
} button_evt_t;

typedef void (*button_cb_t)(button_id_t id, button_evt_t evt);

// pins: array of GPIO numbers, n: number of buttons (≤ BTN_MAX_N)
// long_ms: long press threshold in milliseconds
void button_init(const int *pins, int n, uint32_t long_ms, button_cb_t cb);
void button_tick(void);  // call from main loop task, ~10–20ms interval
