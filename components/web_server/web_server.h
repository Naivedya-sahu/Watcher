#pragma once
#include "fb.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Start HTTP server. fb used for /api/bitmap.
void web_server_start(fb_t *fb);
void web_server_stop(void);

// Push state JSON to all connected WS clients (call on every state change).
void web_server_push_state(void);

// Call from main loop — dispatches any pending commands queued from WS/REST.
void web_server_poll(void);

// Register runtime callbacks provided by `main` to avoid linking directly
// against application symbols. Call from `main` before `web_server_start()`.
void web_server_set_screen_callbacks(
	const char *(*cb_current_id)(void),
	void (*cb_goto)(const char *id),
	void (*cb_force_render)(void)
);

void web_server_set_pomo_time_callbacks(
	void (*cb_pomo_start_stop)(void),
	void (*cb_pomo_reset)(void),
	bool (*cb_pomo_is_running)(void),
	uint32_t (*cb_pomo_get_remaining_s)(void),
	const char *(*cb_pomo_get_mode_str)(void),
	int (*cb_pomo_get_session)(void),
	bool (*cb_time_svc_is_synced)(void)
);

// Register callback invoked after a browser bitmap push.
// Callback should call fb_flush(fb, EPD_REFRESH_FULL) on hardware.
void web_server_set_push_callback(void (*cb_push_raw)(void));

#ifdef __cplusplus
}
#endif
