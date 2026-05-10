// firmware/main/watcher_epd.h
#pragma once
#include "esp_err.h"
#include <stdint.h>

// 400×300, 1-bit, SSD1683 controller
#define EPD_W 400
#define EPD_H 300

esp_err_t watcher_epd_init(void);
void      watcher_epd_clear(int white);
void      watcher_epd_partial_refresh(void);
void      watcher_epd_full_refresh(void);

// Active screen the device is rendering. Pushed to the UI over /ws.
typedef enum { EPD_CLOCK, EPD_ALARM, EPD_POMO, EPD_CAL, EPD_TASKS, EPD_SETTINGS } epd_screen_t;
void watcher_epd_set_screen(epd_screen_t s);
epd_screen_t watcher_epd_get_screen(void);
