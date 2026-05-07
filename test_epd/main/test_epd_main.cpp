// ============================================================
// EPD Driver Verification Test — Watcher v7.1
// ============================================================
// PURPOSE: Gate test. All 6 steps must pass before flashing main firmware.
//
// BUILD:
//   cd v7.1/test_epd
//   idf.py build
//
// FLASH (UART via FTDI, hold BOOT during connect if needed):
//   idf.py -p COM<N> flash monitor
//
// PASS CRITERIA (observe on display in order):
//   Step 1: Full white screen        — no garbage/ghosting
//   Step 2: Two concentric rects     — clean rectangles, no diagonal artifacts
//   Step 3: "8888" 7-segment digits  — all 7 segments visible on each digit
//   Step 4: Text strings             — readable, correct orientation
//   Step 5: Partial refresh x10      — counter increments, no heavy ghosting
//   Step 6: Dot ring (29/58 filled)  — correct clockwise fill from top-left
//
// If any step fails, check wiring before assuming driver bug:
//   MOSI=IO11, CLK=IO12, CS=IO10, DC=IO15, RST=IO16, BUSY=IO17
// ============================================================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "epd.h"
#include "fb.h"

static const char *TAG = "test_epd";

// ── Pin config — matches board_config.h ──────────────────────
#define PIN_MOSI  11
#define PIN_CLK   12
#define PIN_CS    10
#define PIN_DC    15
#define PIN_RST   16
#define PIN_BUSY  17

static fb_t fb;

static void log_step(int n, const char *name) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "══ Step %d: %s ══", n, name);
}

static void wait(int ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static void full(void) { fb_flush(&fb, EPD_REFRESH_FULL); }
static void part(void) { fb_flush(&fb, EPD_REFRESH_PARTIAL); }

static void test_task(void *) {
    ESP_LOGI(TAG, "=== Watcher v7.1 EPD Test ===");
    ESP_LOGI(TAG, "Display: GDEY042T81  400×300  SSD1683");
    ESP_LOGI(TAG, "Driver:  GxEPD2 ESP-IDF port (v7.1/components/epd)");

    epd_pins_t pins = { PIN_MOSI, PIN_CLK, PIN_CS, PIN_DC, PIN_RST, PIN_BUSY };
    epd_init(&pins);
    fb_init(&fb);

    // ── Step 1: Full white ────────────────────────────────────
    log_step(1, "Full white screen (full refresh)");
    fb_clear(&fb);
    full();
    ESP_LOGI(TAG, "EXPECT: blank white screen, no ghosting");
    wait(3000);

    // ── Step 2: Concentric border rects ──────────────────────
    log_step(2, "Two concentric border rectangles");
    fb_clear(&fb);
    fb_draw_rect(&fb,  5,  5, 390, 290, FB_BLACK);
    fb_draw_rect(&fb, 15, 15, 370, 270, FB_BLACK);
    full();
    ESP_LOGI(TAG, "EXPECT: two clean nested rectangles");
    wait(3000);

    // ── Step 3: 7-segment "8888" ─────────────────────────────
    log_step(3, "7-segment digits: 8888 (all segments lit)");
    fb_clear(&fb);
    // digit bounding box: 62×110px; layout matches main firmware clock screen
    fb_draw_7seg_digit(&fb,  18, 90, 8, FB_BLACK, FB_WHITE);
    fb_draw_7seg_digit(&fb,  90, 90, 8, FB_BLACK, FB_WHITE);
    fb_draw_7seg_colon(&fb, 163, 90, true, FB_BLACK);
    fb_draw_7seg_digit(&fb, 187, 90, 8, FB_BLACK, FB_WHITE);
    fb_draw_7seg_digit(&fb, 259, 90, 8, FB_BLACK, FB_WHITE);
    fb_draw_str_centered(&fb, 200, 215, "ALL SEGMENTS LIT", FB_BLACK);
    full();
    ESP_LOGI(TAG, "EXPECT: four digits with all 7 segments visible");
    wait(3000);

    // ── Step 4: Text rendering ────────────────────────────────
    log_step(4, "Text rendering — font, orientation, alignment");
    fb_clear(&fb);
    fb_draw_str_centered(&fb, 200,  30, "WATCHER v7.1", FB_BLACK);
    fb_draw_str_centered(&fb, 200,  50, "EPD DRIVER TEST", FB_BLACK);
    fb_draw_str_centered(&fb, 200,  70, "GDEY042T81 / SSD1683", FB_BLACK);
    fb_draw_hline(&fb, 10, 85, 380, FB_BLACK);
    fb_draw_str(&fb, 10, 100, "abcdefghijklmnopqrstuvwxyz", FB_BLACK);
    fb_draw_str(&fb, 10, 114, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", FB_BLACK);
    fb_draw_str(&fb, 10, 128, "0123456789 +-=:. !?", FB_BLACK);
    fb_draw_hline(&fb, 10, 143, 380, FB_BLACK);
    fb_draw_str_centered(&fb, 200, 158, "LEFT-TO-RIGHT, TOP-TO-BOTTOM", FB_BLACK);
    fb_draw_str_centered(&fb, 200, 178, "IF THIS IS UPSIDE-DOWN:", FB_BLACK);
    fb_draw_str_centered(&fb, 200, 194, "SWAP SCAN DIR IN GxEPD2 INIT", FB_BLACK);
    full();
    ESP_LOGI(TAG, "EXPECT: readable text, correct orientation");
    wait(4000);

    // ── Step 5: Partial refresh ───────────────────────────────
    log_step(5, "Partial refresh — 10 iterations, ~400ms each");
    fb_clear(&fb);
    fb_draw_str_centered(&fb, 200, 120, "PARTIAL REFRESH TEST", FB_BLACK);
    fb_draw_str_centered(&fb, 200, 140, "Counter should increment every 400ms", FB_BLACK);
    full();
    wait(2000);

    for (int i = 0; i <= 9; i++) {
        fb_clear(&fb);
        char msg[32];
        int idx = 0;
        const char *prefix = "PARTIAL  ";
        for (const char *p = prefix; *p && idx < 31; ++p) msg[idx++] = *p;
        int num = i;
        char numbuf[12];
        int np = 0;
        if (num == 0) numbuf[np++] = '0';
        else { int t = num; while (t > 0 && np < 11) { numbuf[np++] = (char)('0' + (t % 10)); t /= 10; } }
        for (int k = np - 1; k >= 0 && idx < 31; --k) msg[idx++] = numbuf[k];
        const char *suffix = " / 9";
        for (const char *p = suffix; *p && idx < 31; ++p) msg[idx++] = *p;
        msg[idx] = '\0';
        fb_draw_str_centered(&fb, 200, 130, msg, FB_BLACK);
        part();
        ESP_LOGI(TAG, "partial %d/9", i);
        wait(600);
    }
    ESP_LOGI(TAG, "EXPECT: counter incremented cleanly, minimal ghosting");
    wait(2000);

    // ── Step 6: Dot ring ─────────────────────────────────────
    log_step(6, "Dot ring — 30 of 60 squares filled (DAY_RING, clockwise)");
    fb_clear(&fb);
    fb_draw_dot_ring(&fb, 30, FB_BLACK, FB_WHITE);
    fb_draw_str_centered(&fb, 200, 136, "DOT RING 30/60", FB_BLACK);
    fb_draw_str_centered(&fb, 200, 153, "HALF FILLED CW", FB_BLACK);
    full();
    ESP_LOGI(TAG, "EXPECT: top+right arc filled, bottom+left arc empty");
    wait(4000);

    // ── Result screen ─────────────────────────────────────────
    fb_clear(&fb);
    fb_draw_rect(&fb, 5, 5, 390, 290, FB_BLACK);
    fb_draw_str_centered(&fb, 200,  60, "TEST COMPLETE", FB_BLACK);
    fb_draw_str_centered(&fb, 200,  85, "Check each step above.", FB_BLACK);
    fb_draw_str_centered(&fb, 200, 115, "ALL PASS →", FB_BLACK);
    fb_draw_str_centered(&fb, 200, 131, "flash main v7.1 firmware", FB_BLACK);
    fb_draw_str_centered(&fb, 200, 165, "ANY FAIL →", FB_BLACK);
    fb_draw_str_centered(&fb, 200, 181, "check wiring, then debug driver", FB_BLACK);
    full();
    ESP_LOGI(TAG, "=== TEST COMPLETE — see display ===");

    vTaskDelete(nullptr);
}

extern "C" void app_main(void) {
    xTaskCreate(test_task, "test_epd", 8192, nullptr, 5, nullptr);
}
