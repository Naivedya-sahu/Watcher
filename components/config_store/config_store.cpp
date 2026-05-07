#include "config_store.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "cfg";
#define NVS_NS "watcher"

watcher_cfg_t g_cfg;

void cfg_defaults(void) {
    memset(&g_cfg, 0, sizeof(g_cfg));
    strncpy(g_cfg.wifi_ssid,    "",              sizeof(g_cfg.wifi_ssid) - 1);
    strncpy(g_cfg.wifi_pass,    "",              sizeof(g_cfg.wifi_pass) - 1);
    strncpy(g_cfg.device_name,  "watcher",       sizeof(g_cfg.device_name) - 1);
    g_cfg.ap_mode             = false;
    strncpy(g_cfg.timezone,     "IST-5:30",      sizeof(g_cfg.timezone) - 1);
    strncpy(g_cfg.ntp_server,   "pool.ntp.org",  sizeof(g_cfg.ntp_server) - 1);
    g_cfg.time_24h            = true;
    g_cfg.date_format         = 0;   // long
    g_cfg.pomo_focus_mins     = 25;
    g_cfg.pomo_break_mins     = 5;
    g_cfg.pomo_long_mins      = 15;
    g_cfg.pomo_cycles         = 3;   // 3 focus before long break
    g_cfg.buzzer_on           = true;
    g_cfg.theme_dark          = false;
    g_cfg.sleep_timeout_min   = 5;   // 5 min idle → sleep (matches HTML default)
    g_cfg.enc_pulses_per_step = 2;
    strncpy(g_cfg.ota_url, "", sizeof(g_cfg.ota_url) - 1);  // set via web portal
}

void cfg_load(void) {
    cfg_defaults();
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "NVS empty — using defaults");
        return;
    }

    size_t len;
#define LOADS(key, dst) do { len = sizeof(dst); nvs_get_str(h, key, dst, &len); } while(0)
#define LOADI(key, dst) do { int32_t v; if(nvs_get_i32(h, key, &v)==ESP_OK) dst=(int)v; } while(0)
#define LOADB(key, dst) do { uint8_t v; if(nvs_get_u8(h, key, &v)==ESP_OK) dst=(bool)v; } while(0)

    LOADS("ssid",       g_cfg.wifi_ssid);
    LOADS("pass",       g_cfg.wifi_pass);
    LOADS("devname",    g_cfg.device_name);
    LOADS("tz",         g_cfg.timezone);
    LOADS("ntp",        g_cfg.ntp_server);
    LOADB("apmode",     g_cfg.ap_mode);
    LOADB("h24",        g_cfg.time_24h);
    LOADI("datefmt",    g_cfg.date_format);
    LOADI("pfocus",     g_cfg.pomo_focus_mins);
    LOADI("pbreak",     g_cfg.pomo_break_mins);
    LOADI("plong",      g_cfg.pomo_long_mins);
    LOADI("pcycles",    g_cfg.pomo_cycles);
    LOADB("buzzer",     g_cfg.buzzer_on);
    LOADB("dark",       g_cfg.theme_dark);
    LOADI("sleepmin",   g_cfg.sleep_timeout_min);
    LOADI("encpps",     g_cfg.enc_pulses_per_step);
    LOADS("otaurl",     g_cfg.ota_url);

    nvs_close(h);
    ESP_LOGI(TAG, "Config loaded from NVS (ssid=%s)", g_cfg.wifi_ssid);
}

void cfg_save(void) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed"); return;
    }

#define SAVES(key, src) nvs_set_str(h, key, src)
#define SAVEI(key, src) nvs_set_i32(h, key, (int32_t)(src))
#define SAVEB(key, src) nvs_set_u8(h, key, (uint8_t)(src))

    SAVES("ssid",     g_cfg.wifi_ssid);
    SAVES("pass",     g_cfg.wifi_pass);
    SAVES("devname",  g_cfg.device_name);
    SAVES("tz",       g_cfg.timezone);
    SAVES("ntp",      g_cfg.ntp_server);
    SAVEB("apmode",   g_cfg.ap_mode);
    SAVEB("h24",      g_cfg.time_24h);
    SAVEI("datefmt",  g_cfg.date_format);
    SAVEI("pfocus",   g_cfg.pomo_focus_mins);
    SAVEI("pbreak",   g_cfg.pomo_break_mins);
    SAVEI("plong",    g_cfg.pomo_long_mins);
    SAVEI("pcycles",  g_cfg.pomo_cycles);
    SAVEB("buzzer",   g_cfg.buzzer_on);
    SAVEB("dark",     g_cfg.theme_dark);
    SAVEI("sleepmin", g_cfg.sleep_timeout_min);
    SAVEI("encpps",   g_cfg.enc_pulses_per_step);
    SAVES("otaurl",   g_cfg.ota_url);

    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Config saved");
}

void cfg_set_wifi(const char *ssid, const char *pass) {
    strncpy(g_cfg.wifi_ssid, ssid, sizeof(g_cfg.wifi_ssid) - 1);
    strncpy(g_cfg.wifi_pass, pass, sizeof(g_cfg.wifi_pass) - 1);
    cfg_save();
}

void cfg_set_ap_mode(bool ap) {
    g_cfg.ap_mode = ap;
    cfg_save();
}

void cfg_set_time_format(bool h24, int date_fmt) {
    g_cfg.time_24h     = h24;
    g_cfg.date_format  = date_fmt;
    cfg_save();
}

void cfg_set_pomo(int focus, int brk, int lng, int cycles) {
    g_cfg.pomo_focus_mins = focus;
    g_cfg.pomo_break_mins = brk;
    g_cfg.pomo_long_mins  = lng;
    g_cfg.pomo_cycles     = cycles;
    cfg_save();
}

void cfg_set_buzzer(bool on) {
    g_cfg.buzzer_on = on;
    cfg_save();
}
