// web_server.cpp — HTTP + WebSocket server for Watcher v8.0
// Serves the standalone React web console at /
// REST API for state query and command dispatch.
// Note: React console is standalone — REST/WS is for future connectivity
//       and for other HTTP clients (curl, Home Assistant, etc.)

#include "web_server.h"
#include "config_store.h"
#include "fb.h"
#include "buzzer.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "esp_app_desc.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// OTA helpers are implemented in main/ota_task.cpp; keep this component
// decoupled from the main component by forward-declaring only what we use.
extern "C" {
bool ota_trigger(const char *url_override);
bool ota_is_running(void);
bool ota_is_armed(void);
bool ota_push_begin(size_t image_size);
bool ota_push_write(const uint8_t *data, size_t len);
bool ota_push_end(void);
void ota_push_abort(void);
}

// Alarm + task JSON helpers (alarm_screen.cpp / tasks_screen.cpp)
char *alarm_get_json(void);
bool  alarm_set_json(const char *json);
char *tasks_get_json(void);
bool  tasks_set_json(const char *json);

// screen_mgr forward declarations (avoid circular include)
const char *screen_current_id(void);
void        screen_goto(const char *id);
void        screen_force_render(void);
// pomo controls + state queries
void        pomo_start_stop(void);
void        pomo_reset(void);
bool        pomo_is_running(void);
uint32_t    pomo_get_remaining_s(void);
const char *pomo_get_mode_str(void);
int         pomo_get_session(void);
// time sync status query
bool time_svc_is_synced(void);   // returns true after first NTP sync

static const char *TAG = "ws_srv";

static uint32_t s_bitmap_rev = 0;  // incremented each EPD flush; webconsole polls for change
static httpd_handle_t s_server = NULL;
static fb_t          *s_fb     = NULL;
static void (*cb_push_raw)(void) = NULL;

// Callback pointers registered by `main` at runtime to avoid link-time
// dependencies on application symbols.
static const char *(*cb_screen_current_id)(void) = NULL;
static void (*cb_screen_goto)(const char *) = NULL;
static void (*cb_screen_force_render)(void) = NULL;

static void (*cb_pomo_start_stop)(void) = NULL;
static void (*cb_pomo_reset)(void) = NULL;
static bool (*cb_pomo_is_running)(void) = NULL;
static uint32_t (*cb_pomo_get_remaining_s)(void) = NULL;
static const char *(*cb_pomo_get_mode_str)(void) = NULL;
static int (*cb_pomo_get_session)(void) = NULL;
static bool (*cb_time_svc_is_synced)(void) = NULL;
static void (*cb_btn)(int btn_id, int evt) = NULL;
static void (*cb_enc)(int delta)           = NULL;

void web_server_set_screen_callbacks(
    const char *(*cb_current_id)(void),
    void (*cb_goto)(const char *id),
    void (*cb_force_render)(void)
){
    cb_screen_current_id = cb_current_id;
    cb_screen_goto = cb_goto;
    cb_screen_force_render = cb_force_render;
}

void web_server_set_push_callback(void (*cb)(void)) {
    cb_push_raw = cb;
}

void web_server_set_input_callbacks(void (*cb_btn_in)(int, int), void (*cb_enc_in)(int)) {
    cb_btn = cb_btn_in;
    cb_enc = cb_enc_in;
}

void web_server_set_pomo_time_callbacks(
    void (*cb_pomo_start_stop_in)(void),
    void (*cb_pomo_reset_in)(void),
    bool (*cb_pomo_is_running_in)(void),
    uint32_t (*cb_pomo_get_remaining_s_in)(void),
    const char *(*cb_pomo_get_mode_str_in)(void),
    int (*cb_pomo_get_session_in)(void),
    bool (*cb_time_svc_is_synced_in)(void)
){
    cb_pomo_start_stop = cb_pomo_start_stop_in;
    cb_pomo_reset = cb_pomo_reset_in;
    cb_pomo_is_running = cb_pomo_is_running_in;
    cb_pomo_get_remaining_s = cb_pomo_get_remaining_s_in;
    cb_pomo_get_mode_str = cb_pomo_get_mode_str_in;
    cb_pomo_get_session = cb_pomo_get_session_in;
    cb_time_svc_is_synced = cb_time_svc_is_synced_in;
}

// ── WebSocket client table ────────────────────────────────────
#define MAX_WS 4
static int s_ws_fds[MAX_WS];
static int s_ws_n = 0;

static void ws_add(int fd) {
    for (int i = 0; i < s_ws_n; i++) if (s_ws_fds[i] == fd) return;
    if (s_ws_n < MAX_WS) s_ws_fds[s_ws_n++] = fd;
}
static void ws_remove(int fd) {
    for (int i = 0; i < s_ws_n; i++)
        if (s_ws_fds[i] == fd) { s_ws_fds[i] = s_ws_fds[--s_ws_n]; return; }
}

// ── State JSON ────────────────────────────────────────────────
// Complete device snapshot sent to WS clients and returned by GET /api/state.
static char *build_state_json(void) {
    const esp_app_desc_t *app = esp_app_get_description();
    cJSON *r = cJSON_CreateObject();

    // ── System ──────────────────────────────────────────────
    cJSON_AddStringToObject(r, "screen",       cb_screen_current_id ? cb_screen_current_id() : "");
    cJSON_AddStringToObject(r, "fw_ver",       app ? app->version : "8.0.0");
    cJSON_AddBoolToObject  (r, "ap_mode",      g_cfg.ap_mode);
    cJSON_AddStringToObject(r, "ssid",         g_cfg.wifi_ssid);
    cJSON_AddStringToObject(r, "tz",           g_cfg.timezone);
    cJSON_AddBoolToObject  (r, "ntp_synced",   cb_time_svc_is_synced ? cb_time_svc_is_synced() : false);
    cJSON_AddBoolToObject  (r, "ota_running",  ota_is_running());
    cJSON_AddBoolToObject  (r, "ota_armed",    ota_is_armed());
    cJSON_AddStringToObject(r, "ota_url",      g_cfg.ota_url);

    // ── Device identity / prefs ─────────────────────────────
    cJSON_AddStringToObject(r, "device_name",       g_cfg.device_name[0] ? g_cfg.device_name : "watcher");
    cJSON_AddStringToObject(r, "ntp_server",        g_cfg.ntp_server);
    cJSON_AddBoolToObject  (r, "buzzer",            g_cfg.buzzer_on);
    cJSON_AddBoolToObject  (r, "dark",              g_cfg.theme_dark);
    cJSON_AddBoolToObject  (r, "h24",               g_cfg.time_24h);
    cJSON_AddNumberToObject(r, "date_format",       g_cfg.date_format);
    cJSON_AddNumberToObject(r, "sleep_timeout_min", g_cfg.sleep_timeout_min);

    // ── Runtime telemetry ────────────────────────────────────
    cJSON_AddNumberToObject(r, "heap_free", (double)esp_get_free_heap_size());
    cJSON_AddNumberToObject(r, "uptime_s",  (double)(esp_timer_get_time() / 1000000ULL));

    // ── Network ──────────────────────────────────────────────
    if (!g_cfg.ap_mode) {
        esp_netif_t *sta_if = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (sta_if) {
            esp_netif_ip_info_t ip_info = {};
            if (esp_netif_get_ip_info(sta_if, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
                char ip_str[16];
                snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
                cJSON_AddStringToObject(r, "ip", ip_str);
            } else {
                cJSON_AddStringToObject(r, "ip", "");
            }
        }
        wifi_ap_record_t ap = {};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            cJSON_AddNumberToObject(r, "rssi", ap.rssi);
        }
    } else {
        cJSON_AddStringToObject(r, "ip", "192.168.4.1");
        cJSON_AddNumberToObject(r, "rssi", 0);
    }

    // ── Pomodoro live state ──────────────────────────────────
    cJSON_AddBoolToObject  (r, "pomo_running",     cb_pomo_is_running ? cb_pomo_is_running() : false);
    cJSON_AddNumberToObject(r, "pomo_remaining_s", cb_pomo_get_remaining_s ? (double)cb_pomo_get_remaining_s() : 0.0);
    cJSON_AddStringToObject(r, "pomo_mode",        cb_pomo_get_mode_str ? cb_pomo_get_mode_str() : "");
    cJSON_AddNumberToObject(r, "pomo_session",     cb_pomo_get_session ? cb_pomo_get_session() : 0);

    // ── Pomodoro config ──────────────────────────────────────
    cJSON_AddNumberToObject(r, "focus_m",  g_cfg.pomo_focus_mins);
    cJSON_AddNumberToObject(r, "break_m",  g_cfg.pomo_break_mins);
    cJSON_AddNumberToObject(r, "long_m",   g_cfg.pomo_long_mins);
    cJSON_AddNumberToObject(r, "cycles",   g_cfg.pomo_cycles);

    // ── Bitmap revision — webconsole re-fetches /api/bitmap on change ─
    cJSON_AddNumberToObject(r, "bitmap_rev", (double)s_bitmap_rev);

    char *s = cJSON_PrintUnformatted(r);
    cJSON_Delete(r);
    return s;
}

// ── Helpers ───────────────────────────────────────────────────
static esp_err_t send_json(httpd_req_t *req, const char *json) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_sendstr(req, json);
}
static esp_err_t send_ok(httpd_req_t *req) {
    return send_json(req, "{\"ok\":true}");
}
static esp_err_t send_err(httpd_req_t *req, const char *msg) {
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"ok\":false,\"err\":\"%s\"}", msg);
    return send_json(req, buf);
}

// ── GET / — serve the main browser console from SPIFFS ───────
static esp_err_t handle_root(httpd_req_t *req) {
    // In AP mode, serve minimal WiFi setup page; otherwise full console
    const char *html_file = g_cfg.ap_mode 
        ? "/spiffs/www/wifi_setup.html"
        : "/spiffs/www/webserver.html";
    
    FILE *f = fopen(html_file, "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            g_cfg.ap_mode ? "wifi_setup.html not found" : "webserver.html not found");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    // Stream file in 4KB chunks to avoid OOM on large file
    char *buf = (char *)malloc(4096);
    if (!buf) { fclose(f); httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM"); return ESP_OK; }

    size_t n;
    while ((n = fread(buf, 1, 4096, f)) > 0) {
        httpd_resp_send_chunk(req, buf, n);
    }
    httpd_resp_send_chunk(req, NULL, 0);  // end chunk
    free(buf);
    fclose(f);
    return ESP_OK;
}

// ── GET /api/state ────────────────────────────────────────────
static esp_err_t handle_state(httpd_req_t *req) {
    char *json = build_state_json();
    esp_err_t ret = send_json(req, json ? json : "{}");
    free(json);
    return ret;
}

// ── POST /api/cmd — unified command endpoint ──────────────────
// Body: {"cmd": "screen", "screen": "pomo"}
// Body: {"cmd": "pomo_toggle"}
// Body: {"cmd": "pomo_reset"}
// Body: {"cmd": "buzzer", "tone": "success"}
// Body: {"cmd": "cfg", "key": "ssid", "val": "MyWifi"}
static esp_err_t handle_cmd(httpd_req_t *req) {
    int len = req->content_len;
    if (len <= 0 || len > 512) return send_err(req, "bad body");

    char *body = (char *)malloc(len + 1);
    if (!body) return send_err(req, "oom");
    httpd_req_recv(req, body, len);
    body[len] = '\0';

    cJSON *j = cJSON_Parse(body);
    free(body);
    if (!j) return send_err(req, "bad json");

    const char *cmd = cJSON_GetStringValue(cJSON_GetObjectItem(j, "cmd"));
    if (!cmd) { cJSON_Delete(j); return send_err(req, "no cmd"); }

    if (strcmp(cmd, "screen") == 0) {
        const char *scr = cJSON_GetStringValue(cJSON_GetObjectItem(j, "screen"));
        if (scr && cb_screen_goto) { cb_screen_goto(scr); }

    } else if (strcmp(cmd, "pomo_toggle") == 0) {
        pomo_start_stop();

    } else if (strcmp(cmd, "pomo_reset") == 0) {
        pomo_reset();

    } else if (strcmp(cmd, "buzzer") == 0) {
        const char *tone = cJSON_GetStringValue(cJSON_GetObjectItem(j, "tone"));
        buzzer_pattern_t t = BUZZ_TICK;
        if      (tone && !strcmp(tone,"boot"))       t = BUZZ_BOOT;
        else if (tone && !strcmp(tone,"success"))    t = BUZZ_SUCCESS;
        else if (tone && !strcmp(tone,"error"))      t = BUZZ_ERROR;
        else if (tone && !strcmp(tone,"alert"))      t = BUZZ_ALERT;
        else if (tone && !strcmp(tone,"pomo_start")) t = BUZZ_POMO_START;
        else if (tone && !strcmp(tone,"pomo_done"))  t = BUZZ_POMO_DONE;
        if (g_cfg.buzzer_on) buzzer_tone(t);

    } else if (strcmp(cmd, "cfg") == 0) {
        const char *key = cJSON_GetStringValue(cJSON_GetObjectItem(j, "key"));
        const char *val = cJSON_GetStringValue(cJSON_GetObjectItem(j, "val"));
        cJSON *nval     = cJSON_GetObjectItem(j, "nval");
        if (key && val) {
            if (!strcmp(key,"ssid"))    strncpy(g_cfg.wifi_ssid,   val, sizeof(g_cfg.wifi_ssid)-1);
            if (!strcmp(key,"pass"))    strncpy(g_cfg.wifi_pass,   val, sizeof(g_cfg.wifi_pass)-1);
            if (!strcmp(key,"tz"))      strncpy(g_cfg.timezone,    val, sizeof(g_cfg.timezone)-1);
            if (!strcmp(key,"ntp"))     strncpy(g_cfg.ntp_server,  val, sizeof(g_cfg.ntp_server)-1);
            if (!strcmp(key,"devname")) strncpy(g_cfg.device_name, val, sizeof(g_cfg.device_name)-1);
            if (!strcmp(key,"ota_url")) strncpy(g_cfg.ota_url,     val, sizeof(g_cfg.ota_url)-1);
            cfg_save();
        } else if (key && nval) {
            if (!strcmp(key,"buzzer"))    g_cfg.buzzer_on        = cJSON_IsTrue(nval);
            if (!strcmp(key,"dark"))      g_cfg.theme_dark       = cJSON_IsTrue(nval);
            if (!strcmp(key,"h24"))       g_cfg.time_24h         = cJSON_IsTrue(nval);
            if (!strcmp(key,"ap_mode"))   g_cfg.ap_mode          = cJSON_IsTrue(nval);
            if (!strcmp(key,"datefmt"))   g_cfg.date_format      = (int)cJSON_GetNumberValue(nval);
            if (!strcmp(key,"focus_m"))   g_cfg.pomo_focus_mins  = (int)cJSON_GetNumberValue(nval);
            if (!strcmp(key,"break_m"))   g_cfg.pomo_break_mins  = (int)cJSON_GetNumberValue(nval);
            if (!strcmp(key,"long_m"))    g_cfg.pomo_long_mins   = (int)cJSON_GetNumberValue(nval);
            if (!strcmp(key,"cycles"))         g_cfg.pomo_cycles        = (int)cJSON_GetNumberValue(nval);
            if (!strcmp(key,"sleep_timeout_min")) g_cfg.sleep_timeout_min  = (int)cJSON_GetNumberValue(nval);
            cfg_save();
        }
        if (cb_screen_force_render) cb_screen_force_render();

    } else if (strcmp(cmd, "refresh") == 0) {
        // Force a full EPD refresh on the current screen.
        if (cb_screen_force_render) {
            if (s_fb) s_fb->force_full_next = true;
            cb_screen_force_render();
        }

    } else if (strcmp(cmd, "btn") == 0) {
        // Simulate a hardware button press (web console HW buttons panel).
        // Body: {"cmd":"btn","id":1,"evt":"short"|"long"}
        cJSON *id_item  = cJSON_GetObjectItem(j, "id");
        const char *evt = cJSON_GetStringValue(cJSON_GetObjectItem(j, "evt"));
        if (id_item && evt && cb_btn) {
            int id_val  = (int)cJSON_GetNumberValue(id_item) - 1; // 1-based → 0-based
            int evt_val = (strcmp(evt, "long") == 0) ? 1 : 0;
            if (id_val >= 0 && id_val <= 2) cb_btn(id_val, evt_val);
        }

    } else if (strcmp(cmd, "enc") == 0) {
        // Simulate encoder rotation. Body: {"cmd":"enc","delta":1|-1}
        cJSON *delta_item = cJSON_GetObjectItem(j, "delta");
        if (delta_item && cb_enc) cb_enc((int)cJSON_GetNumberValue(delta_item));

    } else if (strcmp(cmd, "ota") == 0) {
        // Arm OTA upload mode. Host should then POST binary to /api/ota/upload.
        if (!ota_trigger(NULL)) {
            cJSON_Delete(j);
            return send_err(req, "ota busy");
        }

    } else if (strcmp(cmd, "restart") == 0) {
        cJSON_Delete(j);
        send_json(req, "{\"ok\":true,\"msg\":\"restarting\"}");
        vTaskDelay(pdMS_TO_TICKS(250));
        esp_restart();
        return ESP_OK;
    }

    cJSON_Delete(j);
    return send_ok(req);
}

// ── GET /api/bitmap — raw 1-bit fb ───────────────────────────
static esp_err_t handle_bitmap(httpd_req_t *req) {
    if (!s_fb) return send_err(req, "no fb");
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, (char *)s_fb->buf, FB_BYTES);
}

// ── GET /designer — serve the dedicated designer tool from SPIFFS ─────
static esp_err_t handle_designer(httpd_req_t *req) {
    FILE *f = fopen("/spiffs/www/webserver.html", "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "webserver.html not found — upload SPIFFS image");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    char *buf = (char *)malloc(4096);
    if (!buf) { fclose(f); httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM"); return ESP_OK; }
    size_t n;
    while ((n = fread(buf, 1, 4096, f)) > 0) httpd_resp_send_chunk(req, buf, n);
    httpd_resp_send_chunk(req, NULL, 0);
    free(buf); fclose(f);
    return ESP_OK;
}

// ── GET /screens/<name>.html — serve screen design HTML ───────
// Allows the designer to load screen HTMLs in an iframe (same-origin).
// URI: /screens/clock.html → SPIFFS: /spiffs/screens/clock.html
static esp_err_t handle_screen_file(httpd_req_t *req) {
    // req->uri = "/screens/clock.html"  (9-char prefix "/screens/")
    const char *filename = req->uri + 9;

    // Basic path sanitization — reject traversal attempts
    if (!filename[0] || strstr(filename, "..") || strstr(filename, "/")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid path");
        return ESP_OK;
    }

    char path[80];
    snprintf(path, sizeof(path), "/spiffs/screens/%s", filename);

    FILE *f = fopen(path, "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "screen not found");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    // Allow same-origin iframe embedding (default; no X-Frame-Options needed)

    char *buf = (char *)malloc(4096);
    if (!buf) { fclose(f); httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM"); return ESP_OK; }
    size_t n;
    while ((n = fread(buf, 1, 4096, f)) > 0) httpd_resp_send_chunk(req, buf, n);
    httpd_resp_send_chunk(req, NULL, 0);
    free(buf); fclose(f);
    ESP_LOGD(TAG, "Served screen: %s", filename);
    return ESP_OK;
}

// ── POST /api/push-bitmap — receive 1-bit fb from browser ─────
// Body: exactly FB_BYTES (15000) raw bytes matching fb_t::buf format.
//   Byte layout: row-major, 8 pixels per byte, MSB = leftmost pixel.
//   bit=1 → white pixel, bit=0 → black pixel.
//
// On success: bitmap is loaded into s_fb and cb_push_raw() is called
// (which triggers EPD full refresh on hardware).
// Browser can verify via GET /api/bitmap after the push.
static esp_err_t handle_push_bitmap(httpd_req_t *req) {
    if (!s_fb) return send_err(req, "no framebuffer");

    int expected = FB_BYTES;  // 15000
    if (req->content_len != (size_t)expected) {
        char err[48];
        snprintf(err, sizeof(err), "expected %d bytes, got %d", expected, (int)req->content_len);
        return send_err(req, err);
    }

    // Read directly into framebuffer — avoids a second 15KB malloc
    int received = httpd_req_recv(req, (char *)s_fb->buf, expected);
    if (received != expected) {
        ESP_LOGW(TAG, "push_bitmap: short read %d/%d", received, expected);
        return send_err(req, "incomplete read");
    }

    ESP_LOGI(TAG, "Browser push: %d bytes loaded into framebuffer", expected);

    // Flush to EPD (no-op when HARDWARE_ENABLED=0 — callback not registered)
    if (cb_push_raw) cb_push_raw();

    return send_ok(req);
}

// ── POST /api/ota/upload — receive firmware binary and flash ──
static esp_err_t handle_ota_upload(httpd_req_t *req) {
    if (!ota_is_armed()) return send_err(req, "ota not armed");
    if (req->content_len <= 0) return send_err(req, "bad body");

    if (!ota_push_begin((size_t)req->content_len)) {
        return send_err(req, "ota begin failed");
    }

    uint8_t *buf = (uint8_t *)malloc(4096);
    if (!buf) {
        ota_push_abort();
        return send_err(req, "oom");
    }

    int remaining = req->content_len;
    while (remaining > 0) {
        int chunk = (remaining > 4096) ? 4096 : remaining;
        int n = httpd_req_recv(req, (char *)buf, chunk);
        if (n <= 0) {
            free(buf);
            ota_push_abort();
            return send_err(req, "upload recv failed");
        }
        if (!ota_push_write(buf, (size_t)n)) {
            free(buf);
            return send_err(req, "ota write failed");
        }
        remaining -= n;
    }
    free(buf);

    if (!ota_push_end()) {
        return send_err(req, "ota finalize failed");
    }

    send_json(req, "{\"ok\":true,\"msg\":\"ota uploaded; rebooting\"}");
    vTaskDelay(pdMS_TO_TICKS(250));
    esp_restart();
    return ESP_OK;
}

// ── POST /api/spiffs/upload — receive SPIFFS image and write to spiffs partition ──
static esp_err_t handle_spiffs_upload(httpd_req_t *req) {
    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                           ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
                                                           "spiffs");
    if (!part) return send_err(req, "spiffs partition not found");
    if (req->content_len <= 0) return send_err(req, "bad body");

    if ((size_t)req->content_len > part->size) return send_err(req, "image too large");

    // If SPIFFS is mounted via VFS, unregister it first to avoid corruption
    esp_vfs_spiffs_unregister(NULL);

    // Erase entire partition before writing
    esp_err_t er = esp_partition_erase_range(part, 0, part->size);
    if (er != ESP_OK) {
        ESP_LOGE(TAG, "spiffs erase failed: %s", esp_err_to_name(er));
        return send_err(req, "erase failed");
    }

    uint8_t *buf = (uint8_t *)malloc(4096);
    if (!buf) return send_err(req, "oom");

    size_t offset = 0;
    int remaining = req->content_len;
    while (remaining > 0) {
        int chunk = (remaining > 4096) ? 4096 : remaining;
        int n = httpd_req_recv(req, (char *)buf, chunk);
        if (n <= 0) { free(buf); ESP_LOGE(TAG, "spiffs upload recv failed"); return send_err(req, "upload recv failed"); }
        esp_err_t wr = esp_partition_write(part, offset, buf, n);
        if (wr != ESP_OK) { free(buf); ESP_LOGE(TAG, "spiffs write failed: %s", esp_err_to_name(wr)); return send_err(req, "write failed"); }
        offset += n;
        remaining -= n;
    }
    free(buf);

    // Remount SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 8,
        .format_if_mount_failed = false,
    };
    esp_err_t r = esp_vfs_spiffs_register(&conf);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "spiffs remount failed: %s", esp_err_to_name(r));
        return send_err(req, "remount failed");
    }

    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "SPIFFS uploaded: %d bytes used of %d", (int)used, (int)total);

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"ok\":true,\"used\":%d,\"total\":%d}", (int)used, (int)total);
    send_json(req, resp);
    return ESP_OK;
}

// ── WS handler ───────────────────────────────────────────────
static esp_err_t handle_ws(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ws_add(httpd_req_to_sockfd(req));
        ESP_LOGI(TAG, "WS client connected (%d total)", s_ws_n);
        web_server_push_state();
        return ESP_OK;
    }
    // Receive frame (ignore content for now — console is standalone)
    httpd_ws_frame_t f = {};
    f.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t r = httpd_ws_recv_frame(req, &f, 0);
    if (r == ESP_OK && f.len > 0) {
        f.payload = (uint8_t *)malloc(f.len + 1);
        httpd_ws_recv_frame(req, &f, f.len);
        free(f.payload);
    }
    return ESP_OK;
}

// ── GET /api/alarms ───────────────────────────────────────────
static esp_err_t handle_alarms_get(httpd_req_t *req) {
    char *json = alarm_get_json();
    esp_err_t ret = send_json(req, json ? json : "{\"alarms\":[]}");
    free(json);
    return ret;
}

// ── POST /api/alarms — replace alarm list from JSON ───────────
// Body: {"alarms":[{"hour":7,"min":0,"label":"WAKE","on":true,"days":31},...]}
static esp_err_t handle_alarms_post(httpd_req_t *req) {
    int len = req->content_len;
    if (len <= 0 || len > 2048) return send_err(req, "bad body");
    char *body = (char *)malloc(len + 1);
    if (!body) return send_err(req, "oom");
    httpd_req_recv(req, body, len);
    body[len] = '\0';
    bool ok = alarm_set_json(body);
    free(body);
    if (!ok) return send_err(req, "parse error");
    if (cb_screen_force_render) cb_screen_force_render();
    return send_ok(req);
}

// ── GET /api/tasks ────────────────────────────────────────────
static esp_err_t handle_tasks_get(httpd_req_t *req) {
    char *json = tasks_get_json();
    esp_err_t ret = send_json(req, json ? json : "{\"tasks\":[]}");
    free(json);
    return ret;
}

// ── POST /api/tasks — replace task list from JSON ─────────────
// Body: {"tasks":[{"text":"Review PR","tag":"WORK","done":false},...]}
static esp_err_t handle_tasks_post(httpd_req_t *req) {
    int len = req->content_len;
    if (len <= 0 || len > 4096) return send_err(req, "bad body");
    char *body = (char *)malloc(len + 1);
    if (!body) return send_err(req, "oom");
    httpd_req_recv(req, body, len);
    body[len] = '\0';
    bool ok = tasks_set_json(body);
    free(body);
    if (!ok) return send_err(req, "parse error");
    if (cb_screen_force_render) cb_screen_force_render();
    return send_ok(req);
}

// ── Route table ──────────────────────────────────────────────
void web_server_start(fb_t *fb) {
    s_fb = fb;
    s_ws_n = 0;

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 18;   // includes /api/ota/upload
    cfg.uri_match_fn     = httpd_uri_match_wildcard;
    cfg.stack_size       = 8192;

    ESP_ERROR_CHECK(httpd_start(&s_server, &cfg));

#define ROUTE(m,u,h,ws) do { \
    httpd_uri_t _r = {}; \
    _r.uri = (u); _r.method = (m); _r.handler = (h); _r.is_websocket = (ws); \
    httpd_register_uri_handler(s_server,&_r); } while(0)

    // Specific routes must be registered BEFORE wildcards (first-match wins)
    ROUTE(HTTP_GET,  "/ws",              handle_ws,          true);
    ROUTE(HTTP_GET,  "/",                handle_root,        false);
    ROUTE(HTTP_GET,  "/api/state",       handle_state,       false);
    ROUTE(HTTP_POST, "/api/cmd",         handle_cmd,         false);
    ROUTE(HTTP_GET,  "/api/bitmap",      handle_bitmap,      false);
    ROUTE(HTTP_POST, "/api/push-bitmap", handle_push_bitmap, false);
    ROUTE(HTTP_POST, "/api/ota/upload",  handle_ota_upload,   false);
    ROUTE(HTTP_POST, "/api/spiffs/upload", handle_spiffs_upload, false);
    ROUTE(HTTP_GET,  "/designer",        handle_designer,     false);
    ROUTE(HTTP_GET,  "/api/alarms",      handle_alarms_get,   false);
    ROUTE(HTTP_POST, "/api/alarms",      handle_alarms_post,  false);
    ROUTE(HTTP_GET,  "/api/tasks",       handle_tasks_get,    false);
    ROUTE(HTTP_POST, "/api/tasks",       handle_tasks_post,   false);
    ROUTE(HTTP_GET,  "/screens/?*",      handle_screen_file,  false);  // wildcard last
#undef ROUTE

    ESP_LOGI(TAG, "HTTP started — serving index on :80 | /designer for designer tool");
}

void web_server_stop(void) {
    if (s_server) { httpd_stop(s_server); s_server = NULL; }
}

void web_server_push_state(void) {
    if (!s_server || s_ws_n == 0) return;
    char *json = build_state_json();
    if (!json) return;
    httpd_ws_frame_t f = {};
    f.type = HTTPD_WS_TYPE_TEXT;
    f.payload = (uint8_t *)json;
    f.len = strlen(json);
    for (int i = s_ws_n - 1; i >= 0; i--) {
        esp_err_t ret = httpd_ws_send_frame_async(s_server, s_ws_fds[i], &f);
        if (ret != ESP_OK) { ESP_LOGW(TAG, "WS send fail fd=%d", s_ws_fds[i]); ws_remove(s_ws_fds[i]); }
    }
    free(json);
}

void web_server_poll(void) {}  // placeholder for future queued commands

void web_server_bitmap_updated(void) {
    s_bitmap_rev++;
    web_server_push_state();   // push new bitmap_rev to all WS clients immediately
}
