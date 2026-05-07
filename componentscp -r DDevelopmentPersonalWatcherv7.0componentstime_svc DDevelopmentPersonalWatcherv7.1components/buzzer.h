#pragma once
#include <stdint.h>

// Named tone patterns — no ermine animation, no sprite code
typedef enum {
    BUZZ_BOOT        = 0,  // 800Hz 80ms → 1200Hz 80ms
    BUZZ_TICK        = 1,  // 2000Hz 1ms
    BUZZ_SUCCESS     = 2,  // 1000Hz 100ms → 1500Hz 150ms
    BUZZ_ERROR       = 3,  // 300Hz 400ms
    BUZZ_ALERT       = 4,  // 1800Hz 80ms × 3
    BUZZ_POMO_START  = 5,  // same as SUCCESS
    BUZZ_POMO_DONE   = 6,  // 1000→1200→1500Hz ascending
} buzzer_pattern_t;

void buzzer_init(int gpio_pin);
void buzzer_beep(uint32_t freq_hz, uint32_t ms);
void buzzer_tone(buzzer_pattern_t pattern);
