#pragma once
#include "screen_mgr.h"
#include <stdbool.h>
#include <stdint.h>

extern screen_def_t pomo_screen;

// Timer controls
void pomo_start_stop(void);
void pomo_reset(void);

// State queries (safe to call from any task/component)
bool        pomo_is_running(void);
uint32_t    pomo_get_remaining_s(void);   // seconds left in current interval
const char *pomo_get_mode_str(void);      // "FOCUS" | "SHORT BREAK" | "LONG BREAK"
int         pomo_get_session(void);       // 1-based focus session count
