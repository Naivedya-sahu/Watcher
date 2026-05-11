// settings_screen.cpp — device config display (read-only on v8 hardware)
// 8 rows: 0 WIFI · 1 SIGNAL · 2 IP · 3 THEME · 4 FIRMWARE · 5 UPDATE · 6 TIME ZONE · 7 SLEEP
// Input: encoder scroll/click (sim only — ENABLE_INPUTS=0 in v8)
// Config changes: use web console at http://watcher.local

#include "settings_screen.h"
#include "screen_mgr.h"
#include "fb.h"
#include "config_store.h"
#include "time_svc.h"
#include "ota_task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "settings";

#define ROW_H     32
#define ROW_Y0    22   // just below header hline at y=20
#define ROW_BODY (ROW_H - 2)   // 2 px separator at y+ROW_BODY
#define MAX_ROWS   8   // matches HTML: WIFI/SIGNAL/IP/THEME/FIRMWARE/UPDATE/TIME ZONE/SLEEP

static int s_focus = 0;

// HTML settings.html row keys — order must match HTML ROWS array exactly.
static const char *ROW_KEYS[MAX_ROWS] = {
    "WIFI", "SIGNAL", "IP", "THEME", "FIRMWARE", "UPDATE", "TIME ZONE", "SLEEP"
};

static void get_row_val(int idx, char *buf, size_t cap) {
    switch (idx) {
        case 0:
            snprintf(buf, cap, "%s", g_cfg.wifi_ssid[0] ? g_cfg.wifi_ssid : "NOT SET");
            break;

        case 1: {
            // Real RSSI from last association.  Only valid in STA mode after
            // connect; returns ESP_ERR_WIFI_NOT_CONNECT if not associated yet.
            wifi_mode_t mode = WIFI_MODE_NULL;
            esp_wifi_get_mode(&mode);
            if (!g_cfg.ap_mode && mode == WIFI_MODE_STA) {
                wifi_ap_record_t ap = {};
                if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
                    // Build a simple bar indicator from RSSI (ASCII, font-safe):
                    //   > -60  = ||||  (excellent)
                    //   > -70  = |||.
                    //   > -80  = ||..
                    //   <= -80 = |...
                    const char *bars =
                        ap.rssi > -60 ? "||||" :
                        ap.rssi > -70 ? "|||." :
                        ap.rssi > -80 ? "||.." :
                                        "|...";
                    snprintf(buf, cap, "%d dBm %s", ap.rssi, bars);
                } else {
                    snprintf(buf, cap, "-- dBm");
                }
            } else {
                snprintf(buf, cap, "AP MODE");
            }
            break;
        }

        case 2: {
            // Real IP address from the active netif.
            wifi_mode_t mode = WIFI_MODE_NULL;
            esp_wifi_get_mode(&mode);
            if (g_cfg.ap_mode || mode == WIFI_MODE_AP) {
                // AP mode: default gateway is always 192.168.4.1
                snprintf(buf, cap, "192.168.4.1");
            } else {
                // STA mode: retrieve assigned IP from the default STA netif.
                esp_netif_t *sta_if = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                esp_netif_ip_info_t ip_info = {};
                if (sta_if && esp_netif_get_ip_info(sta_if, &ip_info) == ESP_OK
                           && ip_info.ip.addr != 0) {
                    snprintf(buf, cap, IPSTR, IP2STR(&ip_info.ip));
                } else {
                    snprintf(buf, cap, "---");
                }
            }
            break;
        }

        case 3:
            snprintf(buf, cap, "%s", g_cfg.theme_dark ? "DARK" : "LIGHT");
            break;

        case 4: {
            const esp_app_desc_t *d = esp_app_get_description();
            snprintf(buf, cap, "v%s", d ? d->version : "7.1.0");
            break;
        }

        case 5:
            if (ota_is_running())
                snprintf(buf, cap, "UPDATING...");
            else if (ota_is_armed())
                snprintf(buf, cap, "ARMED");
            else
                snprintf(buf, cap, "IDLE");
            break;

        case 6:
            // POSIX TZ string from NVS (e.g. "IST-5:30", "UTC0", "EST5EDT")
            snprintf(buf, cap, "%s", g_cfg.timezone[0] ? g_cfg.timezone : "UTC0");
            break;

        case 7: {
            // Sleep timeout: display as "N MIN" or "OFF"
            if (g_cfg.sleep_timeout_min <= 0)
                snprintf(buf, cap, "OFF");
            else
                snprintf(buf, cap, "%d MIN", g_cfg.sleep_timeout_min);
            break;
        }

        default:
            buf[0] = '\0';
    }
}

static void settings_render(fb_t *fb) {
    // ── Header — "SETTINGS" left, "N/8" right ─────────────────
    fb_draw_str(fb, 32, 8, "SETTINGS", FB_BLACK);
    char pg[24];
    snprintf(pg, sizeof(pg), "%d/%d", s_focus + 1, MAX_ROWS);
    int pgw = (int)strlen(pg) * FONT_W;
    fb_draw_str(fb, 368 - pgw, 8, pg, FB_BLACK);
    fb_draw_hline(fb, 0, 20, FB_W, FB_BLACK);

    for (int i = 0; i < MAX_ROWS; i++) {
        int y    = ROW_Y0 + i * ROW_H;
        int ty   = y + (ROW_BODY - FONT_H) / 2;   // vertically centred text y
        bool focused = (i == s_focus);

        char val[48];
        get_row_val(i, val, sizeof(val));
        int vw = (int)strlen(val) * FONT_W;

        if (focused) {
            // Full row inversion — black background, white text
            fb_fill_rect(fb, 0, y, FB_W, ROW_BODY, FB_BLACK);
            fb_draw_str(fb, 32,         ty, ROW_KEYS[i], FB_WHITE);
            fb_draw_str(fb, 368 - vw,   ty, val,         FB_WHITE);
        } else {
            fb_draw_str(fb, 32,         ty, ROW_KEYS[i], FB_BLACK);
            fb_draw_str(fb, 368 - vw,   ty, val,         FB_BLACK);
        }

        if (i < MAX_ROWS - 1)
            fb_draw_hline(fb, 0, y + ROW_BODY, FB_W, FB_BLACK);
    }

    // ── Footer — split left/right ──────────────────────────────
    fb_draw_hline(fb, 0, 278, FB_W, FB_BLACK);
    fb_draw_str(fb, 8, 284, "watcher.local", FB_BLACK);
    int selw = (int)strlen("WEB CONSOLE") * FONT_W;
    fb_draw_str(fb, 392 - selw, 284, "WEB CONSOLE", FB_BLACK);
}

static void settings_select(int row) {
    switch (row) {
        case 0: // WIFI — toggle AP/STA mode (full credential change via web portal)
            g_cfg.ap_mode = !g_cfg.ap_mode;
            cfg_save();
            screen_force_render();
            ESP_LOGI(TAG, "AP mode toggled → %s (restart to apply)", g_cfg.ap_mode ? "ON" : "OFF");
            break;
        case 3: // THEME — toggle light/dark; force full refresh to avoid ghosting on inversion
            g_cfg.theme_dark = !g_cfg.theme_dark;
            cfg_save();
            screen_force_full();
            screen_force_render();
            break;
        case 7: // SLEEP — cycle: OFF → 5 → 10 → 15 → 30 → OFF
            switch (g_cfg.sleep_timeout_min) {
                case  0: g_cfg.sleep_timeout_min =  5; break;
                case  5: g_cfg.sleep_timeout_min = 10; break;
                case 10: g_cfg.sleep_timeout_min = 15; break;
                case 15: g_cfg.sleep_timeout_min = 30; break;
                default: g_cfg.sleep_timeout_min =  0; break;
            }
            cfg_save();
            screen_force_render();
            break;
        case 5: // UPDATE — arm push OTA; host then POSTs binary to /api/ota/upload
            if (!ota_trigger(NULL))
                ESP_LOGW(TAG, "OTA busy — already armed or running");
            screen_force_render();
            break;
        default:
            // Rows 1/2/4/6 are read-only on device; edit via web portal
            break;
    }
}

static void settings_btn(btn_id_t id, btn_evt_t evt) {
    if (id == BTN_1 && evt == BTN_LONG)  { screen_goto("clock"); return; }
    if (id == BTN_1 && evt == BTN_SHORT) {
        s_focus = (s_focus - 1 + MAX_ROWS) % MAX_ROWS;
        screen_force_render();
    }
    if (id == BTN_2 && evt == BTN_SHORT) {
        s_focus = (s_focus + 1) % MAX_ROWS;
        screen_force_render();
    }
    if (id == BTN_3 && evt == BTN_SHORT) { settings_select(s_focus); }
}

static void settings_enc(int delta) {
    s_focus = (s_focus + delta + MAX_ROWS) % MAX_ROWS;
    screen_force_render();
}
static void settings_enc_click(void) { settings_select(s_focus); }
static void settings_enter(void) { s_focus = 0; screen_force_render(); }

screen_def_t settings_screen = {
    .id           = "settings",
    .label        = "SETTINGS",
    .group        = "sys",
    .enter        = settings_enter,
    .exit         = NULL,
    .tick         = NULL,
    .render       = settings_render,
    .on_button    = settings_btn,
    .on_encoder   = settings_enc,
    .on_enc_click = settings_enc_click,
    .needs_render = true,
    .force_full   = true,
};
