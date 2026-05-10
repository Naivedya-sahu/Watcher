// firmware/main/watcher_nvs.c
#include "watcher_nvs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "watcher_nvs";

esp_err_t watcher_nvs_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t watcher_nvs_get_blob(const char *key, char **out_buf, size_t *out_len) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(WATCHER_NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    size_t len = 0;
    err = nvs_get_blob(h, key, NULL, &len);
    if (err != ESP_OK || len == 0) { nvs_close(h); *out_buf = NULL; *out_len = 0; return err; }

    char *buf = malloc(len + 1);
    if (!buf) { nvs_close(h); return ESP_ERR_NO_MEM; }
    err = nvs_get_blob(h, key, buf, &len);
    nvs_close(h);
    if (err != ESP_OK) { free(buf); return err; }
    buf[len] = '\0';
    *out_buf = buf; *out_len = len;
    return ESP_OK;
}

esp_err_t watcher_nvs_set_blob(const char *key, const char *buf, size_t len) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(WATCHER_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, key, buf, len);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err == ESP_OK) ESP_LOGI(TAG, "wrote %s (%u bytes)", key, (unsigned)len);
    return err;
}
