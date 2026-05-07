#include "time_svc.h"
#include "esp_sntp.h"
#include "esp_log.h"

static const char *TAG = "time_svc";
static bool s_synced = false;

// NTP sync callback — fires after successful SNTP sync
static void sntp_cb(struct timeval *tv) {
    s_synced = true;
    // settimeofday is called automatically by ESP-IDF SNTP before this cb
    ESP_LOGI(TAG, "NTP sync complete: epoch %ld", (long)tv->tv_sec);
}

void time_svc_init(void) {
    // No RTC: system clock starts at epoch 0.
    // Clock app will show "--:--" or "00:00" until NTP syncs.
    ESP_LOGI(TAG, "time_svc init (NTP-only, no RTC)");
}

void time_svc_sync_ntp(const char *tz, const char *ntp_server) {
    setenv("TZ", tz, 1);
    tzset();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, ntp_server);
    sntp_set_time_sync_notification_cb(sntp_cb);
    esp_sntp_init();
    ESP_LOGI(TAG, "NTP sync started: %s tz=%s", ntp_server, tz);
}

time_t time_svc_get(void) {
    time_t now;
    time(&now);
    return now;
}

bool time_svc_is_synced(void) {
    return s_synced;
}

void time_svc_set(time_t t) {
    struct timeval tv = {};
    tv.tv_sec = t;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
}
