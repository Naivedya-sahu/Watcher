// ota_task.cpp — push OTA upload state machine
//
// OTA flow:
//   1) arm mode (serial 'u' or cmd {"cmd":"ota"})
//   2) host uploads raw firmware bytes to /api/ota/upload
//   3) write + verify + set boot partition
//   4) caller reboots device

#include "ota_task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

static const char *TAG = "ota";

static volatile bool s_running = false;
static volatile bool s_armed = false;

static esp_ota_handle_t s_ota_handle = 0;
static const esp_partition_t *s_update_partition = NULL;
static size_t s_written = 0;
static bool s_push_active = false;

static void ota_reset_session_flags(void) {
    s_ota_handle = 0;
    s_update_partition = NULL;
    s_written = 0;
    s_push_active = false;
    s_running = false;
}

bool ota_is_running(void) {
    return s_running;
}

bool ota_is_armed(void) {
    return s_armed;
}

bool ota_trigger(const char *unused) {
    (void)unused;
    if (s_running || s_push_active) {
        ESP_LOGW(TAG, "OTA busy");
        return false;
    }
    s_armed = true;
    ESP_LOGI(TAG, "OTA mode armed; waiting for POST /api/ota/upload");
    return true;
}

bool ota_push_begin(size_t image_size) {
    if (!s_armed || s_running || s_push_active) {
        ESP_LOGW(TAG, "OTA begin rejected (armed=%d running=%d active=%d)", (int)s_armed, (int)s_running, (int)s_push_active);
        return false;
    }

    s_update_partition = esp_ota_get_next_update_partition(NULL);
    if (!s_update_partition) {
        ESP_LOGE(TAG, "No OTA partition available");
        ota_reset_session_flags();
        return false;
    }

    esp_err_t err = esp_ota_begin(
        s_update_partition,
        (image_size > 0) ? image_size : OTA_SIZE_UNKNOWN,
        &s_ota_handle
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        ota_reset_session_flags();
        return false;
    }

    s_running = true;
    s_push_active = true;
    s_written = 0;
    ESP_LOGI(TAG, "OTA upload begin (target=%s, size=%u)", s_update_partition->label, (unsigned)image_size);
    return true;
}

bool ota_push_write(const uint8_t *data, size_t len) {
    if (!s_push_active || !data || len == 0) {
        return false;
    }

    esp_err_t err = esp_ota_write(s_ota_handle, data, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
        ota_push_abort();
        return false;
    }

    s_written += len;
    return true;
}

bool ota_push_end(void) {
    if (!s_push_active) {
        return false;
    }

    esp_err_t err = esp_ota_end(s_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        ota_push_abort();
        return false;
    }

    err = esp_ota_set_boot_partition(s_update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        ota_reset_session_flags();
        s_armed = false;
        return false;
    }

    ESP_LOGI(TAG, "OTA upload complete (%u bytes), boot partition updated", (unsigned)s_written);
    ota_reset_session_flags();
    s_armed = false;
    return true;
}

void ota_push_abort(void) {
    if (s_push_active) {
        esp_ota_abort(s_ota_handle);
        ESP_LOGW(TAG, "OTA upload aborted");
    }
    ota_reset_session_flags();
    s_armed = false;
}
