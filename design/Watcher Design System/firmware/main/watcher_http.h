// firmware/main/watcher_http.h
#pragma once
#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t watcher_http_start(httpd_handle_t *out_server);
void      watcher_http_push_state(const char *json);   // broadcast over /ws
void      watcher_http_push_log(const char *tag, const char *msg);
