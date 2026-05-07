// test_periph.c — Watcher v7.1 bring-up test
// Tests: EC11 encoder (A/B quadrature + SW click), passive buzzer (LEDC PWM),
//        single GPIO button (active-low).
// Watcher pinout reference (ESP32 GPIO numbers):
// | EPD MOSI | 11 |
// | EPD CLK | 12 |
// | EPD CS | 10 |
// | EPD DC | 15 |
// | EPD RST | 16 |
// | EPD BUSY | 17 |
// | BUZZER | 41 |
// | BTN A (back) | 40 |
// | BTN B (next) | 39 |
// | BTN C (pomo) | 38 |
// | ENC B | 37 |
// | ENC A | 36 |
// | ENC SW| 35 |
//============================================================

// If running WITHOUT the peripheral board: wire your own 10K pull-ups to 3.3V.
//
// Build & flash:
//   cd v7.1/test_periph
//   idf.py set-target esp32s3
//   idf.py -p COM<N> flash monitor
//
// Expected serial output (115200 baud):
//   [PERIPH TEST] Watcher v7.1 — encoder + buzzer + button
//   [BTN ] press #1  — buzzer: 440 Hz
//   [ENC ] CW  +1  total: +1
//   [ENC ] CCW -1  total:  0
//   [SW  ] encoder click #1

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <stdint.h>
#include <stdio.h>

static const char *TAG = "periph";

// ── Pin assignments ───────────────────────────────────────────
#define PIN_BUZZER 41 
#define PIN_BTN_A  40 
#define PIN_BTN_B  39 
#define PIN_BTN_C  38 
#define PIN_ENC_B  37 
#define PIN_ENC_A  36 
#define PIN_ENC_SW 35 

// ── LEDC (buzzer) ─────────────────────────────────────────────
// ESP32-S3 has only low-speed LEDC channels.
#define BUZZ_TIMER   LEDC_TIMER_0
#define BUZZ_CHANNEL LEDC_CHANNEL_0
#define BUZZ_DUTY_RES LEDC_TIMER_10_BIT   // 1024 steps
#define BUZZ_DUTY_50  512                  // 50% duty

// Tone frequencies (Hz) cycled on each button press
static const uint32_t TONES[] = { 440, 880, 1047, 1319, 0 };
// 0 = silence (buzzer off)

static volatile int s_tone_idx = 0;   // cycles through TONES[]

static void buzzer_play(uint32_t freq_hz) {
    if (freq_hz == 0) {
        ledc_stop(LEDC_LOW_SPEED_MODE, BUZZ_CHANNEL, 0);
        return;
    }
    ledc_set_freq(LEDC_LOW_SPEED_MODE, BUZZ_TIMER, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZ_CHANNEL, BUZZ_DUTY_50);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZ_CHANNEL);
}

static void buzzer_stop(void) {
    ledc_stop(LEDC_LOW_SPEED_MODE, BUZZ_CHANNEL, 0);
}

// ── Shared counters (written in ISR, read in main) ────────────
static volatile int32_t s_enc_total  = 0;   // net encoder position
static volatile int      s_btn_count  = 0;   // button press count
static volatile int      s_sw_count   = 0;   // encoder click count

// ISR flags — set in ISR, cleared in main loop
static volatile bool s_enc_event = false;
static volatile int  s_enc_dir   = 0;   // +1 CW, -1 CCW
static volatile bool s_btn_event = false;
static volatile bool s_sw_event  = false;

// ── Encoder ISR — quadrature decode on A edge ─────────────────
// Simple X1 decode: ISR fires on any edge of A; sample B to get direction.
//   A rising  + B low  → CW   (+1)
//   A rising  + B high → CCW  (-1)
//   A falling + B high → CW   (+1)
//   A falling + B low  → CCW  (-1)
static void IRAM_ATTR enc_isr(void *arg) {
    int a = gpio_get_level(PIN_ENC_A);
    int b = gpio_get_level(PIN_ENC_B);
    int dir = (a == b) ? -1 : +1;   // XOR determines direction
    s_enc_total += dir;
    s_enc_dir   = dir;
    s_enc_event = true;
}

// ── Button ISR (active-low, falling edge) ─────────────────────
static void IRAM_ATTR btn_isr(void *arg) {
    s_btn_count++;
    s_btn_event = true;
}

// ── Encoder SW ISR (active-low, falling edge) ─────────────────
static void IRAM_ATTR sw_isr(void *arg) {
    s_sw_count++;
    s_sw_event = true;
}

// ── Hardware init ─────────────────────────────────────────────
static void periph_init(void) {

    // LEDC timer
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = BUZZ_DUTY_RES,
        .timer_num       = BUZZ_TIMER,
        .freq_hz         = 440,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    // LEDC channel (buzzer pin, initially off)
    ledc_channel_config_t ch = {
        .gpio_num   = PIN_BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BUZZ_CHANNEL,
        .timer_sel  = BUZZ_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ch);

    // Encoder A — input, no internal pull (external 4.7K on peripheral board)
    gpio_config_t enc_a = {
        .pin_bit_mask = (1ULL << PIN_ENC_A),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,   // fallback if no peripheral board
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&enc_a);

    // Encoder B — input only, no ISR (sampled inside A's ISR)
    gpio_config_t enc_b = {
        .pin_bit_mask = (1ULL << PIN_ENC_B),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&enc_b);

    // Encoder SW — falling edge
    gpio_config_t enc_sw = {
        .pin_bit_mask = (1ULL << PIN_ENC_SW),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&enc_sw);

    // Button — falling edge (active-low)
    gpio_config_t btn = {
        .pin_bit_mask = (1ULL << PIN_BTN_A),  // using BTN A (IO40) for this test
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn);

    // Install GPIO ISR service (shared) and attach handlers
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_ENC_A,  enc_isr, NULL);
    gpio_isr_handler_add(PIN_ENC_SW, sw_isr,  NULL);
    gpio_isr_handler_add(PIN_BTN_A,  btn_isr, NULL);
}

// ── Main ──────────────────────────────────────────────────────
void app_main(void) {
    periph_init();

    ESP_LOGI(TAG, "=== Watcher v7.1 — encoder + buzzer + button test ===");
    ESP_LOGI(TAG, "Rotate encoder → direction + total printed");
    ESP_LOGI(TAG, "Press encoder SW → click count printed");
    ESP_LOGI(TAG, "Press button (IO40) → buzzer plays tone, cycles A4/A5/C6/E6/OFF");
    ESP_LOGI(TAG, "======================================================");

    // Play a short boot chirp so you know the buzzer works at start
    buzzer_play(880);
    vTaskDelay(pdMS_TO_TICKS(120));
    buzzer_stop();

    while (true) {

        // ── Encoder rotation ──────────────────────────────────
        if (s_enc_event) {
            s_enc_event = false;
            int dir   = s_enc_dir;
            int total = (int)s_enc_total;
            if (dir > 0)
                printf("[ENC ] CW  +1  total: %+d\n", total);
            else
                printf("[ENC ] CCW -1  total: %+d\n", total);
        }

        // ── Encoder click ─────────────────────────────────────
        if (s_sw_event) {
            s_sw_event = false;
            printf("[SW  ] encoder click #%d\n", s_sw_count);
            // Short double-chirp on click
            buzzer_play(1319);
            vTaskDelay(pdMS_TO_TICKS(60));
            buzzer_stop();
            vTaskDelay(pdMS_TO_TICKS(40));
            buzzer_play(1319);
            vTaskDelay(pdMS_TO_TICKS(60));
            buzzer_stop();
        }

        // ── Button press ──────────────────────────────────────
        if (s_btn_event) {
            s_btn_event = false;

            uint32_t freq = TONES[s_tone_idx];
            s_tone_idx = (s_tone_idx + 1) % (sizeof(TONES) / sizeof(TONES[0]));

            if (freq == 0)
                printf("[BTN ] press #%d  — buzzer: OFF\n", s_btn_count);
            else
                printf("[BTN ] press #%d  — buzzer: %lu Hz\n",
                       s_btn_count, (unsigned long)freq);

            buzzer_play(freq);
            // Tone plays until next button press or encoder click stops it.
            // This lets you hold the button and hear the tone clearly.
            // (For auto-stop add: vTaskDelay(200); buzzer_stop(); here)
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // 100 Hz poll — fast enough for serial output
    }
}
