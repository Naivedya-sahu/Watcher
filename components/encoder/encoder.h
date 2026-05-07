#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── EC11 rotary encoder driver ────────────────────────────────
// Quadrature decoding via ISR on A and B channels.
// Optical mouse scroll wheel: no debounce needed on A/B.
// SW (click) uses the existing button driver; register with button_init.
//
// GPIO assignments (from board_config.h):
//   A  = IO35   (ISR ANYEDGE, 4.7K pull-up on peripheral board)
//   B  = IO36   (ISR ANYEDGE, 4.7K pull-up)
//   SW = IO37   (short/long press via button driver)

// Number of raw pulses per logical step (tune after hardware test)
#define ENC_PULSES_PER_STEP  2

typedef void (*enc_step_cb_t)(int delta);  // +1 = CW, -1 = CCW

// Install encoder ISR and start accumulating.
// cb fires every ENC_PULSES_PER_STEP pulses.
// Call from main task after gpio driver init.
void encoder_init(int pin_a, int pin_b, enc_step_cb_t cb);

// Poll: call from main loop (or timer) to dispatch accumulated steps.
// Fires cb for each logical step accumulated since last poll.
void encoder_poll(void);

// Get raw pulse counter (for debugging / calibration)
int  encoder_get_raw(void);

#ifdef __cplusplus
}
#endif
