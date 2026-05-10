// firmware/main/watcher_nvs.h
#pragma once
#include <stddef.h>
#include "esp_err.h"

#define WATCHER_NVS_NS "watcher"

// Generic JSON-blob NVS slot. Caller free()s out_buf.
esp_err_t watcher_nvs_get_blob(const char *key, char **out_buf, size_t *out_len);
esp_err_t watcher_nvs_set_blob(const char *key, const char *buf, size_t len);

// Convenience init — opens the namespace once at boot.
esp_err_t watcher_nvs_init(void);
