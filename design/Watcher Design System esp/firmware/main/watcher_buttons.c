// firmware/main/watcher_buttons.c
#include "watcher_buttons.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define PIN_BTN1 39
#define PIN_BTN2 40
#define PIN_BTN3 41
#define LONG_PRESS_US 700000   // 700 ms

static const int PINS[3] = { PIN_BTN1, PIN_BTN2, PIN_BTN3 };
static watcher_btn_cb_t s_cb;

static void btn_task(void *_) {
    int prev[3] = {1,1,1};
    int64_t down_at[3] = {0,0,0};
    while (1) {
        for (int i = 0; i < 3; ++i) {
            int v = gpio_get_level(PINS[i]);
            if (prev[i] == 1 && v == 0)        down_at[i] = esp_timer_get_time();
            else if (prev[i] == 0 && v == 1) {
                int64_t held = esp_timer_get_time() - down_at[i];
                if (s_cb) s_cb(i + 1, held > LONG_PRESS_US);
            }
            prev[i] = v;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t watcher_buttons_start(watcher_btn_cb_t cb) {
    s_cb = cb;
    for (int i = 0; i < 3; ++i) {
        gpio_config_t c = { .pin_bit_mask = 1ULL << PINS[i],
                            .mode = GPIO_MODE_INPUT,
                            .pull_up_en = GPIO_PULLUP_ENABLE };
        gpio_config(&c);
    }
    xTaskCreate(btn_task, "btn", 2048, NULL, 5, NULL);
    return ESP_OK;
}
