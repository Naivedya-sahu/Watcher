#include "buzzer.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "buzzer";
static int s_pin = -1;

void buzzer_init(int gpio_pin) {
    s_pin = gpio_pin;
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_8_BIT;
    timer.timer_num = LEDC_TIMER_0;
    timer.freq_hz = 2000;
    timer.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {};
    ch.gpio_num = gpio_pin;
    ch.speed_mode = LEDC_LOW_SPEED_MODE;
    ch.channel = LEDC_CHANNEL_0;
    ch.timer_sel = LEDC_TIMER_0;
    ch.duty = 0;
    ch.hpoint = 0;
    ledc_channel_config(&ch);
    ESP_LOGI(TAG, "Buzzer init gpio %d", gpio_pin);
}

void buzzer_beep(uint32_t freq_hz, uint32_t ms) {
    if (s_pin < 0 || freq_hz == 0) return;
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128);  // 50% duty
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void buzzer_tone(buzzer_pattern_t pattern) {
    switch (pattern) {
        case BUZZ_BOOT:
            buzzer_beep(800, 80); buzzer_beep(1200, 80); break;
        case BUZZ_TICK:
            buzzer_beep(2000, 1); break;
        case BUZZ_SUCCESS:
        case BUZZ_POMO_START:
            buzzer_beep(1000, 100); buzzer_beep(1500, 150); break;
        case BUZZ_ERROR:
            buzzer_beep(300, 400); break;
        case BUZZ_ALERT:
            for (int i = 0; i < 3; i++) {
                buzzer_beep(1800, 80);
                vTaskDelay(pdMS_TO_TICKS(80));
            }
            break;
        case BUZZ_POMO_DONE:
            buzzer_beep(1000, 200);
            buzzer_beep(1200, 200);
            buzzer_beep(1500, 300);
            break;
    }
}
