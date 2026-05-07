// ============================================================
// Peripheral Verification Test — Watcher v7.1
// ============================================================
// PURPOSE: Gate test. All 5 steps must pass before running
//          main firmware. Tests every peripheral on the board
//          (buttons, encoder, buzzer). EPD tested separately
//          via test_epd/.
//
// BUILD:
//   cd v7.1/test_peripheral
//   idf.py build
//
// FLASH (UART via FTDI):
//   idf.py -p COM<N> flash monitor
//
// PASS CRITERIA (observe on serial monitor):
//   Step 1: Buzzer — 3 tones heard (low, mid, high), then melody
//   Step 2: Button A (IO39) — press 3× → logged each time
//   Step 3: Button B (IO40) — press 3× → logged each time
//   Step 4: Button C (IO41) — press 3× → logged each time
//   Step 5: Encoder — rotate CW/CCW + click → all logged
//
// Wiring (matches board_config.h):
//   Buttons:  IO39=A, IO40=B, IO41=C  (active-low, ext 10K pull-up)
//   Encoder:  IO35=A, IO36=B, IO37=SW (active-low, ext 10K pull-up)
//   Buzzer:   IO38  (passive, LEDC PWM ch0)
// ============================================================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "button.h"
#include "buzzer.h"
#include "encoder.h"

static const char *TAG = "test_periph";

// ── Pin config — matches board_config.h ──────────────────────
#define PIN_BTN_A   39
#define PIN_BTN_B   40
#define PIN_BTN_C   41
#define PIN_ENC_A   35
#define PIN_ENC_B   36
#define PIN_ENC_SW  37
#define PIN_BUZZER  38

// ── Event queue ───────────────────────────────────────────────
typedef struct {
    uint8_t  source;   // 0=btn, 1=enc_rot, 2=enc_click
    int      id;       // button_id_t or encoder delta
    uint8_t  evt;      // button_evt_t (for source=0)
} periph_evt_t;

static QueueHandle_t s_queue;

static void on_button(button_id_t id, button_evt_t evt) {
    // BTN_ID_ENC (index 3) = encoder click — tagged source=2 for routing
    uint8_t src = (id == BTN_ID_ENC) ? 2 : 0;
    periph_evt_t e = { src, (int)id, (uint8_t)evt };
    xQueueSendFromISR(s_queue, &e, nullptr);
}

static void on_encoder(int delta) {
    periph_evt_t e = { 1, delta, 0 };
    xQueueSend(s_queue, &e, 0);
}

// Encoder SW click arrives via button driver as BTN_ID_ENC (index 3)

// ── Helpers ───────────────────────────────────────────────────
static void wait(int ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static void log_step(int n, const char *name) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "══ Step %d: %s ══", n, name);
}

// Wait for N button presses on specific button id. Timeout 30s.
static bool wait_presses(button_id_t target, int count) {
    int got = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while (got < count) {
        periph_evt_t e;
        TickType_t now = xTaskGetTickCount();
        if (now >= deadline) {
            ESP_LOGE(TAG, "TIMEOUT waiting for button %d", (int)target);
            return false;
        }
        if (xQueueReceive(s_queue, &e, deadline - now) == pdTRUE) {
            if (e.source == 0 && e.id == (int)target && e.evt == BTN_EVT_SHORT) {
                got++;
                ESP_LOGI(TAG, "  Button %d press %d/%d ✓", (int)target, got, count);
            }
        }
    }
    return true;
}

// Wait for N encoder rotate steps (source=1). Timeout 30s.
static bool wait_enc_rotate(int count) {
    int got = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while (got < count) {
        periph_evt_t e;
        TickType_t now = xTaskGetTickCount();
        if (now >= deadline) { ESP_LOGE(TAG, "TIMEOUT enc rotate"); return false; }
        if (xQueueReceive(s_queue, &e, deadline - now) == pdTRUE) {
            if (e.source == 1) {
                got++;
                ESP_LOGI(TAG, "  Enc rotate delta=%+d (%d/%d) ✓", e.id, got, count);
            }
        }
    }
    return true;
}

// Wait for N encoder clicks (source=2). Timeout 30s.
static bool wait_enc_clicks(int count) {
    int got = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while (got < count) {
        periph_evt_t e;
        TickType_t now = xTaskGetTickCount();
        if (now >= deadline) { ESP_LOGE(TAG, "TIMEOUT enc click"); return false; }
        if (xQueueReceive(s_queue, &e, deadline - now) == pdTRUE) {
            if (e.source == 2 && e.evt == (uint8_t)BTN_EVT_SHORT) {
                got++;
                ESP_LOGI(TAG, "  Enc click (%d/%d) ✓", got, count);
            }
        }
    }
    return true;
}

// ── Test task ─────────────────────────────────────────────────
static void test_task(void *) {
    ESP_LOGI(TAG, "=== Watcher v7.1 Peripheral Test ===");
    ESP_LOGI(TAG, "Buttons: IO39/40/41  Encoder: IO35/36/SW=IO37  Buzzer: IO38");

    // Init queue
    s_queue = xQueueCreate(32, sizeof(periph_evt_t));

    // Init peripherals
    // Buttons: 4 total (A/B/C + encoder SW via button driver)
    const int btn_pins[4] = { PIN_BTN_A, PIN_BTN_B, PIN_BTN_C, PIN_ENC_SW };
    button_init(btn_pins, 4, 600, on_button);

    buzzer_init(PIN_BUZZER);
    encoder_init(PIN_ENC_A, PIN_ENC_B, on_encoder);   // SW click = BTN_ID_ENC via button driver

    bool all_pass = true;

    // ── Step 1: Buzzer ────────────────────────────────────────
    log_step(1, "Buzzer tones — listen for 3 pitches then melody");
    ESP_LOGI(TAG, "Playing: 220 Hz (A3, low)...");
    buzzer_beep(220, 500);
    wait(700);
    ESP_LOGI(TAG, "Playing: 440 Hz (A4, mid)...");
    buzzer_beep(440, 500);
    wait(700);
    ESP_LOGI(TAG, "Playing: 880 Hz (A5, high)...");
    buzzer_beep(880, 500);
    wait(700);
    // Short melody: C-E-G
    ESP_LOGI(TAG, "Playing: melody C4-E4-G4...");
    buzzer_beep(262, 200); wait(250);
    buzzer_beep(330, 200); wait(250);
    buzzer_beep(392, 400); wait(500);
    ESP_LOGI(TAG, "EXPECT: 3 distinct pitches (low→mid→high) + rising 3-note chord");
    ESP_LOGI(TAG, "Step 1 done — mark PASS or FAIL manually");
    wait(1000);

    // ── Step 2: Button A ──────────────────────────────────────
    log_step(2, "Button A (IO39) — press 3 times");
    ESP_LOGI(TAG, "Waiting for 3 presses on Button A...");
    if (!wait_presses(BTN_ID_1, 3)) all_pass = false;
    buzzer_beep(1000, 80);

    // ── Step 3: Button B ──────────────────────────────────────
    log_step(3, "Button B (IO40) — press 3 times");
    ESP_LOGI(TAG, "Waiting for 3 presses on Button B...");
    if (!wait_presses(BTN_ID_2, 3)) all_pass = false;
    buzzer_beep(1000, 80);

    // ── Step 4: Button C ──────────────────────────────────────
    log_step(4, "Button C (IO41) — press 3 times");
    ESP_LOGI(TAG, "Waiting for 3 presses on Button C...");
    if (!wait_presses(BTN_ID_3, 3)) all_pass = false;
    buzzer_beep(1000, 80);

    // ── Step 5: Encoder ───────────────────────────────────────
    log_step(5, "Encoder (IO35/36/37) — rotate CW 3 steps, CCW 3 steps, click 2×");
    ESP_LOGI(TAG, "Rotate CW 3 detents...");
    if (!wait_enc_rotate(3)) all_pass = false;
    ESP_LOGI(TAG, "Rotate CCW 3 detents...");
    if (!wait_enc_rotate(3)) all_pass = false;
    ESP_LOGI(TAG, "Click encoder button 2×...");
    if (!wait_enc_clicks(2)) all_pass = false;
    buzzer_beep(1000, 80);

    // ── Result ────────────────────────────────────────────────
    ESP_LOGI(TAG, "");
    if (all_pass) {
        ESP_LOGI(TAG, "╔══════════════════════════════╗");
        ESP_LOGI(TAG, "║   ALL STEPS PASSED ✓         ║");
        ESP_LOGI(TAG, "║   Flash main v7.1 firmware   ║");
        ESP_LOGI(TAG, "╚══════════════════════════════╝");
        // Victory melody
        buzzer_beep(523, 150); wait(180);
        buzzer_beep(659, 150); wait(180);
        buzzer_beep(784, 150); wait(180);
        buzzer_beep(1047, 400);
    } else {
        ESP_LOGE(TAG, "╔══════════════════════════════╗");
        ESP_LOGE(TAG, "║   ONE OR MORE STEPS FAILED   ║");
        ESP_LOGE(TAG, "║   Check wiring + pull-ups    ║");
        ESP_LOGE(TAG, "╚══════════════════════════════╝");
        buzzer_beep(200, 800);
    }

    vTaskDelete(nullptr);
}

extern "C" void app_main(void) {
    xTaskCreate(test_task, "test_periph", 8192, nullptr, 5, nullptr);
}
