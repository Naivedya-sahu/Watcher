// firmware/main/main.c
//
// Watcher entry point. Boots NVS, brings WiFi up (STA if creds in NVS, else
// AP-SETUP at 192.168.4.1), starts the HTTP/WS server, the EPD, and the
// hardware-button task.

#include <string.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "watcher_nvs.h"
#include "watcher_http.h"
#include "watcher_epd.h"
#include "watcher_buttons.h"

static const char *TAG = "watcher";

// ── wifi bring-up ───────────────────────────────────────────────────────────

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "sta disconnect — retrying");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "sta got ip");
        watcher_http_push_log("WIFI", "sta got ip");
    }
}

static void wifi_start(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    char *cfg_buf = NULL; size_t cfg_n = 0;
    watcher_nvs_get_blob("cfg", &cfg_buf, &cfg_n);
    cJSON *cfg = cfg_buf ? cJSON_Parse(cfg_buf) : NULL;

    bool ap_mode  = cJSON_IsTrue(cJSON_GetObjectItem(cfg, "apMode"));
    const char *ssid = cJSON_GetStringValue(cJSON_GetObjectItem(cfg, "wifiSsid"));
    const char *pass = cJSON_GetStringValue(cJSON_GetObjectItem(cfg, "wifiPass"));

    if (ap_mode || !ssid || !*ssid) {
        esp_netif_create_default_wifi_ap();
        wifi_init_config_t wic = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&wic));
        wifi_config_t wc = {0};
        strcpy((char*)wc.ap.ssid, "WATCHER-SETUP");
        wc.ap.ssid_len = strlen("WATCHER-SETUP");
        wc.ap.max_connection = 4;
        wc.ap.authmode = WIFI_AUTH_OPEN;
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wc));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "AP-SETUP up · 192.168.4.1");
    } else {
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t wic = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&wic));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, on_wifi_event, NULL));
        wifi_config_t wc = {0};
        strncpy((char*)wc.sta.ssid,     ssid, sizeof wc.sta.ssid - 1);
        if (pass) strncpy((char*)wc.sta.password, pass, sizeof wc.sta.password - 1);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_connect();
        ESP_LOGI(TAG, "STA → %s", ssid);
    }

    cJSON_Delete(cfg);
    free(cfg_buf);
}

// ── hw button bridge ───────────────────────────────────────────────────────

static void on_btn(int btn, int lp) {
    char buf[64];
    snprintf(buf, sizeof buf, "btn %d %s", btn, lp ? "long" : "short");
    watcher_http_push_log("BTN", buf);
    // dispatch into your screen state machine here
}

// ── app entry ──────────────────────────────────────────────────────────────

void app_main(void) {
    ESP_ERROR_CHECK(watcher_nvs_init());
    watcher_epd_init();
    wifi_start();
    ESP_ERROR_CHECK(watcher_http_start(NULL));
    watcher_buttons_start(on_btn);
    ESP_LOGI(TAG, "watcher up · listening :80 · /ws ready");
}
