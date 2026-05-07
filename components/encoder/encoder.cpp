#include "encoder.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

// ── ISR state (IRAM) ──────────────────────────────────────────
static volatile int  s_raw_count    = 0;  // accumulated half-steps
static volatile int  s_prev_state   = 0;  // AB state at last edge
static int           s_pin_a        = -1;
static int           s_pin_b        = -1;
static enc_step_cb_t s_cb           = NULL;
static int           s_dispatch_pos = 0;   // last dispatched count

// Gray-code transition table.
// Index = (prev_AB << 2) | curr_AB  (4 bits → 16 entries)
// +1 = CW step, -1 = CCW step, 0 = no change or invalid
static const int8_t TRANS[16] = {
     0, -1, +1,  0,   // prev=00 → 00,01,10,11
    +1,  0,  0, -1,   // prev=01 → 00,01,10,11
    -1,  0,  0, +1,   // prev=10 → 00,01,10,11
     0, +1, -1,  0    // prev=11 → 00,01,10,11
};

static void IRAM_ATTR enc_isr(void *arg) {
    int a = gpio_get_level((gpio_num_t)(intptr_t)arg == 0
                           ? (gpio_num_t)s_pin_a : (gpio_num_t)s_pin_a);
    // Read both pins each ISR (fired on either edge)
    int pa = gpio_get_level((gpio_num_t)s_pin_a);
    int pb = gpio_get_level((gpio_num_t)s_pin_b);
    int curr = (pa << 1) | pb;
    int idx  = (s_prev_state << 2) | curr;
    s_raw_count += TRANS[idx];
    s_prev_state = curr;
    (void)a;
}

void encoder_init(int pin_a, int pin_b, enc_step_cb_t cb) {
    s_pin_a = pin_a;
    s_pin_b = pin_b;
    s_cb    = cb;

    // Read initial state
    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << pin_a) | (1ULL << pin_b);
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_DISABLE;  // external pull-ups on peripheral board
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&io);

    int pa = gpio_get_level((gpio_num_t)pin_a);
    int pb = gpio_get_level((gpio_num_t)pin_b);
    s_prev_state = (pa << 1) | pb;

    // Install ISR service if not already running
    gpio_install_isr_service(0);
    // Both pins fire same ISR — the ISR always reads both levels
    gpio_isr_handler_add((gpio_num_t)pin_a, enc_isr, (void *)(intptr_t)0);
    gpio_isr_handler_add((gpio_num_t)pin_b, enc_isr, (void *)(intptr_t)1);
}

void encoder_poll(void) {
    if (!s_cb) return;
    // Read atomically (single int read on Xtensa is atomic)
    int curr = s_raw_count;
    int diff = curr - s_dispatch_pos;
    int steps = diff / ENC_PULSES_PER_STEP;
    if (steps == 0) return;
    s_dispatch_pos += steps * ENC_PULSES_PER_STEP;
    s_cb(steps > 0 ? +1 : -1);  // fire once per logical step (not per step count)
    // If |steps| > 1 (fast spin), fire remaining
    int remaining = abs(steps) - 1;
    int dir = steps > 0 ? +1 : -1;
    for (int i = 0; i < remaining; i++) s_cb(dir);
}

int encoder_get_raw(void) { return s_raw_count; }
