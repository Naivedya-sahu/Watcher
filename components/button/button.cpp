#include "button.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "button";

static int         s_n;
static int         s_pins[BTN_MAX_N];
static uint32_t    s_long_ms;
static button_cb_t s_cb;

static volatile uint32_t s_press_time[BTN_MAX_N];
static volatile bool     s_pressed[BTN_MAX_N];
static volatile bool     s_handled[BTN_MAX_N];

static void IRAM_ATTR btn_isr(void *arg) {
    int idx = (int)(intptr_t)arg;
    if (!s_pressed[idx]) {
        s_press_time[idx] = (uint32_t)(esp_timer_get_time() / 1000);
        s_pressed[idx] = true;
        s_handled[idx] = false;
    }
}

void button_init(const int *pins, int n, uint32_t long_ms, button_cb_t cb) {
    s_n       = (n > BTN_MAX_N) ? BTN_MAX_N : n;
    s_long_ms = long_ms;
    s_cb      = cb;

    for (int i = 0; i < s_n; i++) {
        s_pins[i]      = pins[i];
        s_press_time[i] = 0;
        s_pressed[i]   = false;
        s_handled[i]   = false;
    }

    // Install ISR service — may already be installed by encoder; ignore error
    gpio_install_isr_service(0);

    for (int i = 0; i < s_n; i++) {
        gpio_config_t cfg = {};
        cfg.pin_bit_mask = (1ULL << pins[i]);
        cfg.mode = GPIO_MODE_INPUT;
        cfg.pull_up_en = GPIO_PULLUP_DISABLE;   // ext pull-up on peripheral board
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type = GPIO_INTR_NEGEDGE;      // falling edge = press
        gpio_config(&cfg);
        gpio_isr_handler_add((gpio_num_t)pins[i], btn_isr, (void *)(intptr_t)i);
        ESP_LOGI(TAG, "btn[%d] on IO%d", i, pins[i]);
    }
}

void button_tick(void) {
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    for (int i = 0; i < s_n; i++) {
        if (!s_pressed[i] || s_handled[i]) continue;
        int level = gpio_get_level((gpio_num_t)s_pins[i]);
        uint32_t held = now_ms - s_press_time[i];
        if (level == 1) {
            // Released
            s_handled[i] = true;
            s_pressed[i] = false;
            if (held < 50) continue;  // debounce
            button_evt_t evt = (held >= s_long_ms) ? BTN_EVT_LONG : BTN_EVT_SHORT;
            if (s_cb) s_cb((button_id_t)i, evt);
        }
    }
}
