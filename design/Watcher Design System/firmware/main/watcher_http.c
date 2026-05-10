// firmware/main/watcher_http.c
//
// HTTP + WS server. Mirrors the JS contract documented in ../../watcher-api.js.
//
// `GET /` rewrites the bootstrap <script> tag in the embedded index.html with
// the live NVS blob so the React UI hydrates without a second round-trip.

#include "watcher_http.h"
#include "watcher_nvs.h"
#include "watcher_epd.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "watcher_http";

// embedded web console — see EMBED_FILES in main/CMakeLists.txt
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

static httpd_handle_t s_server = NULL;
static int s_ws_fd = -1;          // single-client; extend to fd-set for multi-client

// ───── helpers ─────────────────────────────────────────────────────────────

static esp_err_t send_json(httpd_req_t *r, cJSON *j) {
    char *out = cJSON_PrintUnformatted(j);
    httpd_resp_set_type(r, "application/json");
    esp_err_t e = httpd_resp_sendstr(r, out ? out : "{}");
    free(out);
    cJSON_Delete(j);
    return e;
}

static cJSON *read_body_json(httpd_req_t *r) {
    int total = r->content_len;
    if (total <= 0 || total > 32 * 1024) return NULL;
    char *buf = malloc(total + 1);
    if (!buf) return NULL;
    int got = 0;
    while (got < total) {
        int n = httpd_req_recv(r, buf + got, total - got);
        if (n <= 0) { free(buf); return NULL; }
        got += n;
    }
    buf[total] = '\0';
    cJSON *j = cJSON_Parse(buf);
    free(buf);
    return j;
}

static esp_err_t put_blob_from_body(httpd_req_t *r, const char *key) {
    int total = r->content_len;
    if (total <= 0 || total > 32 * 1024) return httpd_resp_send_500(r);
    char *buf = malloc(total + 1);
    if (!buf) return httpd_resp_send_500(r);
    int got = 0;
    while (got < total) {
        int n = httpd_req_recv(r, buf + got, total - got);
        if (n <= 0) { free(buf); return httpd_resp_send_500(r); }
        got += n;
    }
    buf[total] = '\0';
    esp_err_t e = watcher_nvs_set_blob(key, buf, total);
    free(buf);
    if (e != ESP_OK) return httpd_resp_send_500(r);
    httpd_resp_set_type(r, "application/json");
    return httpd_resp_sendstr(r, "{\"ok\":true}");
}

static esp_err_t serve_blob(httpd_req_t *r, const char *key, const char *fallback) {
    char *buf = NULL; size_t len = 0;
    watcher_nvs_get_blob(key, &buf, &len);
    httpd_resp_set_type(r, "application/json");
    esp_err_t e = httpd_resp_sendstr(r, buf ? buf : fallback);
    free(buf);
    return e;
}

// ───── GET / — rewrite bootstrap tag with NVS state ────────────────────────

static esp_err_t root_get(httpd_req_t *r) {
    const char *html = (const char *)index_html_start;
    size_t      html_len = index_html_end - index_html_start;

    // assemble bootstrap JSON
    char *cfg = NULL, *alarms = NULL, *events = NULL, *tasks = NULL;
    size_t cfg_n=0, al_n=0, ev_n=0, tk_n=0;
    watcher_nvs_get_blob("cfg",    &cfg,    &cfg_n);
    watcher_nvs_get_blob("alarms", &alarms, &al_n);
    watcher_nvs_get_blob("events", &events, &ev_n);
    watcher_nvs_get_blob("tasks",  &tasks,  &tk_n);

    char *boot = NULL;
    asprintf(&boot,
        "{\"cfg\":%s,\"alarms\":%s,\"events\":%s,\"tasks\":%s,\"fwVer\":\"7.1.0\"}",
        cfg    ? cfg    : "{}",
        alarms ? alarms : "[]",
        events ? events : "[]",
        tasks  ? tasks  : "[]");
    free(cfg); free(alarms); free(events); free(tasks);

    // find <script id="watcher-bootstrap" ...> and replace its contents
    const char *open_tag  = "<script id=\"watcher-bootstrap\"";
    const char *open_end  = ">";
    const char *close_tag = "</script>";

    const char *p = strstr(html, open_tag);
    if (!p) { httpd_resp_set_type(r, "text/html"); free(boot);
              return httpd_resp_send(r, html, html_len); }
    const char *body_start = strstr(p, open_end);
    const char *body_end   = body_start ? strstr(body_start, close_tag) : NULL;
    if (!body_start || !body_end) { httpd_resp_set_type(r, "text/html"); free(boot);
              return httpd_resp_send(r, html, html_len); }

    httpd_resp_set_type(r, "text/html");
    httpd_resp_send_chunk(r, html, body_start + 1 - html);  // up to and including '>'
    httpd_resp_send_chunk(r, boot, strlen(boot));
    httpd_resp_send_chunk(r, body_end, html + html_len - body_end);
    httpd_resp_send_chunk(r, NULL, 0);
    free(boot);
    return ESP_OK;
}

// ───── REST: cfg ───────────────────────────────────────────────────────────

static esp_err_t cfg_get(httpd_req_t *r)   { return serve_blob(r, "cfg", "{}"); }

static esp_err_t cfg_patch(httpd_req_t *r) {
    cJSON *patch = read_body_json(r);
    if (!patch) return httpd_resp_send_500(r);
    char *cur = NULL; size_t n = 0;
    watcher_nvs_get_blob("cfg", &cur, &n);
    cJSON *root = cur ? cJSON_Parse(cur) : cJSON_CreateObject();
    if (!root) root = cJSON_CreateObject();
    cJSON *it = NULL;
    cJSON_ArrayForEach(it, patch) {
        cJSON_DeleteItemFromObject(root, it->string);
        cJSON_AddItemToObject(root, it->string, cJSON_Duplicate(it, 1));
    }
    char *out = cJSON_PrintUnformatted(root);
    watcher_nvs_set_blob("cfg", out, strlen(out));
    free(cur); free(out);
    cJSON_Delete(patch);
    return send_json(r, root);
}

// ───── REST: collections ───────────────────────────────────────────────────

static esp_err_t alarms_get(httpd_req_t *r) { return serve_blob(r, "alarms", "[]"); }
static esp_err_t alarms_put(httpd_req_t *r) { return put_blob_from_body(r, "alarms"); }
static esp_err_t events_get(httpd_req_t *r) { return serve_blob(r, "events", "[]"); }
static esp_err_t events_put(httpd_req_t *r) { return put_blob_from_body(r, "events"); }
static esp_err_t tasks_get (httpd_req_t *r) { return serve_blob(r, "tasks",  "[]"); }
static esp_err_t tasks_put (httpd_req_t *r) { return put_blob_from_body(r, "tasks"); }

// ───── REST: telemetry / actions ───────────────────────────────────────────

static esp_err_t device_get(httpd_req_t *r) {
    cJSON *j = cJSON_CreateObject();
    cJSON_AddNumberToObject(j, "uptime_s", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(j, "heap_free", esp_get_free_heap_size());
    cJSON_AddNumberToObject(j, "psram",     heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    cJSON_AddStringToObject(j, "fw",        "7.1.0");
    cJSON_AddStringToObject(j, "epd_ctrl",  "SSD1683");
    return send_json(r, j);
}

static esp_err_t refresh_post(httpd_req_t *r) {
    watcher_epd_full_refresh();
    httpd_resp_set_type(r, "application/json");
    return httpd_resp_sendstr(r, "{\"ok\":true}");
}

static esp_err_t reboot_post(httpd_req_t *r) {
    httpd_resp_set_type(r, "application/json");
    httpd_resp_sendstr(r, "{\"ok\":true,\"rebooting\":true}");
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
    return ESP_OK;
}

// ───── WS ──────────────────────────────────────────────────────────────────

static esp_err_t ws_handler(httpd_req_t *r) {
    if (r->method == HTTP_GET) { s_ws_fd = httpd_req_to_sockfd(r); return ESP_OK; }
    httpd_ws_frame_t pkt = {0};
    pkt.type = HTTPD_WS_TYPE_TEXT;
    httpd_ws_recv_frame(r, &pkt, 0);
    if (pkt.len) {
        uint8_t *buf = calloc(1, pkt.len + 1);
        pkt.payload = buf;
        httpd_ws_recv_frame(r, &pkt, pkt.len);
        free(buf);
    }
    return ESP_OK;
}

void watcher_http_push_state(const char *json) {
    if (s_ws_fd < 0 || !s_server) return;
    httpd_ws_frame_t f = { .type = HTTPD_WS_TYPE_TEXT,
                           .payload = (uint8_t *)json, .len = strlen(json) };
    httpd_ws_send_frame_async(s_server, s_ws_fd, &f);
}

void watcher_http_push_log(const char *tag, const char *msg) {
    char *out = NULL;
    asprintf(&out, "{\"type\":\"log\",\"tag\":\"%s\",\"msg\":\"%s\"}", tag, msg);
    if (out) { watcher_http_push_state(out); free(out); }
}

// ───── start ───────────────────────────────────────────────────────────────

static const httpd_uri_t URIS[] = {
    { "/",            HTTP_GET,   root_get,     NULL },
    { "/api/cfg",     HTTP_GET,   cfg_get,      NULL },
    { "/api/cfg",     HTTP_PATCH, cfg_patch,    NULL },
    { "/api/alarms",  HTTP_GET,   alarms_get,   NULL },
    { "/api/alarms",  HTTP_PUT,   alarms_put,   NULL },
    { "/api/events",  HTTP_GET,   events_get,   NULL },
    { "/api/events",  HTTP_PUT,   events_put,   NULL },
    { "/api/tasks",   HTTP_GET,   tasks_get,    NULL },
    { "/api/tasks",   HTTP_PUT,   tasks_put,    NULL },
    { "/api/device",  HTTP_GET,   device_get,   NULL },
    { "/api/refresh", HTTP_POST,  refresh_post, NULL },
    { "/api/reboot",  HTTP_POST,  reboot_post,  NULL },
};

esp_err_t watcher_http_start(httpd_handle_t *out_server) {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 24;
    cfg.stack_size       = 8192;

    esp_err_t e = httpd_start(&s_server, &cfg);
    if (e != ESP_OK) return e;

    for (size_t i = 0; i < sizeof(URIS)/sizeof(URIS[0]); ++i)
        httpd_register_uri_handler(s_server, &URIS[i]);

    httpd_uri_t ws = { .uri = "/ws", .method = HTTP_GET, .handler = ws_handler,
                       .is_websocket = true };
    httpd_register_uri_handler(s_server, &ws);

    if (out_server) *out_server = s_server;
    ESP_LOGI(TAG, "http listening :80");
    return ESP_OK;
}
