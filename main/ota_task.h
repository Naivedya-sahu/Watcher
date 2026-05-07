#pragma once
// ota_task.h — push OTA state machine
//
// Workflow:
//   1) Arm OTA mode via ota_trigger(NULL) or serial 'u'
//   2) Upload firmware binary to POST /api/ota/upload
//   3) Device writes update partition and reboots

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Arm OTA upload mode. Returns false if OTA is already busy.
bool ota_trigger(const char *unused);

// True while OTA write/verify is in progress.
bool ota_is_running(void);

// True when OTA upload mode is armed and waiting for binary upload.
bool ota_is_armed(void);

// Upload session primitives used by web_server /api/ota/upload.
bool ota_push_begin(size_t image_size);
bool ota_push_write(const uint8_t *data, size_t len);
bool ota_push_end(void);
void ota_push_abort(void);

#ifdef __cplusplus
}
#endif
