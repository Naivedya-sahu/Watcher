// firmware/main/watcher_buttons.h
#pragma once
#include "esp_err.h"

typedef void (*watcher_btn_cb_t)(int btn /* 1..3 */, int long_press /* 0/1 */);
esp_err_t watcher_buttons_start(watcher_btn_cb_t cb);
